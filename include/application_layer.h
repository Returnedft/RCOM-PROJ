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

// Function for creating the control packet.
// Where C can be either 0 or 3 (Start or End control packet), name is the name of the file, length of the file, and the size of the packet created.
// Returns the Control Packet as a buffer.
unsigned char * createControlPacket(const unsigned int C, const char* name, long int length, int* packetSize);

// Function for creating the data packet.
// Sequence is the number of the frame to send, writeSize is the length of the data, and the data is the buffer containing the information.
// Returns the Data Packet as a buffer.
unsigned char* createDataPacket(int sequence, long int writeSize, unsigned char* data);


// Function for reading the Control Packet.
// Gets the name of the file, the size of the file and puts them in a buffer.
void readControlPacket(const unsigned char* packet, unsigned long int *fileSize, unsigned char *name);

// Function for reading the Data Packet.
// Removes unnecessary information from the data and stores it on buf.
void readDataPacket(unsigned char* buf, const unsigned char* packet, const unsigned int size);
#endif // _APPLICATION_LAYER_H_
