//
// Created by Nicholas Debaise on 2/15/24.
//
#include "Arduino.h"
#include <ctime>
#include "timing.h"

/**
 * Wait for the device to sync with NTP time
 */
void waitForNTPSync() {
    Serial.print("Waiting for time sync via pool.ntp.org");
    while (time(nullptr) < 8 * 3600 * 2) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println("\nTime synced");
    printFormattedTime();
}



/**
 * Calculate the next scan time for the device
 * Increments in steps of 15 i.e. 9:00, 9:15, 9:30, 9:45, etc.
 * @param step: the step to increment next scan time to
 */
unsigned long calculateNextScanTime(int step) {
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    int minutesToNextStep = step - (timeinfo.tm_min % step);
    if (minutesToNextStep == step) minutesToNextStep = 0;

    timeinfo.tm_min += minutesToNextStep;
    timeinfo.tm_sec = 0;

    time_t nextScanEpoch = mktime(&timeinfo);


    Serial.print("Next scan scheduled at: ");
    Serial.println(ctime(&nextScanEpoch));

    return nextScanEpoch * 1000;
}


/**
 * Print the current time in a formatted string
 * e.g. 02/15/2024 12:00:00
 */
void printFormattedTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    char buffer[30];
    strftime(buffer, sizeof(buffer), "%m/%d/%Y %H:%M:%S", &timeinfo);
    Serial.print("Current time: ");
    Serial.println(buffer);
}
