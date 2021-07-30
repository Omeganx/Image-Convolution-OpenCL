#ifndef PERFORMANCELOGGER_H
#define PERFORMANCELOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>

#define CLOCK_REALTIME 0

/* 
 * TimeLog structure that will store the time a function took to compute.
 */

typedef struct timeLog timeLog_t;
struct timeLog
{
    char *functionName;
    clock_t timeStart;
    double totalTime_ms;
    int isActive;

    timeLog_t *nextTimeLogEntry;
    timeLog_t *childrenEntry;

    int depth;
};

/* 
 * Linked List that will store all the timeLog structs
 */
typedef struct timeLogList timeLogList_t;
struct timeLogList
{
    timeLog_t *head;
};

void putTolog(char *name, double time, timeLogList_t **timeLogs);
void put_log(char *name, double time, timeLog_t **timeLog, int depth);
void start_log(char *name, timeLogList_t **timeLogs);
void stop_log(char *name, timeLogList_t **timeLogs);
void freelog(timeLogList_t **timeLogs);
void freelog_el(timeLog_t **timeLog);
void add_log(char *name, timeLog_t **timeLog, int depth);
void end_log(char *name, timeLog_t **timeLog);

void display_log(timeLog_t **timeLog);

void display_timeLog(timeLogList_t **timeLogs);

void startTimeLog(char *name);
void stopTimeLog(char *name);
void putTimeLog(char *name, double time);
void display_timeLog();
void displayLogs();
void freeLogs();

#endif