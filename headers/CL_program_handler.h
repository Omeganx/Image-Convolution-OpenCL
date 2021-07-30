#ifndef PROGRAM_HANDLER_H
#define PROGRAM_HANDLER_H
#include <CL/cl.h>
#include "err_code.h"
#include <string.h>
typedef struct Program Program;
struct Program
{
    char *name;
    cl_program clprogram;

    Program *next;
};

typedef struct ProgramList ProgramList;
struct ProgramList
{

    int size;
    Program *head_program;
};

void buildAll(ProgramList list, cl_context context, cl_device_id device);
void insertProgramList(ProgramList list, char *programName);
void displayProgramList(ProgramList list);
cl_program createProgram(cl_context context, cl_device_id device, const char *fileName);
cl_program getBuildProgram(char *programName, ProgramList *list, cl_context context, cl_device_id device);
void clearPrograms(ProgramList *list);

#endif