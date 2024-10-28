#include "application_layer.h"
#include "link_layer.h"
#include <string.h>  // for strncpy
#include <math.h>
#include <stdio.h>
#include <stdlib.h>  // For exit()

void applicationLayer(const char *serialPort, const char *role, int baudRate, int nTries, int timeout, const char *filename) {
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

    llopen(linklayer);
    if (linklayer.role == LlTx) {

        FILE* file = fopen(filename, "rb");
        if (file == NULL) exit(-1);

        // Determine the file size
        fseek(file, 0L, SEEK_END);
        long int fileSize = ftell(file);
        fseek(file, 0L, SEEK_SET); // get back to the beginning of the file

        int controlPacketSize;
        unsigned char* startControlPacket = createControlPacket(1, filename, fileSize, &controlPacketSize);
        if (llwrite(startControlPacket, controlPacketSize) == -1) {
            free(startControlPacket);
            fclose(file);
            exit(-1);
        }
        free(startControlPacket); // Free the control packet after use

        long int bytesRemaining = fileSize;
        int sequence = 0;

        // Allocate memory for the entire file content
        unsigned char* content = (unsigned char*) malloc(fileSize * sizeof(unsigned char));
        if (content == NULL) {
            fclose(file);
            exit(-1);
        }

        // Read the file into memory
        fread(content, sizeof(unsigned char), fileSize, file);
        fclose(file);

        // Loop to send data packets
        long int offset = 0;
        while (bytesRemaining > 0) {
            int writeSize = bytesRemaining > MAX_PAYLOAD_SIZE ? MAX_PAYLOAD_SIZE : bytesRemaining;

            unsigned char* data = (unsigned char*) malloc(writeSize * sizeof(unsigned char));
            if (data == NULL) {
                free(content);
                exit(-1);
            }
            
            memcpy(data, content + offset, writeSize);
            unsigned char* dataPacket = createDataPacket(sequence, writeSize, data);

            if (llwrite(dataPacket, writeSize + 4) == -1) {
                free(dataPacket);
                free(data);
                free(content);
                exit(-1);
            }
            free(dataPacket); // Free the data packet after sending
            free(data);       // Free the temporary data buffer
            bytesRemaining -= writeSize;
            offset += writeSize;
            sequence = (sequence + 1) % 255;
        }

        free(content); // Free the main file content buffer
        
        unsigned char *controlPacketEnd = createControlPacket(3, filename, fileSize, &controlPacketSize);
        if(llwrite(controlPacketEnd, controlPacketSize) == -1) { 
            printf("Exit: error in end packet\n");
            exit(-1);
        }
    } else {
        unsigned char *data = (unsigned char *)malloc(6 + 2*MAX_PAYLOAD_SIZE);
        int packetSize = llread(data);
        if (packetSize == -1) exit(-1);
        unsigned long int rxFileSize = 0;
        unsigned char* name = 0;
        readControlPacket(data, &rxFileSize, name); 
        FILE* newFile = fopen((char *) filename, "wb");
        while (1) {
            packetSize = llread(data);
            if(data[0] != 3){
                unsigned char *buf = (unsigned char*)malloc(packetSize);
                readDataPacket(buf, data, packetSize);
                fwrite(buf, sizeof(unsigned char), packetSize-4, newFile);
                free(buf);
            }
            else break;
        }
        fclose(newFile);
    }

    return;
}

unsigned char* createControlPacket(const unsigned int C, const char* name, long int length, int* packetSize) {
    int fileSize=0;
    unsigned int tempSize = length;
    while (tempSize>0){
        fileSize++;
        tempSize >>= 8;

    }
    int nameSize= strlen(name);
    *packetSize = fileSize+nameSize+5;
    
    printf("%d",*packetSize);
    unsigned char* controlPacket = (unsigned char*) malloc(*packetSize); // Dynamically allocate memory

    controlPacket[0] = C;

    unsigned int pos = 1;
    controlPacket[pos++] = 0x00;
    controlPacket[pos++] = fileSize;

    for (int i = 0; i < fileSize; i++) {
        controlPacket[pos + fileSize - 1 - i] = length & 0xFF;
        length >>= 8;
    }

    pos += fileSize;
    controlPacket[pos++] = 1;
    controlPacket[pos++] = nameSize;

    memcpy(controlPacket + pos, name, nameSize);
    
    return controlPacket;
}

unsigned char* createDataPacket(int sequence, long int writeSize, unsigned char* data) {
    unsigned char* dataPacket_ = (unsigned char*) malloc(writeSize + 4); // Dynamically allocate memory

    dataPacket_[0] = 2;
    dataPacket_[1] = sequence;
    dataPacket_[2] = (writeSize >> 8) & 0xFF;
    dataPacket_[3] = writeSize & 0xFF;

    memcpy(dataPacket_ + 4, data, writeSize);

    return dataPacket_;
}

void readControlPacket(const unsigned char* packet, unsigned long int *fileSize, unsigned char *name) {
    unsigned char bytesOfSize, bytesOfName;
    bytesOfSize = packet[2];
    unsigned char fileSizeData[bytesOfSize];
    memcpy(fileSizeData, packet+3, bytesOfSize);
    for(unsigned int i = 0; i < bytesOfSize; i++){
        *fileSize |= (fileSizeData[bytesOfSize-i-1] << (8*i));
    }
    bytesOfName = packet[bytesOfSize+4];
    name = (unsigned char*)malloc(bytesOfName);
    memcpy(name, packet+bytesOfSize+5, bytesOfName);
}

void readDataPacket(unsigned char* buf, const unsigned char* packet, const unsigned int size) {
    memcpy(buf,packet+4,size-4);
}