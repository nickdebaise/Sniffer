//
// Created by Nicholas Debaise on 2/15/24.
//

#ifndef SNIFFER_TIMING_H
#define SNIFFER_TIMING_H

/* === */
/* Function declarations */
/* === */
void waitForNTPSync();
void printFormattedTime();
unsigned long calculateNextScanTime(int step);

#endif //SNIFFER_TIMING_H
