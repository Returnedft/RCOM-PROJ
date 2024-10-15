// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"

// Alarm 
#include "alarm.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

////////////////////////////////////////////////
// LLSENDSET 
////////////////////////////////////////////////
int llsendSet(const unsigned char *buf, int bufSize){

    unsigned char byte = 0;
    int check = 0;

    while (alarmCount < 4){
        if (alarmEnabled == FALSE){
          alarm(3);
          alarmEnabled = TRUE;
          writeBytesSerialPort(buf, bufSize);
          sleep(1);
        }

        int read = readByteSerialPort(&byte);

        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        if (uaState(byte, check)) break;
    }

    printf("Ua received \n");
    sleep(1);

    return 0;
}
////////////////////////////////////////////////
// LLSENDUA
////////////////////////////////////////////////
int llsendUA(){

    int STOP = FALSE;
    unsigned char byte = 0;
    int check = 0;

    while (STOP == FALSE){

        int read = readByteSerialPort(&byte);

        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        if (setState(byte, check)) STOP = TRUE;

    }
    const unsigned char buf[5] = {FLAG,A2,C2,A2^C2,FLAG};

    writeBytesSerialPort(buf,sizeof(buf));
    sleep(1);
    printf("Sended UA \n");

    return 0;
}

////////////////////////////////////////////////
// UASTATE
////////////////////////////////////////////////
int uaState(int byte, int check){
    switch(check){
               case 0:
                   if (byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 1:
                   if (byte==A2){
                       check=2;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 2:
                   if (byte==C2){
                       check=3;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 3:
                   if (byte == (A2^C2)){
                       check=4;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                       check=5;
                   }
                   else{
                       check=0;
                   }
                   break;
           }    

    return (check == 5) ? 1 : 0;
}

////////////////////////////////////////////////
// SETSTATE
////////////////////////////////////////////////

int setState(int byte, int check){
    switch(check){
               case 0:
                   if (byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 1:
                   if (byte==A1){
                       check=2;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 2:
                   if (byte==C1){
                       check=3;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 3:
                   if (byte == (A1^C1)){
                       check=4;
                   }
                   else if(byte==FLAG){
                       check=1;
                   }
                   else{
                       check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                       check=5;
                   }
                   else{
                       check=0;
                   }
                   break;
           }    

    return (check == 5) ? 1 : 0;
}


////////////////////////////////////////////////
// LLOPEN
////////////////////////////////////////////////
int llopen(LinkLayer connectionParameters)
{
    if (openSerialPort(connectionParameters.serialPort,
                       connectionParameters.baudRate) < 0)
    {
        return -1;
    }
    const unsigned char buf [5] = {FLAG,A1,C1, A1 ^ C1, FLAG};

    llsendSet(buf,sizeof(buf));
    llsendUA();

    return 1;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{


    return bufSize;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    // TODO

    return 0;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
int llclose(int showStatistics)
{
    // TODO

    int clstat = closeSerialPort();
    return clstat;
}

