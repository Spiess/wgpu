#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <nvml.h>

// Constants

#define PROCESS_NAME_BUFFER_SIZE 64
#define USER_NAME_BUFFER_SIZE 64

typedef struct Process {
    unsigned int pid;
    char name[PROCESS_NAME_BUFFER_SIZE];
    unsigned int uid;
    char userName[USER_NAME_BUFFER_SIZE];
} Process;

typedef struct GPU {
    unsigned int id;
    char name[NVML_DEVICE_NAME_BUFFER_SIZE];
    unsigned int numProcesses;
    struct Process *processes;
    unsigned int utilization;
    unsigned long long usedMemory;
    unsigned long long totalMemory;
} GPU;

typedef struct GPUList {
    GPU *gpus;
    unsigned int length;
} GPUList;

GPUList getGpuInfo()
{
    nvmlReturn_t result;
    unsigned int deviceCount, i;
    // Declare arrays for device information
    GPU *gpus = NULL;
    GPUList gpuList;

    // Initialize NVML library
    result = nvmlInit();
    if (NVML_SUCCESS != result)
    {
        printf("Failed to initialize NVML: %s\n", nvmlErrorString(result));
        exit(1);
    }

    result = nvmlDeviceGetCount(&deviceCount);
    if (NVML_SUCCESS != result)
    { 
        printf("Failed to query device count: %s\n", nvmlErrorString(result));
        goto Error;
    }

    // Allocate required memory for GPU structs
    gpus = (GPU *)malloc(sizeof(GPU) * deviceCount);
    gpuList.length = deviceCount;
    gpuList.gpus = gpus;

    for (i = 0; i < deviceCount; i++)
    {
        nvmlDevice_t device;
        nvmlUtilization_t utilization;
        nvmlMemory_t memory;
        unsigned int j;
        nvmlProcessInfo_t *processes = NULL;

        // Set GPU ID
        gpus[i].id = i;

        // Get device handle
        result = nvmlDeviceGetHandleByIndex(i, &device);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get handle for device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Get device name
        result = nvmlDeviceGetName(device, gpus[i].name, NVML_DEVICE_NAME_BUFFER_SIZE);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get name of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Get device utilization
        result = nvmlDeviceGetUtilizationRates(device, &utilization);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get utilization of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Set GPU utilization
        gpus[i].utilization = utilization.gpu;

        // Get device memory
        result = nvmlDeviceGetMemoryInfo(device, &memory);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get memory info of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Set GPU memory usage
        gpus[i].usedMemory = memory.used;
        gpus[i].totalMemory = memory.total;

        // Get device processes
        gpus[i].numProcesses = 0;
        gpus[i].processes = NULL;
        result = nvmlDeviceGetComputeRunningProcesses(device, &(gpus[i].numProcesses), NULL);
        if (result == NVML_ERROR_INSUFFICIENT_SIZE)
        {
            processes = (nvmlProcessInfo_t *)malloc(sizeof(nvmlProcessInfo_t) * gpus[i].numProcesses);
            result = nvmlDeviceGetComputeRunningProcesses(device, &(gpus[i].numProcesses), processes);
            if (NVML_SUCCESS != result)
            {
                printf("Failed to get processes of device %i: %s\n", i, nvmlErrorString(result));
                goto Error;
            }

            // Set GPU process info
            gpus[i].processes = (Process *)malloc(sizeof(Process) * gpus[i].numProcesses);
            for (j = 0; j < gpus[i].numProcesses; j++)
            {
                gpus[i].processes[j].pid = processes[j].pid;

                result = nvmlSystemGetProcessName(processes[j].pid, gpus[i].processes[j].name, PROCESS_NAME_BUFFER_SIZE);
                if (NVML_SUCCESS != result)
                {
                    printf("Failed to get process name of device %i process %i: %s\n", i, j, nvmlErrorString(result));
                    goto Error;
                }
            }

            // Free the allocated arrays
            free(processes);
        }
        else if (NVML_SUCCESS != result)
        { 
            printf("Failed to get number of processes of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }
    }

    // Shutdown NVML library
    result = nvmlShutdown();
    if (NVML_SUCCESS != result)
    {
        printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));
        exit(1);
    }

    return gpuList;

Error:
    result = nvmlShutdown();
    if (NVML_SUCCESS != result)
        printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));

    exit(1);
}

unsigned int uidFromPid(unsigned int pid)
{
    struct stat stats;
    char fileName[64];

    sprintf(fileName, "/proc/%d", pid);

    if (stat(fileName, &stats) == 0)
    {
        return stats.st_uid;
    }

    return -1;
}

void getUsernameFromUid(char *dest, unsigned int uid)
{
    struct passwd *pwd;

    pwd = getpwuid(uid);

    if (pwd)
    {
        strcpy(dest, pwd->pw_name);
    }
    else
    {
        strcpy(dest, "ERROR");
    }
}

void updateProcessUserInfo(GPUList gpuList)
{
    unsigned int i, j;
    for (i = 0; i < gpuList.length; i++)
    {
        for (j = 0; j < gpuList.gpus[i].numProcesses; j++)
        {
            gpuList.gpus[i].processes[j].uid = uidFromPid(gpuList.gpus[i].processes[j].pid);
            if (gpuList.gpus[i].processes[j].uid < 0)
            {
                // UID retrieval has failed!
                strcpy(gpuList.gpus[i].processes[j].userName, "ERROR");
            }
            else
            {
                getUsernameFromUid(gpuList.gpus[i].processes[j].userName, gpuList.gpus[i].processes[j].uid);
            }
        }
    }
}

void printGpuList(GPUList gpuList)
{
    unsigned int i, j;
    const char spacer[] = "   ";
    GPU *gpu;
    Process *process;

    printf("GPU%sName%sUtil%sMemory Usage%sMemory Total%sIn Use\n", spacer, spacer, spacer, spacer, spacer);
    printf("---%s----%s----%s------------%s------------%s------\n", spacer, spacer, spacer, spacer, spacer);

    for (i = 0; i < gpuList.length; i++)
    {
        gpu = &(gpuList.gpus[i]);
        printf("%d%s%s%s%d%%%s%lld MiB%s%lld MiB%s%s\n", gpu->id, spacer, gpu->name, spacer, gpu->utilization, spacer, gpu->usedMemory / 1024 / 1024, spacer, gpu->totalMemory / 1024 / 1024, spacer, gpu->numProcesses > 0 ? "Yes" : "No");
    }

    printf("\n");

    printf("User%sGPU%sProcess\n", spacer, spacer);
    printf("----%s---%s-------\n", spacer, spacer);

    for (i = 0; i < gpuList.length; i++)
    {
        gpu = &(gpuList.gpus[i]);
        for (j = 0; j < gpu->numProcesses; j++)
        {
            process = &(gpu->processes[j]);
            printf("%s%s%d%s%s\n", process->userName, spacer, gpu->id, spacer, process->name);
        }
    }
}

int main()
{
    GPUList gpuList;
    gpuList = getGpuInfo();
    updateProcessUserInfo(gpuList);
    printGpuList(gpuList);

    return 0;
}
