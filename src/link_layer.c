// Link layer protocol implementation

#include "link_layer.h"
#include "serial_port.h"
#include <string.h>
#include <stdlib.h>

// Alarm 
#include "alarm.h"
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

// MISC
#define _POSIX_SOURCE 1 // POSIX compliant source

int ns = 0;


unsigned char * byteStuffing(const unsigned char *buf, int bufSize, int * contentSize,unsigned char * bcc){
    unsigned char* content = (unsigned char*) malloc(bufSize*2);
    int j = 0;
    for (int i = 0; i<bufSize; i++){
        if (buf[i] == FLAG){
            content[j] = 0x7d;
            j++;
            content[j] = 0x5e;
            j++;
        }
        else if (buf[i] == ESC){
            content[j] = 0x7d;
            j++;
            content[j] = 0x5d;
            j++;
        }
        else {
            content[j] = buf[i];
            j++;
        }
        *bcc ^= buf[i];
    }

    *contentSize = j;

    return content;
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
        if (uaState(byte, &check, 0)) break;
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
int uaState(unsigned char byte, int *check, int sender){
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
            if (byte==A1 || sender == 1){
                *check=2;
            }
            if (byte==A2 || sender == 0){
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
int setState(unsigned char byte, int *check){
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
    int fd_;
    if ((fd_ = openSerialPort(connectionParameters.serialPort,connectionParameters.baudRate)) < 0)
    {
        return -1;
    }
    const unsigned char buf [5] = {FLAG,A1,C1, A1 ^ C1, FLAG};

    if (connectionParameters.role == LlTx) {
        llsendSet(buf,sizeof(buf));
    }
    else {
        llsendUA();
    }
    return fd_;
}

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
int llwrite(const unsigned char *buf, int bufSize){
    alarmEnabled = 0;
    alarmCount = 0;
    unsigned char byte = 0;
    int check = 0;
    int responseCheck;

    unsigned char* content = (unsigned char*) malloc(bufSize*2 + 6); // worst case scenario all data is flags or esc
    content[0] = FLAG;
    content[1] = A1;
    content[2] = ns == 1 ? 0x80 : 0x00;
    content[3] = A1^content[2];

    int contentSize = 0;
    unsigned char bcc = 0;
    unsigned char * byteStuffedBuffer = byteStuffing(buf,bufSize,&contentSize,&bcc);
    memcpy(content+4,byteStuffedBuffer,contentSize);
    content[4+contentSize] = bcc;
    content[5+contentSize] = FLAG;

    while (alarmCount < 3){
        if (alarmEnabled == FALSE){
            alarm(4);
            if (writeBytesSerialPort(content, contentSize) == -1) return -1;
            sleep(1);
            alarmEnabled = TRUE;
        }
        int read = readByteSerialPort(&byte);
        if ( read == -1 ) return -1;
        else if (read == 0) continue;
        responseCheck = responseState(byte, &check);
        if (responseCheck == 1) break;
        else if (responseCheck == -1) alarmEnabled = FALSE;
    }

    sleep(1);

    return contentSize;
}

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
int llread(unsigned char *data){
    int STOP = FALSE;
    unsigned char byte = 0;
    int check = 0;
    unsigned char BCC = 0;
    unsigned char last = 0;
    int dataCheck;
    int i = 0;
    while (STOP == FALSE){
        int read = readByteSerialPort(&byte);
        if ( read == -1 ) return 1;
        if ( read == 0 ) continue;
        else printf("Data = 0x%02X\n", byte);
        dataCheck = receiveData(byte, &check, &BCC, &last, data ,&i);
        if (dataCheck != 0) STOP = TRUE;
    }
    unsigned char C;
    if (dataCheck == -1){
        if (ns == 1) C = REJ1;
        else C = REJ0;
    }else{
        if (ns == 1) C = RR1;
        else C = RR0;
    }
    printf("Received data ");
        printf("%d\n", i);
        unsigned char *buff = (unsigned char*)malloc(5);
        buff[0]=FLAG;
        buff[1]=A2;
        buff[2]=C;
        buff[3]=A2^C;
    buff[4]=FLAG;
    writeBytesSerialPort(buff,sizeof(buff));
    sleep(1);
    free(buff);
    return i;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
/* 
int llclose(int showStatistics) {
   if (connectionParameters.role == LlTx) {
       llsendDisc(0);
       alarmEnabled = 0;
       alarmCount = 0;
       while (alarmCount < 3) {
           if (alarmEnabled == FALSE) {
               alarm(4);
               if (writeBytesSerialPort(buf, bufSize) == -1) return 1;
               sleep(1);
               alarmEnabled = TRUE;
           }
       }
   }
   else {
       if (llsendDisc(1) == -1) {
           return 0;
       }
       int STOP = FALSE;
       unsigned char byte = 0;
       int check = 0;
       while (STOP == FALSE) {
           int read = readByteSerialPort(&byte);
           if ( read == -1 ) return 1;
           else if (read == 0) continue;
           else printf("Data = 0x%02X\n", byte);
           uaState (byte, &check, 1);
       }
   }
   int clstat = closeSerialPort();
   return clstat;
}
*/

////////////////////////////////////////////////
// RECEIVEDATA
////////////////////////////////////////////////
int receiveData(unsigned char byte, int*check, unsigned char *BCC, unsigned char *last, unsigned char* packet, int *i){
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
            }
            else if(byte==FLAG){
            *check=1;
            }
            else{
            *check=0;
            }
            break;
        case 4:
            if(byte==FLAG){
                if (*BCC==*last){
                *check=6;
                }else{
                *check=0;
                }
            }
            else if(byte==0x7D){
                    *check=5;
            }
            else{
                    if (*last != 0x7D){
                        packet[(*i)++] = *last;
                        *BCC = *BCC ^ *last;
                    }
                *last = byte;
            }
            break;
        case 5:
            if(byte==0x5D){ //esc
                packet[(*i)++] = 0x7D;
                *BCC = *BCC ^ 0x7D;
                *check=4;
            }else if(byte==0x5E){ //flag
                packet[(*i)++] = FLAG;
                *BCC = *BCC ^ FLAG;
                *check=4;
            }else{
                *check=0; //?
            }
            *last=0x7D;
            break;
    }
    if (*check == 6){ 
        ns = ns ^ 1;
        return 1;
    }
    return 0;
}

////////////////////////////////////////////////
// RESPONSESTATE
////////////////////////////////////////////////
int responseState(unsigned char byte, int*check){
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
            if ((byte == (A2^RR0)) | (byte == (A2 ^ RR1))){
                *check=4;
            }
            else if ((byte == (A2^REJ0)) | (byte == (A2^REJ1))){
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
// DISCSTATE
////////////////////////////////////////////////
int discState(unsigned char byte, int*check, int sender){
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
            if (byte==A2 || sender == 1){
            *check=2;
            }
            else if (byte == A1 || sender == 0){
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
            if (discState(byte, &check, 0)) break;
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
            if (discState(byte, &check, 1)) STOP = TRUE;
        }
        const unsigned char buf[5] = {FLAG,A1,DISC,A1^DISC,FLAG};
        writeBytesSerialPort(buf,sizeof(buf));
    }

   return 1;
}
