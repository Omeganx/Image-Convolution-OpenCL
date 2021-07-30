#include "../headers/CL_program_handler.h"
#include <stdio.h>

void buildAll(ProgramList list, cl_context context, cl_device_id device)
{
    Program *program = list.head_program;
    while (program != NULL)
    {
        program->clprogram = createProgram(context, device, program->name);
        program = program->next;
    }
}

void insertProgramList(ProgramList list, char *programName)
{
    Program *program = (Program *)malloc(sizeof(*program));

    program->name = programName;

    if (list.size == 0)
    {
        program->next = NULL;
        list.head_program = program;
    }
    else
    {
        program->next = list.head_program;
        list.head_program = program;
    }
    list.size += 1;
}

void displayProgramList(ProgramList list)
{
    Program *program = list.head_program;
    while (program != NULL)
    {
        printf("Program name: %s \n", program->name);
        program = program->next;
    }
}

cl_program createProgram(cl_context context, cl_device_id device, const char *fileName)
{
    cl_int errNum;
    cl_program program;

    char *scrStr;
    FILE *fp = fopen(fileName, "r");

    if (fp == NULL)
    {
        printf("Error, Could not open kernel source file%s\n", fileName);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    int len = ftell(fp) + 1;
    rewind(fp);
    scrStr = (char *)malloc(len * sizeof(*scrStr));
    memset(scrStr, 0x0, len);
    if (!scrStr)
    {
        printf("Error,cound not allocate memory for source string\n");
    }
    fread(scrStr, sizeof(char), len, fp);

    fclose(fp);
    program = clCreateProgramWithSource(context, 1, (const char **)&scrStr, NULL, &errNum);
    checkError(errNum, "Creating program with source");
    if (program == NULL)
    {
        printf("Failed to create CL program from source.\n");
        return NULL;
    }

    errNum = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (errNum != CL_SUCCESS)
    {
        char buildLog[16384];
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
        printf("Error: %s\n", buildLog);

        clReleaseProgram(program);
        return NULL;
    }
    free(scrStr);
    checkError(errNum, "build program");

    printf("Open CL program %s build sucessfully.\n", fileName);
    return program;
}

cl_program getBuildProgram(char *programName, ProgramList *list, cl_context context, cl_device_id device)
{
    Program *program = list->head_program;
    //Get program
    while (program != NULL)
    {
        if (strcmp(programName, program->name) == 0)
        {
            if (program->clprogram == NULL)
                program->clprogram = createProgram(context, device, programName);
            return program->clprogram;
        }
        program = program->next;
    }

    //Build otherwise
    Program *prog = (Program *)malloc(sizeof(*prog));
    prog->name = programName;
    prog->clprogram = createProgram(context, device, programName);

    prog->next = list->head_program;
    list->head_program = prog;
    list->size += 1;
    return prog->clprogram;
}

void clearPrograms(ProgramList *list)
{
    Program *program = list->head_program;
    Program *next;
    while (program != NULL)
    {
        next = program->next;
        free(program->name);
        clReleaseProgram(program->clprogram);
        free(program);
        program = next;
    }
}