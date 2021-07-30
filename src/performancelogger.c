#include "../headers/performancelogger.h"
#include <math.h>

static timeLogList_t *timeLogsVar = NULL;

void putTolog(char *name, double time, timeLogList_t **timeLogs)
{
    if (*timeLogs == NULL)
    {
        timeLogList_t *temp = (timeLogList_t *)malloc(sizeof(timeLogList_t));
        *timeLogs = temp;
        temp->head = NULL;
        put_log(name, time, &temp->head, 0);
    }
    else
    {
        timeLogList_t *temp = *timeLogs;
        put_log(name, time, &temp->head, 0);
    }
}

void put_log(char *name, double time, timeLog_t **timeLog, int depth)
{
    timeLog_t *current = *timeLog;
    timeLog_t *old = current;

    if (current == NULL)
    {
        current = (timeLog_t *)malloc(sizeof(timeLog_t));
        current->functionName = name;
        current->totalTime_ms = time;
        current->depth = depth;
        current->childrenEntry = NULL;
        current->nextTimeLogEntry = NULL;
        current->isActive = 0;
        *timeLog = current;
        return;
    }

    while (current != NULL && current->isActive == 0)
    {
        old = current;

        if (strcmp(current->functionName, name) == 0 && current->isActive == 1)
            printf("Error, the process has alread started.\n");

        if (strcmp(current->functionName, name) == 0)
        {

            current->totalTime_ms += time;
            return;
        }

        current = current->nextTimeLogEntry;
    }

    if (current != NULL && current->isActive == 1)
    {
        put_log(name, time, &current->childrenEntry, depth + 1);
        return;
    }

    put_log(name, time, &old->nextTimeLogEntry, depth);
}

void start_log(char *name, timeLogList_t **timeLogs)
{
    if (*timeLogs == NULL)
    {
        timeLogList_t *temp = (timeLogList_t *)malloc(sizeof(timeLogList_t));
        *timeLogs = temp;
        temp->head = NULL;
        add_log(name, &temp->head, 0);
    }
    else
    {
        timeLogList_t *temp = *timeLogs;

        add_log(name, &temp->head, 0);
    }
}

void add_log(char *name, timeLog_t **timeLog, int depth)
{
    timeLog_t *current = *timeLog;
    timeLog_t *old = current;
    if (current == NULL)
    {
        current = (timeLog_t *)malloc(sizeof(timeLog_t));
        current->functionName = name;
        current->timeStart = clock();
        current->isActive = 1;
        current->totalTime_ms = 0;
        current->depth = depth;
        current->childrenEntry = NULL;
        current->nextTimeLogEntry = NULL;
        *timeLog = current;
        return;
    }
    while (current != NULL && current->isActive == 0)
    {
        old = current;

        if (strcmp(current->functionName, name) == 0 && current->isActive == 1)
            printf("Error, the process has alread started.\n");

        if (strcmp(current->functionName, name) == 0)
        {

            current->isActive = 1;
            current->timeStart = clock();
            return;
        }

        current = current->nextTimeLogEntry;
    }

    if (current != NULL && current->isActive == 1)
    {
        add_log(name, &current->childrenEntry, depth + 1);
        return;
    }

    add_log(name, &old->nextTimeLogEntry, depth);
}

void stop_log(char *name, timeLogList_t **timeLogs)
{
    timeLogList_t *temp = *timeLogs;
    end_log(name, &temp->head);
}

void end_log(char *name, timeLog_t **timeLog)
{
    timeLog_t *current = *timeLog;
    while (current != NULL && current->isActive == 0)
    {
        current = current->nextTimeLogEntry;
    }

    if (current != NULL && strcmp(name, current->functionName) == 0)
    {
        clock_t timeStop = clock();
        current->totalTime_ms += 1000 * (((double)(timeStop - current->timeStart)) / CLOCKS_PER_SEC);
        current->isActive = 0;
    }
    else if (current != NULL && current->isActive == 1)
    {
        end_log(name, &current->childrenEntry);
    }
    else
    {
        printf("Error timelog entry '%s'not found.\n", name);
    }
}

void freelog(timeLogList_t **timeLogs)
{
    timeLogList_t *temp = *timeLogs;
    freelog_el(&temp->head);
    free(*timeLogs);
}

void freelog_el(timeLog_t **timeLog)
{
    timeLog_t *current = *timeLog;
    timeLog_t *old = current;
    while (current != NULL)
    {

        if (current->childrenEntry != NULL)
        {
            freelog_el(&current->childrenEntry);
        }
        old = current;
        current = current->nextTimeLogEntry;
        free(old);
    }
}

void display_timeLog(timeLogList_t **timeLogs)
{
    timeLogList_t *temp = *timeLogs;
    display_log(&temp->head);
}

void display_log(timeLog_t **timeLog)
{
    timeLog_t *current = *timeLog;
    //printf("curent name = %s", current->functionName);
    while (current != NULL)
    {

        if (current->depth > 0)
        {
            char tab[current->depth * 2 + 1];
            for (int i = 0; i < 2 * current->depth; i++)
                tab[i] = ' ';
            tab[current->depth * 2] = '\0';
            printf("%s| '%s' process time = %.1f ms.\n", tab, current->functionName, current->totalTime_ms);
        }
        else

            printf("| '%s' process time = %.1f ms.\n", current->functionName, current->totalTime_ms);

        if (current->childrenEntry != NULL)
        {
            display_log(&current->childrenEntry);
        }
        current = current->nextTimeLogEntry;
    }
}

void putTimeLog(char *name, double time)
{
    putTolog(name, time, &timeLogsVar);
}

void startTimeLog(char *name)
{
    start_log(name, &timeLogsVar);
}

void stopTimeLog(char *name)
{
    stop_log(name, &timeLogsVar);
}

void displayLogs()
{
    display_timeLog(&timeLogsVar);
}

void freeLogs()
{
    freelog(&timeLogsVar);
}

/*
int main()
{
    start("Hey This is a test0", &timeLogs);
    start("Hey This is a test1", &timeLogs);
    start("Hey This is a test2", &timeLogs);
    int x = 0;
    for(int i = 0; i<10000; i++)
    {
        x += i;
    }
    stop("Hey This is a test2", &timeLogs);
    display_timeLog(&timeLogs);
    return 0;
}
*/