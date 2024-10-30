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

LinkLayer linkLayer;
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

    while (alarmCount < linkLayer.nRetransmissions){

        if (alarmEnabled == FALSE){
        
            alarm(4);
            if (writeBytesSerialPort(buf, bufSize) == -1) return 1;
            printf("Sending SET..\n");
            sleep(1);
            alarmEnabled = TRUE;
        }

        int read = readByteSerialPort(&byte);

        if ( read == -1 ) return 1;
        else if (read == 0) continue;
        if (uaState(byte, &check, 0)) break;
    }

    if (alarmCount == linkLayer.nRetransmissions) {
        printf("Max number of retransmissions reached...\n");
        exit(0);
    }

    if (check == 5) printf("Ua received \n");
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
        if (setState(byte, &check)){ 
            printf("Received SET \n");
            STOP = TRUE;
        }
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
            if (byte==A1 && sender == 1){
                *check=2;
            }
            if (byte==A2 && sender == 0){
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
            if ((byte == (A2^C2) && sender == 0) || (byte == (A1^C2) && sender == 1)){
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
    linkLayer = connectionParameters;
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
    while (alarmCount < linkLayer.nRetransmissions){
        if (alarmEnabled == FALSE){
            alarm(4);
            if (writeBytesSerialPort(content, contentSize+6) == -1) return -1;
            printf("Sending data packet I%d... \n\n",ns);
            sleep(1);
            alarmEnabled = TRUE;
        }
        int read = readByteSerialPort(&byte);
        if ( read == -1 ) return -1;
        else if (read == 0) continue;
        responseCheck = responseState(byte, &check);
        if (responseCheck == 1) break;
        else if (responseCheck == -1) {
            printf("Received REJ%d, proceding...\n",ns);
            alarmEnabled = FALSE;
        }
    }

    if (alarmCount == linkLayer.nRetransmissions) {
        printf("Max number of retransmissions reached...\n");
        exit(0);
    }

    printf("Received RR%d, proceding...\n",ns);

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
    int dataCheck;
    int i = 0;
    while (STOP == FALSE){
        int read = readByteSerialPort(&byte);
        if ( read == -1 ) return 1;
        if ( read == 0 ) continue;
        dataCheck = receiveData(byte, &check, data ,&i);
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
    printf("Received data of size ");
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

    if (dataCheck == -1){
        printf("Sending REJ%d...\n",ns);
    }
    else printf("Sending RR%d...\n",ns);

    return i;
}

////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
 
int llclose(LinkLayer linklayer, int showStatistics) {
    if (linklayer.role == LlTx) {
        if (llsendDiscTransmitter() == -1) exit(-1);
        const unsigned char buf[5] = {FLAG,A1,C2,A1^C2,FLAG};
        if (writeBytesSerialPort(buf, sizeof(buf)) == -1) return 1;
        sleep(1);
        printf("UA sended\n");
    }
   else {
        if (llsendDiscReceiver() == -1) exit(-1);
        /*
        unsigned char byte = 0;
        int STOP = FALSE;
        int check = 0;
        while (STOP == FALSE) {
            int read = readByteSerialPort(&byte);
            printf("byte :%d\n",byte);
            if (uaState(byte, &check, 1)) STOP = TRUE;
            else if ( read == -1 ) return 1;
            else continue;
        }
        printf("UA read\n");
        */
   }
   int clstat = closeSerialPort();
   return clstat;
}


////////////////////////////////////////////////
// RECEIVEDATA
////////////////////////////////////////////////
int receiveData(unsigned char byte, int*check, unsigned char* packet, int *i){
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
                unsigned char bcc= packet[0];
                unsigned char bcc2 = packet[*i-1];
                (*i)--;
                packet[*i] = '\0';
                for (unsigned int j = 1; j < *i; j++)
                    bcc ^= packet[j];
                if (bcc==bcc2){
                    *check=6;
                }else{
                    *check=0;
                }
            }
            else if(byte==0x7D){
                *check=5;
            }
            else{
                packet[(*i)++] = byte;
            }
            break;
        case 5:
            if(byte==0x5D){ //esc
                packet[(*i)++] = 0x7D;
                *check=4;
            }else if(byte==0x5E){ //flag
                packet[(*i)++] = FLAG;
                *check=4;
            }
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
            if (byte==A2 && sender == 0){
            *check=2;
            }
            else if (byte == A1 && sender == 1){
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
            if (byte == (A2^DISC) && sender == 0){
            *check=4;
            }
            else if (byte == (A1^DISC) && sender == 1){
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

int llsendDiscTransmitter(){
    int check = 0;
    int read = 0;
    alarmEnabled = 0;
    alarmCount = 0;
    unsigned char byte = 0;
    unsigned char *buf = (unsigned char*)malloc(5);
    buf[0]=FLAG;
    buf[1]=A1;
    buf[2]=DISC;
    buf[3]=A1^DISC;
    buf[4]=FLAG;
    while (alarmCount < linkLayer.nRetransmissions){
        if (alarmEnabled == FALSE){
            alarm(4);
            if (writeBytesSerialPort(buf,5) == -1) return -1;
            sleep(1);
            alarmEnabled = TRUE; 
        }
        read = readByteSerialPort(&byte);
        if (read == 1) {
            if (discState(byte, &check, 0)) break;
        }
        else if ( read == -1 ) return 1;
        else continue;
    }
    free(buf);
    if (alarmCount == linkLayer.nRetransmissions) {
        printf("Max number of retransmissions reached...\n");
        exit(0);
    }
    printf("disc Sended and read\n");
    return 1;
}

int llsendDiscReceiver(){
    int check = 0;
    int STOP = FALSE;
    unsigned char byte = 0;
    while (STOP == FALSE){

        int read = readByteSerialPort(&byte);
        if (read == 1) {
            if (discState(byte, &check,1)) STOP = TRUE;
        }
        if ( read == -1 ) return 1;
        else continue;

    }
    const unsigned char buf[5] = {FLAG,A2,DISC,A2^DISC,FLAG};
    writeBytesSerialPort(buf,sizeof(buf));
    sleep(1);
    printf("Sended Disc \n");
    return 0;
}