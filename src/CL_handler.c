#include "../headers/CL_handler.h"

#include <time.h>


void cl_handler_init(cl_handler *handler, int platform_id)
{
    startTimeLog("openCL init");
    cl_int error;
    cl_uint numPlatforms;
    cl_uint numDevices;

    handler->programs.size = 0;
    handler->programs.head_program = NULL;
    handler->events = NULL;
    cl_platform_id *platformIds;
    cl_device_id *devices;
    error = clGetPlatformIDs(0, NULL, &numPlatforms);
    platformIds = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatforms);

    error |= clGetPlatformIDs(numPlatforms, platformIds, NULL);
    checkError(error, "Getting platforms.");

    handler->platform = platformIds[platform_id];

    error = clGetDeviceIDs(handler->platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
    devices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevices);

    error |= clGetDeviceIDs(handler->platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
    checkError(error, "Getting devices");

    handler->device = devices[0];
    handler->context = clCreateContext(NULL, 1, &handler->device, NULL, NULL, &error);
    checkError(error, "Getting devices");

    handler->command_queue = clCreateCommandQueue(handler->context, handler->device, CL_QUEUE_PROFILING_ENABLE, &error);
    checkError(error, "Creating command queue");
    stopTimeLog("openCL init");
}

void cl_handler_info(cl_handler *handler)
{
    cl_int error;
    size_t size;
    char *name;

    error = clGetPlatformInfo(handler->platform, CL_PLATFORM_NAME, 0, NULL, &size);
    name = (char *)malloc(sizeof(char) * size);
    error |= clGetPlatformInfo(handler->platform, CL_PLATFORM_NAME, size, name, NULL);
    checkError(error, "Getting platform info");

    printf("Open CL Platform Name : %s.\n", name);
    free(name);

    error = clGetDeviceInfo(handler->device, CL_DEVICE_NAME, 0, NULL, &size);
    name = (char *)malloc(sizeof(char) * size);
    error |= clGetDeviceInfo(handler->device, CL_DEVICE_NAME, size, name, NULL);
    checkError(error, "Getting device name");

    printf("Open CL Device Name : %s.\n", name);
    free(name);
}

void cl_handler_clear(cl_handler *handler)
{
    clReleaseCommandQueue(handler->command_queue);
    clReleaseDevice(handler->device);
    clReleaseContext(handler->context);
    clearPrograms(&handler->programs);
    free_events(&handler->events);
}

void addEventProfiling(cl_handler *handler)
{
    add_to_logs(&handler->events);
}

void cl_display_programs(cl_handler *handler)
{
    displayProgramList(handler->programs);
}

kernel_size getKernelSize(int work_dim, size_t *global_work_offset, size_t *global_work_size, size_t *local_work_size)
{
    kernel_size kernel_size;
    kernel_size.work_dim = work_dim;
    kernel_size.global_work_offset = global_work_offset;
    kernel_size.global_work_size = global_work_size;
    kernel_size.local_work_size = local_work_size;

    return kernel_size;
}

void queueKernel(cl_handler *handler, char *pName, char *kName, kernel_size kernelSize, int numArgs, ...)
{
    int error;

    cl_double totalTime;
    cl_event perf_event = clCreateUserEvent(handler->context, &error);
    checkError(error, "Event Creation");
    cl_ulong start = 0, end = 0;

    char *programName = (char *)malloc(sizeof(char) * (strlen(pName) + 1));
    char *kernelName = (char *)malloc(sizeof(char) * (strlen(kName) + 1));

    strncpy(programName, pName, strlen(pName) + 1);
    strncpy(kernelName, kName, strlen(kName) + 1);

    cl_program program = getProgram(handler, programName);
    cl_kernel kernel = clCreateKernel(program, kernelName, &error);
    checkError(error, "Making kernel");

    va_list ap;

    void *memoryObject;
    size_t sizeObject;
    va_start(ap, 2 * numArgs);

    error = 0;
    for (int i = 0; i < numArgs; i++)
    {
        memoryObject = va_arg(ap, void *);
        sizeObject = va_arg(ap, size_t);
        error = clSetKernelArg(kernel, i, sizeObject, memoryObject);

        checkError(error, "Setting kernel arguments");
    }
    va_end(ap);
    error = clEnqueueNDRangeKernel(handler->command_queue, kernel, kernelSize.work_dim, kernelSize.global_work_offset, kernelSize.global_work_size, kernelSize.local_work_size, 0, NULL, &perf_event);
    checkError(error, "Launching Kernel");
    add_event(&handler->events, kName, perf_event);
}

void launchKernel(cl_handler *handler, char *pName, char *kName, kernel_size kernelSize, int numArgs, ...)
{
    startTimeLog(kName);
    int error;

    cl_double totalTime;
    cl_event perf_event = clCreateUserEvent(handler->context, &error);
    checkError(error, "Event Creation");
    cl_ulong start = 0, end = 0;

    char *programName = (char *)malloc(sizeof(char) * (strlen(pName) + 1));
    char *kernelName = (char *)malloc(sizeof(char) * (strlen(kName) + 1));

    strncpy(programName, pName, strlen(pName) + 1);
    strncpy(kernelName, kName, strlen(kName) + 1);

    cl_program program = getProgram(handler, programName);
    cl_kernel kernel = clCreateKernel(program, kernelName, &error);
    checkError(error, "Making kernel");

    va_list ap;

    void *memoryObject;
    size_t sizeObject;
    va_start(ap, 2 * numArgs);

    error = 0;
    for (int i = 0; i < numArgs; i++)
    {
        memoryObject = va_arg(ap, void *);
        sizeObject = va_arg(ap, size_t);
        error = clSetKernelArg(kernel, i, sizeObject, memoryObject);

        checkError(error, "Setting kernel arguments");
    }
    va_end(ap);
    error = clEnqueueNDRangeKernel(handler->command_queue, kernel, kernelSize.work_dim, kernelSize.global_work_offset, kernelSize.global_work_size, kernelSize.local_work_size, 0, NULL, &perf_event);
    checkError(error, "Launching Kernel");

    clFinish(handler->command_queue);

    error = clWaitForEvents(1, &perf_event);
    checkError(error, "Wating for events");

    error = clGetEventProfilingInfo(perf_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    error |= clGetEventProfilingInfo(perf_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    checkError(error, "Getting event profiling info");
    totalTime = (cl_double)(end - start) * (cl_double)(1e-06);
    clReleaseEvent(perf_event);
    //printf("%s : %.2f ms execution time.\n", kName, totalTime);
    putTimeLog("kernel", totalTime);
    stopTimeLog(kName);
}

cl_mem CL_CreateBuffer(cl_context context, cl_mem_flags flags, size_t size)
{
    int error;
    cl_mem buffer = clCreateBuffer(context, flags, size, NULL, &error);
    checkError(error, "Creating buffer");
    return buffer;
}

cl_mem CL_CreateBufferFrom(cl_context context, cl_mem_flags flags, size_t size, void *ptr)
{
    startTimeLog("write cl_mem");
    int error;
    cl_mem buffer = clCreateBuffer(context, flags | CL_MEM_COPY_HOST_PTR, size, ptr, &error);
    checkError(error, "Creating buffer");

    stopTimeLog("write cl_mem");
    return buffer;
}

void CL_ReadBufferTo(cl_handler *handler, cl_mem membuffer, size_t size, void *host_ptr)
{
    int error;
    error = clEnqueueReadBuffer(handler->command_queue, membuffer, CL_TRUE, 0, size, host_ptr, 0, NULL, NULL);
    checkError(error, "Reading buffer");
}

cl_mem CL_Create2DImage(cl_context context, cl_mem_flags flags, int width, int height, cl_channel_order cl_channel_order, cl_channel_type cl_channel_type)
{
    startTimeLog("write cl_mem");
    int error;
    cl_image_format image_format;
    image_format.image_channel_order = cl_channel_order;
    image_format.image_channel_data_type = cl_channel_type;
    cl_image_desc image_desc;
    memset(&image_desc, '\0', sizeof(cl_image_desc));

    image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    image_desc.image_width = width;
    image_desc.image_height = height;
    image_desc.buffer = NULL;

    cl_mem cl_image = clCreateImage(context, CL_MEM_READ_WRITE, &image_format, &image_desc, NULL, &error);

    stopTimeLog("write cl_mem");
    checkError(error, "Create input image\n");

    return cl_image;
}

cl_mem CL_Create2DImageFrom(cl_context context, cl_mem_flags flags, int width, int height, cl_channel_order cl_channel_order, cl_channel_type cl_channel_type, void *host_ptr)
{
    int error;
    cl_image_format image_format;
    image_format.image_channel_order = cl_channel_order;
    image_format.image_channel_data_type = cl_channel_type;
    cl_image_desc image_desc;
    memset(&image_desc, '\0', sizeof(cl_image_desc));

    image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    image_desc.image_width = width;
    image_desc.image_height = height;
    image_desc.buffer = NULL;

    cl_mem cl_image = clCreateImage(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, &image_format, &image_desc, host_ptr, &error);
    checkError(error, "Create input image");
    return cl_image;
}

void CL_Write2DImage(cl_command_queue cl_command_queue, cl_mem image2D, int width, int height, void *host_ptr)
{
    int error;
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, 1};

    cl_event write_event;
    error = clEnqueueWriteImage(cl_command_queue, image2D, CL_TRUE, origin, region, 0, 0, host_ptr, 0, NULL, &write_event);
    checkError(error, "Enqueue Write Image2D");

    cl_ulong start = 0, end = 0;
    error = clGetEventProfilingInfo(write_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    error |= clGetEventProfilingInfo(write_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    cl_double totalTime = (cl_double)(end - start) * (cl_double)(1e-06);
    clReleaseEvent(write_event);
    //printf("Write Time : %.2f ms", totalTime);
    putTimeLog("write cl_mem", totalTime);
}

void CL_Read2DImage(cl_command_queue cl_command_queue, cl_mem image2D, int width, int height, void *host_ptr)
{
    int error;
    size_t origin[3] = {0, 0, 0};
    size_t region[3] = {width, height, 1};

    cl_event read_event;
    error = clEnqueueReadImage(cl_command_queue, image2D, CL_TRUE, origin, region, 0, 0, host_ptr, 0, NULL, &read_event);
    checkError(error, "Enqueue Write Image2D");

    cl_ulong start = 0, end = 0;
    error = clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL);
    error |= clGetEventProfilingInfo(read_event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL);
    cl_double totalTime = (cl_double)(end - start) * (cl_double)(1e-06);
    clReleaseEvent(read_event);
    putTimeLog("read cl_mem", totalTime);
    //printf("Read Time : %.2f ms\n", totalTime);
}

void buildPrograms(cl_handler *handler)
{
    buildAll(handler->programs, handler->context, handler->device);
}

cl_program getProgram(cl_handler *handler, char *programName)
{
    return getBuildProgram(programName, &handler->programs, handler->context, handler->device);
}