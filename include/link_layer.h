// Link layer header.
// NOTE: This file must not be changed.

#ifndef _LINK_LAYER_H_
#define _LINK_LAYER_H_

typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

extern int numberOfFrames;
extern int numberOfRetransmissions;
extern int numberOfTimeouts;
extern long int timer;
extern int ns;

// SIZE of maximum acceptable payload.
// Maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000

// MISC
#define FALSE 0
#define TRUE 1
#define C1 0x03
#define C2 0x07
#define A1 0x03
#define A2 0x01
#define FLAG 0X7E
#define RR0 0xAA
#define RR1 0xAB
#define REJ0 0x54
#define REJ1 0X55
#define DISC 0x0B
#define ESC 0x7D

// Open a connection using the "port" parameters defined in struct linkLayer.
// Return "1" on success or "-1" on error.
int llopen(LinkLayer connectionParameters);

// Send SET(Set-up) in buf with size bufSize.
// Return "1" on sucess or "-1" on error.
int llsendSet(const unsigned char *buf, int bufSize);

// Send UA(Set-up) in buf with size bufSize.
// Return "1" on sucess or "-1" on error.
int llsendUA();

//State Machine for Set command
//Return "1" when all Set command correct or "0" if not
int setState(unsigned char byte, int *check);

//State Machine for UA command
//Return "1" when all UA command correct or "0" if not
int uaState(unsigned char byte, int *check, int sender);

// Send data in buf with size bufSize.
// Return number of chars written, or "-1" on error.
int llwrite(const unsigned char *buf, int bufSize);

// Receive data in packet.
// Return number of chars read, or "-1" on error.
int llread(unsigned char *data);

// Close previously opened connection.
// if showStatistics == TRUE, link layer should print statistics in the console on close.
// Return "1" on success or "-1" on error.
int llclose(LinkLayer linklayer, int showStatistics);

//State Machine for Information frames
//Return "1" when all Information frame is correct, "0" if it is processing or "-1" if there is a problem with the BCC2
int receiveData(unsigned char byte, int*check, unsigned char* packet, int *i);

//State Machine for information frames
//Return "1" when all information frame is correct, "0" if it is processing or "-1" if there is a problem with the BCC2
int responseState(unsigned char byte, int *check, int *C);

//State Machine for Disc command
//Return "1" when all Disc command correct or "0" if not
int discState(unsigned char byte, int *check, int sender);

// Send the receiver DISC(Set-up) in buf with size bufSize.
// Return "1" on sucess or "-1" on error.
int llsendDiscReceiver();

// Send the transmitter DISC(Set-up) in buf with size bufSize.
// Return "1" on sucess or "-1" on error.
int llsendDiscTransmitter();

// Receive the UA command.
// Return "1" on sucess or "-1" on error.
int receiveUA();

#endif // _LINK_LAYER_H_
