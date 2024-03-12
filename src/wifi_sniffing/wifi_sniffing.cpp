//
// Created by Nicholas Debaise on 2/15/24.
//
#include "Arduino.h"
#include "esp_wifi_types.h"
#include <unordered_set>
#include <string>
#include "organization_unique_identifiers.h"
#include "main.h"
#include "wifi_sniffing.h"

/**
 * Handle incoming packets sniffed by WiFi
 * @param buff the buffer containing the packet
 * @param type the type of packet
 */
void WiFiSnifferPacketHandler(void* buff, wifi_promiscuous_pkt_type_t type) {
    if (type != WIFI_PKT_MGMT) return;

    const wifi_promiscuous_pkt_t* ppkt = (wifi_promiscuous_pkt_t*)buff;
    const wifi_ieee80211_packet_t* ipkt = (wifi_ieee80211_packet_t*)ppkt->payload;
    const wifi_ieee80211_mac_hdr_t* hdr = &ipkt->hdr;

    if ((hdr->frame_ctrl & 0xFC) == 0x40) {
        int rssi = ppkt->rx_ctrl.rssi;
        int rssiIndex = rssiToIndex(rssi);


        char ouiStr[9];
        uint8_t binOUI[3];

        snprintf(ouiStr, sizeof(ouiStr), "%02X:%02X:%02X", hdr->addr2[0], hdr->addr2[1], hdr->addr2[2]);
        convertStringToBinaryOUI(ouiStr, binOUI);

        std::string addrStr = macToString(hdr->addr2);

        if (isOUIValid(binOUI)) {
            // update valid RSSI counters
            if(uniqueDevices.find(addrStr) == uniqueDevices.end()) {
                for (int i = 0; i <= rssiIndex; i++) {
                    N_v[i]++;
                }

                uniqueDevices.insert(addrStr);
            }

        } else {
            // Update randomized RSSI counters
            if(randomDevices.find(addrStr) == randomDevices.end()) {
                for (int i = 0; i <= rssiIndex; i++) {
                    N_r[i]++;
                }

                randomDevices.insert(addrStr);
            }
        }
    }
}

/**
 * Given a pointer to a 3-byte OUI in memory, return if it matches any known/pre-determined manufacturer OUI
 * @param oui the OUI to check
 * @return true if the OUI is valid, false otherwise
 */
bool isOUIValid(const uint8_t* oui) {
    for (size_t i = 0; i < numberOfOUIs; i++) {
        if (memcmp(oui, validOUIs[i], 3) == 0) {
            return true; // OUI found
        }
    }
    return false; // OUI not found
}

/**
 * Given a string (i.e. AF:12:BE), store each individual section (byte) of the OUI in memory
 * @param strOUI the string representation of the OUI
 * @param binOUI the binary representation of the OUI to store
 */
void convertStringToBinaryOUI(const char* strOUI, uint8_t* binOUI) {
    sscanf(strOUI, "%hhx:%hhx:%hhx", &binOUI[0], &binOUI[1], &binOUI[2]);
}

/**
 * Map the RSSI value to the corresponding bin index.
 * This function assumes RSSI values are evenly distributed across the bins.
 * @param rssi the RSSI value to map
 * @return the index of the RSSI value in the bins
 */
int rssiToIndex(int rssi) {
    if (rssi < RSSI_MIN) rssi = RSSI_MIN;
    if (rssi > RSSI_MAX) rssi = RSSI_MAX;
    int index = (rssi - RSSI_MIN) / ((RSSI_MAX - RSSI_MIN + 1) / RSSI_BINS);
    return index;
}

/**
 * Convert a MAC address to a string
 * @param mac the MAC address to convert
 * @return the string representation of the MAC address
 */
std::string macToString(const uint8_t* mac) {
    char buf[18];  // Format: XX:XX:XX:XX:XX:XX\0
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return std::string(buf);
}
