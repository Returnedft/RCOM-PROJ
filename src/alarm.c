#include "alarm.h"
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int alarmEnabled = 0;
int alarmCount = 0;

// Alarm function handler
void alarmHandler(int signal)
{
    alarmEnabled = 0;
    alarmCount++;

    printf("Alarm #%d\n", alarmCount);
}
