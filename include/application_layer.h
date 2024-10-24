// Application layer protocol header.
// NOTE: This file must not be changed.

#ifndef _APPLICATION_LAYER_H_
#define _APPLICATION_LAYER_H_

// Application layer main function.
// Arguments:
//   serialPort: Serial port name (e.g., /dev/ttyS0).
//   role: Application role {"tx", "rx"}.
//   baudrate: Baudrate of the serial port.
//   nTries: Maximum number of frame retries.
//   timeout: Frame timeout.
//   filename: Name of the file to send / receive.
void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename);
unsigned char * createControlPacket(const unsigned int C, const char* name, long int length, int* packetSize);
unsigned char* createDataPacket(int sequence, long int writeSize, unsigned char* data);

void readControlPacket(unsigned char* packet, unsigned long int fileSize);

void readDataPacket(unsigned char buf, const unsigned char* packet, const unsigned int size);
#endif // _APPLICATION_LAYER_H_
