//
// Created by Nicholas DeBaise on 3/4/24.
//

#include "model.h"
#include "regressor.h"

Eloquent::ML::Port::RandomForestRegressor regressor;

/**
 * Run the model and return the predicted occupancy
 * @param useNeuralNet whether to use the neural network or not
 * @param numValidDevices the number of valid devices
 * @param numRandomDevices the number of random devices
 * @param N_v_curr the current valid RSSI counters
 * @param N_r_curr the current random RSSI counters
 * @return the predicted occupancy (int)
 */
int estimateOccupancy(ModelType modelType, int numValidDevices, int numRandomDevices, short N_v_curr[RSSI_BINS], short N_r_curr[RSSI_BINS]) {

    switch(modelType) {
        case NEURAL_NET:
            break;
        case REGRESSION:
        default:
            float X[2 + RSSI_BINS + RSSI_BINS];
            X[0] = numValidDevices;
            X[1] = numRandomDevices;
            for (int i = 0; i < RSSI_BINS; i++) {
                X[2 + i] = N_v_curr[i];
                X[2 + RSSI_BINS + i] = N_r_curr[i];
            }

            float y_pred = regressor.predict(X);

            return (int)y_pred;
    }

    return -1;
}