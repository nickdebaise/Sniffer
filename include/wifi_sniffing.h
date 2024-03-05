//
// Created by Nicholas Debaise on 2/15/24.
//

#ifndef SNIFFER_WIFI_SNIFFING_H
#define SNIFFER_WIFI_SNIFFING_H

#include "Arduino.h"

void WiFiSnifferPacketHandler(void* buff, wifi_promiscuous_pkt_type_t type);
std::string macToString(const uint8_t* mac);
bool isOUIValid(const uint8_t* oui);
void convertStringToBinaryOUI(const char* strOUI, uint8_t* binOUI);
int rssiToIndex(int rssi);

#endif //SNIFFER_WIFI_SNIFFING_H
