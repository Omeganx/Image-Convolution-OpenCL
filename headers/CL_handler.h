#ifndef CL_HANDLER_H
#define CL_HANDLER_H

#include "err_code.h"
#include <CL/cl.h>
#include <string.h>
#include <stdarg.h>
#include "CL_program_handler.h"
#include "performancelogger.h"
#include "CL_event_list.h"

typedef struct _cl_handler cl_handler;
struct _cl_handler
{
    cl_platform_id platform;
    cl_context context;
    cl_device_id device;

    cl_command_queue command_queue;
    ProgramList programs;

    event_handler *events;

};

typedef struct _kernel_size kernel_size;
struct _kernel_size
{
    cl_uint work_dim;
    size_t *global_work_offset;
    size_t *global_work_size;
    size_t *local_work_size;
};

void cl_handler_init(cl_handler *handler, int platform_id);
void cl_handler_info(cl_handler *handler);
void cl_handler_clear(cl_handler *handler);
void cl_display_programs(cl_handler *handler);

void addEventProfiling(cl_handler *handler); 
kernel_size getKernelSize(int work_dim, size_t *global_work_offset, size_t *global_work_size, size_t *local_work_size);
void queueKernel(cl_handler *handler, char *pName, char *kName, kernel_size kernelSize, int numArgs, ...);
void launchKernel(cl_handler *handler, char *pName, char *kName, kernel_size kernelSize, int numArgs, ...);

cl_mem CL_CreateBufferFrom(cl_context context, cl_mem_flags flags, size_t size, void *ptr);
cl_mem CL_CreateBuffer(cl_context context, cl_mem_flags flags, size_t size);

cl_mem CL_Create2DImage(cl_context context, cl_mem_flags flags, int width, int height, cl_channel_order cl_channel_order, cl_channel_type cl_channel_type);
cl_mem CL_Create2DImageFrom(cl_context context, cl_mem_flags flags, int width, int height, cl_channel_order cl_channel_order, cl_channel_type cl_channel_type, void *host_ptr);
void CL_Write2DImage(cl_command_queue cl_command_queue, cl_mem image2D, int width, int height, void *host_ptr);
void CL_Read2DImage(cl_command_queue cl_command_queue, cl_mem image2D, int width, int height, void *host_ptr);
void CL_ReadBufferTo(cl_handler *handler, cl_mem membuffer, size_t size, void *host_ptr);

void buildPrograms(cl_handler *handler);
cl_program getProgram(cl_handler *handler, char *programName);


const char *channel_data(cl_channel_type type);
const char *channel_order(cl_channel_order order);
void printFormat();

#endif