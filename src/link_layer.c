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

int ns = 0;


unsigned char * byteStuffing(const unsigned char *buf, int bufSize){
    
}

////////////////////////////////////////////////
// LLSENDSET 
////////////////////////////////////////////////
int llsendSet(const unsigned char *buf, int bufSize){

    signal(SIGALRM, alarmHandler);
    alarmEnabled = 0;

    unsigned char byte = 0;
    int check = 0;

    while (alarmCount < 3){

        if (alarmEnabled == FALSE){
        
          alarm(4);
          if (writeBytesSerialPort(buf, bufSize) == -1) return 1;
          sleep(1);
          alarmEnabled = TRUE;
        }

        int read = readByteSerialPort(&byte);

        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        if (uaState(byte, &check)) break;
    }

    if (check == 5 ) printf("Ua received \n");
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
        if (setState(byte, &check)) STOP = TRUE;

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
int uaState(int byte, int *check){
    switch(*check){
               case 0:
                   if (byte==FLAG){
                       *check=1;
                   }
                   else{
                       *check=0;
                   }
                   break;
               case 1:
                   if (byte==A2){
                       *check=2;
                   }
                   else if(byte==FLAG){
                       *check=1;
                   }
                   else{
                       *check=0;
                   }
                   break;
               case 2:
                   if (byte==C2){
                       *check=3;
                   }
                   else if(byte==FLAG){
                       *check=1;
                   }
                   else{
                       *check=0;
                   }
                   break;
               case 3:
                   if (byte == (A2^C2)){
                       *check=4;
                   }
                   else if(byte==FLAG){
                       *check=1;
                   }
                   else{
                       *check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                       *check=5;
                   }
                   else{
                       *check=0;
                   }
                   break;
           }    

    return (*check == 5) ? 1 : 0;
}

////////////////////////////////////////////////
// SETSTATE
////////////////////////////////////////////////
int setState(int byte, int *check){
    switch(*check){
               case 0:
                   if (byte==FLAG){
                       *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 1:
                   if (byte==A1){
                    *check=2;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 2:
                   if (byte==C1){
                    *check=3;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 3:
                   if (byte == (A1^C1)){
                    *check=4;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                    *check=5;
                   }
                   else{
                    *check=0;
                   }
                   break;
           }    

    return (*check == 5) ? 1 : 0;
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

    if (connectionParameters.role == LlTx) {
        llsendSet(buf,sizeof(buf));
    }
    else {
        if (llsendUA() == -1){
            return -1;
        }
    }
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize)
{
    alarmEnabled = 0;
    alarmCount = 0;
    unsigned char byte = 0;
    int check = 0;
    int responseCheck;

    while (alarmCount < 3){

        if (alarmEnabled == FALSE){
        
          alarm(4);
          if (writeBytesSerialPort(buf, bufSize) == -1) return 1;
          sleep(1);
          alarmEnabled = TRUE;
        }

        int read = readByteSerialPort(&byte);

        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        printf("Response = 0x%02X\n", byte);
        responseCheck = responseState(byte, &check);
        if (responseCheck == 1) break;
        else if (responseCheck == -1) alarmEnabled = FALSE;
    }

    printf("ns = %d\n", ns);

    sleep(1);

    return bufSize;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *packet)
{
    int STOP = FALSE;
    unsigned char byte = 0;
    int check = 0;
    unsigned char BCC = 0;
    unsigned char last = 0;
    int dataCheck;
    while (STOP == FALSE){
        int read = readByteSerialPort(&byte);
        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        else printf("Data = 0x%02X\n", byte);
        dataCheck = receiveData(byte, &check, &BCC, &last);
        if (dataCheck != 0) STOP = TRUE;
    }
    unsigned char C;
    if (dataCheck == -1){
        if (ns == 1) C = REJ1;
        else C = REJ0;
    }else{
        printf("Received data \n");
        if (ns == 1) C = RR1;
        else C = RR0;
    }
    const unsigned char buf[5] = {FLAG,A2,C,A2^C,FLAG};

    writeBytesSerialPort(buf,sizeof(buf));
    sleep(1);

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

////////////////////////////////////////////////
// RECEIVEDATA
////////////////////////////////////////////////
int receiveData(unsigned char byte, int*check, unsigned char *BCC, unsigned char *last){
    unsigned char infoFrame;
    if (ns==0){
        infoFrame = 0x00;
    }
    else{
        infoFrame = 0x80;
    }
    switch(*check){
        case 0:
            if (byte==FLAG){
                printf("1");
            *check=1;
            }
            else{
            *check=0;
            }
            break;
        case 1:
            if (byte==A1){
                printf("2");
            *check=2;
            }
            else if(byte==FLAG){
            *check=1;
            }
            else{
            *check=0;
            }
            break;
        case 2:
            printf("ns = %d", ns);
            if (byte==infoFrame){
            *check=3;
            }
            else if(byte==FLAG){
            *check=1;
            }
            else{
            *check=0;
            }
            break;
        case 3:
            if (byte==(A1 ^ infoFrame)){
            *check=4;
                printf("4");
            }
            else if(byte==FLAG){
            *check=1;
            }
            else{
            *check=0;
            }
            break;
        case 4:
            printf("data");
            if(byte==FLAG){
                if (*BCC==*last){
                *check=5;
                }else{
                *check=0;
                }
            }
            else{
                *BCC = *BCC ^ *last;
                *last = byte;
            }
            break;
    }
    if (*check == 5){ 
        ns = ns ^ 1;
        return 1;
    }
    return 0;
}

////////////////////////////////////////////////
// RESPONSESTATE
////////////////////////////////////////////////
int responseState(int byte, int*check){

    switch(*check){
            case 0:
                printf("0");
                if (byte==FLAG){
                    *check=1;
                }
                else{
                    *check=0;
                }
                break;
            case 1:
            printf("1");
                if (byte==A2){
                    *check=2;
                }
                else if(byte==FLAG){
                    *check=1;
                }
                else{
                    *check=0;
                }
                break;
            case 2:
                if (byte==RR0){
                    *check=3;
                    ns = 0;
                }
                else if (byte == RR1){
                    *check = 3;
                    ns = 1;
                }
                else if (byte==REJ0){
                    *check=3;
                    ns = 1;
                }
                else if (byte == REJ1){
                    *check = 3;
                    ns = 0;

                }
                else if(byte==FLAG){
                    *check=1;
                }
                else{
                    *check=0;
                }
                break;
            case 3:
                if (byte == (A2^RR0) | byte == (A2 ^ RR1)){
                    *check=4;
                }
                else if (byte == (A2^REJ0) | byte == (A2^REJ1)){
                    *check=4;
                }
                else if(byte==FLAG){
                    *check=1;
                    return -1;
                }
                else{
                    *check=0;
                    return -1;
                }
                break;
            case 4:
            printf("4");
                if (byte==FLAG){
                    *check=5;
                }
                else{
                    *check=0;
                }
                break;
        }
    return (*check == 5) ? 1 : 0;
}

////////////////////////////////////////////////
// TRANSMITERDISCSTATE
////////////////////////////////////////////////
int transmiterDiscState(const unsigned char *byte, int*check){
    switch( *check){
               case 0:
                   if (byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 1:
                   if (byte==A2){
                    *check=2;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 2:
                   if (byte==DISC){
                    *check=3;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 3:
                   if (byte == (A2^DISC)){
                    *check=4;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                    *check=5;
                   }
                   else{
                    *check=0;
                   }
                   break;
           }    

    return (*check == 5) ? 1 : 0;
}

////////////////////////////////////////////////
// RECEIVERDISCSTATE
////////////////////////////////////////////////
int receiverDiscState(int byte, int*check){
    switch(*check){
               case 0:
                   if (byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 1:
                   if (byte==A1){
                    *check=2;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 2:
                   if (byte==DISC){
                    *check=3;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 3:
                   if (byte == (A1^DISC)){
                    *check=4;
                   }
                   else if(byte==FLAG){
                    *check=1;
                   }
                   else{
                    *check=0;
                   }
                   break;
               case 4:
                   if (byte==FLAG){
                    *check=5;
                   }
                   else{
                    *check=0;
                   }
                   break;
           }    

    return (*check == 5) ? 1 : 0;
}

int llsendDisc(int *sender){
    int check = 0;
    if (sender==0){
        unsigned char byte = 0;
        const unsigned char bufs [5] = {FLAG,A1,DISC, A1 ^ DISC, FLAG};
        while (alarmCount < 3){
            if (alarmEnabled == FALSE){
                alarm(4);
            if (writeBytesSerialPort(bufs, sizeof(bufs)) == -1) return 1;
                sleep(1);
                alarmEnabled = TRUE;
            }
            int read = readByteSerialPort(&byte);
            if ( read == -1 ) return 1;
            else if (read == 0) continue;
            if (transmiterDiscState(bufs, &check)) break;
        }

    }
    else{
        int STOP = FALSE;
        unsigned char byte = 0;
        int check = 0;
        while (STOP == FALSE){
            int read = readByteSerialPort(&byte);
            if ( read == -1 ) return 1;
            else if (read == 0) continue;
            if (receiverDiscState(byte, &check)) STOP = TRUE;
        }
        const unsigned char buf[5] = {FLAG,A1,DISC,A1^DISC,FLAG};
        writeBytesSerialPort(buf,sizeof(buf));
    }

    return 1;
}

