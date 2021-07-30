#ifndef EVENT_H
#define EVENT_H

#include <CL/cl.h>
#include "performancelogger.h"
#include "err_code.h"

typedef struct cl_event_handler event_handler;
struct cl_event_handler
{
    cl_event event;
    char *name;
    event_handler *next_event;
};


void free_events(event_handler **events);
void add_to_logs(event_handler **events);
void add_event(event_handler **events, char *name, cl_event event);
#endif

