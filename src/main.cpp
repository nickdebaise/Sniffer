#include "Arduino.h"
#include "HTTPClient.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include <ctime>
#include <unordered_set>
#include <string>
#include "main.h"
#include "timing.h"
#include "wifi_sniffing.h"
#include <ArduinoJson.h>
#include "model.h"


/* ====== */
/* Variable declarations */
/* ====== */
const char* SSID = "Union-Guest";    // SSID of network
const char* password = "UNION1795";  // Password of network

std::unordered_set<std::string> uniqueDevices;  // stores the unique device set when scanning for devices
std::unordered_set<std::string> randomDevices;  // stores the random device set when scanning for devices
const long wifiScanTime = 9 * 60 * 1000;        // 9 minutes in milliseconds
const long delayTime = 10 * 1000;               // 10 seconds in milliseconds
unsigned long nextScanTime = 0;                 // Stores the next scheduled scan time in milliseconds

short N_r[RSSI_BINS] = {};
short N_v[RSSI_BINS] = {};


const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -3600 * 5;
const int daylightOffset_sec = 0;


/* ======= */
/* Function Implementations */
/* ======= */
void performWiFiScanning();
void uploadData(int estimate);
void reconnectToWifi();


void setup() {
    Serial.begin(9600);
    delay(5000);

    Serial.println("------BEGIN------");
    pinMode(LED_BUILTIN, OUTPUT);

    // Connect to Wi-Fi
    WiFi.disconnect();
    delay(100);
    WiFi.begin(SSID, password);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected to Wi-Fi");
    Serial.println("");

    // Setup time from ntp server
    Serial.println("Setting up NTP server for RTC");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    waitForNTPSync();

    // Calculate the initial scan time
    nextScanTime = calculateNextScanTime(15);

    // Setup for WiFi scanning
    Serial.println("Setting up WiFi Station");

    WiFi.mode(WIFI_STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(WiFiSnifferPacketHandler);
    esp_wifi_set_promiscuous(false);

    WiFi.disconnect();

    Serial.println("WiFi station finished setup");
}

void loop() {
    unsigned long currentEpoch = (unsigned long)time(nullptr);


    if (currentEpoch * 1000 >= nextScanTime) {
        uniqueDevices.clear();
        randomDevices.clear();

        performWiFiScanning();

        int numUniqueWiFiDevices = uniqueDevices.size();
        int numRandomDevices = randomDevices.size();
        uniqueDevices.clear();
        randomDevices.clear();
        delay(2000);

        Serial.print("Unique WiFi devices found: ");
        Serial.println(numUniqueWiFiDevices);
        Serial.print("Random WiFi MAC addresses found: ");
        Serial.println(numRandomDevices);

        reconnectToWifi();

        // get estimate
        int occupancyEstimate = estimateOccupancy(ModelType::REGRESSION,numUniqueWiFiDevices, numRandomDevices, N_v, N_r);

        // Uploading data to server
        uploadData(occupancyEstimate);

        WiFi.disconnect();

        // find next time
        nextScanTime = calculateNextScanTime(15);
        delay(delayTime);
    } else {
        delay(2000);
        Serial.print("|");
    }

}

/**
 * Perform WiFi scanning for 9 minutes
 */
void performWiFiScanning() {
    Serial.println("\nStarting WiFi scan.");

    unsigned long startMillis = millis();
    unsigned long currentMillis = startMillis;
    unsigned long lastChannelSwitchTime = startMillis;

    bool ledState = false;
    unsigned long lastBlinkTime = 0;

    const int channelSwitchInterval = wifiScanTime / 3;
    const int channels[3] = {1, 6, 11};
    int currentChannelIndex = 0;

    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channels[currentChannelIndex], WIFI_SECOND_CHAN_NONE); // Set initial channel

    // Use millis() for non-blocking delay
    while (currentMillis - startMillis < wifiScanTime) {
        currentMillis = millis();

        // Switch channel every channelSwitchInterval
        if (currentMillis - lastChannelSwitchTime >= channelSwitchInterval) {
            currentChannelIndex = (currentChannelIndex + 1) % 3; // Cycle through channels 1, 6, 11
            WiFi.disconnect(); // ensure wifi stops operations on curr channel
            esp_wifi_set_channel(channels[currentChannelIndex], WIFI_SECOND_CHAN_NONE);
            Serial.print("Switched to channel ");
            Serial.println(channels[currentChannelIndex]);
            lastChannelSwitchTime = currentMillis;
        }

        // Check if it's time to toggle the LED
        if (currentMillis - lastBlinkTime >= BLINK_INTERVAL) {
            ledState = !ledState;
            digitalWrite(LED_BUILTIN, ledState);
            lastBlinkTime = currentMillis;
        }
    }

    esp_wifi_set_promiscuous(false);
    digitalWrite(LED_BUILTIN, LOW);

    Serial.println("WiFi scan complete.");
}

/**
 * Upload data to server
 * Uses backoff algorithm to retry sending data
 * @param estimate the occupancy estimate from the ML model
 */
void uploadData(int estimate) {
    const int maxRetries = 5;
    int retryDelay = 1000;
    const String serverURL = "https://us-central1-scholarsproject-1f3ff.cloudfunctions.net/uploadWifiEstimates";
    int retries = 0;

    WiFi.disconnect();
    delay(100);
    WiFi.reconnect();

    while(retries < maxRetries) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("Wi-Fi not connected. Attempting to reconnect...");
            reconnectToWifi();
        }

        if (WiFi.status() == WL_CONNECTED) {
            HTTPClient http;
            http.begin(serverURL);
            http.addHeader("Content-Type", "application/x-www-form-urlencoded");
            Serial.println("Sending HTTP POST to: " + serverURL);

            String httpRequestData = "estimate=" + String(estimate);

            Serial.println("Request data = " + httpRequestData);
            int httpResponseCode = http.POST(httpRequestData);

            if (httpResponseCode > 0) {
                String response = http.getString();
                Serial.println("Data sent successfully");
                break;
            } else {
                Serial.print("Error on sending POST: ");
                Serial.println(httpResponseCode);
                Serial.println(http.errorToString(httpResponseCode)); // Print error details if available
            }
            http.end();
        } else {
            Serial.println("Wi-Fi not connected. Cannot upload data.");
            break;
        }

        retries++;
        if(retries < maxRetries) {
            Serial.println("Retry in " + String(retryDelay) + " milliseconds...");
            delay(retryDelay);
            retryDelay *= 2;
        }
    }

    if(retries == maxRetries) {
        Serial.println("Failed to send data after " + String(maxRetries) + " attempts.");
    }
}

/**
 * Reconnect to Wi-Fi if not connected
 * Will attempt to reconnect to Wi-Fi for 100 seconds
 */
void reconnectToWifi() {
    // Check Wi-Fi connection and reconnect if necessary
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to Wi-Fi...");
        WiFi.disconnect();
        delay(1000);
        WiFi.begin(SSID, password);

        int reconnectAttempt = 0;
        while (WiFi.status() != WL_CONNECTED && reconnectAttempt < 100) {
            delay(1000);
            Serial.print(".");
            reconnectAttempt++;
        }
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nFailed to reconnect to Wi-Fi.");
            return;
        }
        Serial.println("\nReconnected to Wi-Fi.");
    }
}

