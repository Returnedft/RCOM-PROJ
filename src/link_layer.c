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

////////////////////////////////////////////////
// RECEIVEDATA
////////////////////////////////////////////////
int receiveData(unsigned char byte, int check, int n, unsigned char *BCC, unsigned char *last){
    unsigned char infoFrame;
    if (n==0){
        infoFrame = 0x00;
    }
    else{
        infoFrame = 0x80;
    }
    switch(check){
        case 0:
            if (byte==FLAG){
                printf("1");
                check=1;
            }
            else{
                check=0;
            }
            break;
        case 1:
            if (byte==A1){
                printf("2");
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
            if (byte==infoFrame){
                check=3;
                printf("3");
            }
            else if(byte==FLAG){
                check=1;
            }
            else{
                check=0;
            }
            break;
        case 3:
            if (byte==(A1 ^ infoFrame)){
                check=4;
                printf("4");
            }
            else if(byte==FLAG){
                check=1;
            }
            else{
                check=0;
            }
            break;
        case 4:
            printf("data");
            if(byte==FLAG){
                if (*BCC==*last){
                    check=5;
                }else{
                    check=0;
                }
            }
            else{
                *BCC = *BCC ^ *last;
                *last = byte;
            }
            break;
    }
    return (check == 5) ? 1 : 0;
}

////////////////////////////////////////////////
// RESPONSESTATE
////////////////////////////////////////////////
int responseState(int byte, int check, int n, int *error){
    unsigned char RR, REJ;
    if (n==0){
        RR = RR0;
        REJ = REJ0;
    }else{
        RR = RR1;
        REJ = REJ1;
    }
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
                if (byte==RR){
                    check=3;
                    *error=0;
                }
                else if (byte==REJ){
                    check=3;
                    *error=1;
                }
                else if(byte==FLAG){
                    check=1;
                }
                else{
                    check=0;
                }
                break;
            case 3:
                if (*error == 0 || byte == (A2^RR)){
                    check=4;
                }
                else if (*error == 1 || byte == (A2^REJ)){
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
// TRANSMITERDISCSTATE
////////////////////////////////////////////////
int transmiterDiscState(int byte, int check){
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
                   if (byte==DISC){
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
                   if (byte == (A2^DISC)){
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
// RECEIVERDISCSTATE
////////////////////////////////////////////////
int receiverDiscState(int byte, int check){
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
                   if (byte==DISC){
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
                   if (byte == (A1^DISC)){
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