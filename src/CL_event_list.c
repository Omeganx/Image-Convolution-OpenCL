#include "../headers/CL_event_list.h"

void add_event(event_handler **events, char *name, cl_event event)
{   
    event_handler *temp_event = (event_handler *)malloc(sizeof(event_handler));
    temp_event->event = event;
    temp_event->name = name;
    temp_event->next_event = NULL;

    if(*events == NULL) 
    {   
        *events = temp_event;
    }
    else
    {
        /*event_handler *current = *events;
        *events = temp_event;
        temp_event->next_event = current;*/
        
        event_handler *current = *events;
        event_handler *old = current;
        while(current != NULL)
        {
            old = current;
            current = current->next_event;
        }
        old->next_event = temp_event;
    }
}

void add_to_logs(event_handler **events)
{   
    int error;
    event_handler *current = *events;
    cl_event event;
    cl_ulong start = 0, end = 0;
    double totalTime;
    while(current != NULL)
    {
        event = current->event;
        error = clWaitForEvents(1, &event);
        checkError(error, "Wating for events");
        error = clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
        error |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
        checkError(error, "Getting event profiling info");
        totalTime = (cl_double)(end - start) * (cl_double)(1e-06);
        putTimeLog(current->name, totalTime);

        current = current->next_event;
    }
}

void free_events(event_handler **events)
{
    event_handler *current = *events;
    event_handler *old = *events;

    while(current != NULL)
    {   old = current;
        clReleaseEvent(current->event);
        current = current->next_event;
        free(old);
    }
}