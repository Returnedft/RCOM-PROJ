// Application layer protocol implementation

#include "application_layer.h"
#include "link_layer.h"
#include <string.h>  // for strncpy

#include <stdio.h>

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{
    LinkLayer linklayer;

    // Copy serialPort string into the struct safely
    strncpy(linklayer.serialPort, serialPort, sizeof(linklayer.serialPort) - 1);

    // Map role string ("tx" or "rx") to enum
    if (strcmp(role, "tx") == 0) {
        linklayer.role = LlTx;
    } else if (strcmp(role, "rx") == 0) {
        linklayer.role = LlRx;
    } else {
        printf("Error: Invalid role '%s'. Use 'tx' for transmitter or 'rx' for receiver.\n", role);
        return;
    }

    // Assign other parameters
    linklayer.baudRate = baudRate;
    linklayer.nRetransmissions = nTries;
    linklayer.timeout = timeout;

    // Call llopen with the properly initialized linklayer
    llopen(linklayer);
}
