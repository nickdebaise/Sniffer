//
// Created by Nicholas Debaise on 3/4/24.
//

#ifndef SNIFFER_MODEL_H
#define SNIFFER_MODEL_H

#include "main.h"

int estimateOccupancy(ModelType modelType, int numValidDevices, int numRandomDevices, short N_v_curr[RSSI_BINS], short N_r_curr[RSSI_BINS]);

#endif //SNIFFER_MODEL_H
