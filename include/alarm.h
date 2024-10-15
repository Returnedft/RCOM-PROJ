#ifndef _ALARM_H_
#define _ALARM_H_

extern int alarmEnabled;
extern int alarmCount;

// Handler for the alarm
void alarmHandler(int signal);

#endif
