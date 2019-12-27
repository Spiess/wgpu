#include <stdio.h>
#include <stdlib.h>
#include <nvml.h>

// Constants

#define PROCESS_NAME_BUFFER_SIZE 64

void getGpuInfo()
{
    nvmlReturn_t result;
    unsigned int device_count, i;
    // Declare arrays for device information
    // TODO

    // Initialize NVML library
    result = nvmlInit();
    if (NVML_SUCCESS != result)
    {
        printf("Failed to initialize NVML: %s\n", nvmlErrorString(result));
        exit(1);
    }

    result = nvmlDeviceGetCount(&device_count);
    if (NVML_SUCCESS != result)
    { 
        printf("Failed to query device count: %s\n", nvmlErrorString(result));
        goto Error;
    }
    printf("Found %d device%s\n", device_count, device_count != 1 ? "s" : "");

    for (i = 0; i < device_count; i++)
    {
        nvmlDevice_t device;
        char name[NVML_DEVICE_NAME_BUFFER_SIZE];
        nvmlUtilization_t utilization;
        nvmlMemory_t memory;
        unsigned long long usedMemory, totalMemory;
        unsigned int numProcesses, j;
        nvmlProcessInfo_t *processes;

        // Get device handle
        result = nvmlDeviceGetHandleByIndex(i, &device);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get handle for device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Get device name
        result = nvmlDeviceGetName(device, name, NVML_DEVICE_NAME_BUFFER_SIZE);
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

        // Get device memory
        result = nvmlDeviceGetMemoryInfo(device, &memory);
        if (NVML_SUCCESS != result)
        { 
            printf("Failed to get memory info of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        // Get device processes
        numProcesses = 0;
        result = nvmlDeviceGetComputeRunningProcesses(device, &numProcesses, NULL);
        if (result == NVML_ERROR_INSUFFICIENT_SIZE)
        {
            processes = (nvmlProcessInfo_t *)malloc(sizeof(nvmlProcessInfo_t) * numProcesses);
            nvmlDeviceGetComputeRunningProcesses(device, &numProcesses, processes);
        }
        else if (NVML_SUCCESS != result)
        { 
            printf("Failed to get processes of device %i: %s\n", i, nvmlErrorString(result));
            goto Error;
        }

        usedMemory = memory.used / 1024 / 1024;
        totalMemory = memory.total / 1024 / 1024;

        printf("%d: %s (%d%%) [%lld MiB / %lld MiB] - %d processes\n", i, name, utilization.gpu, usedMemory, totalMemory, numProcesses);
        for (j = 0; j < numProcesses; j++)
        {
            char process_name[PROCESS_NAME_BUFFER_SIZE];
            result = nvmlSystemGetProcessName(processes[j].pid, process_name, PROCESS_NAME_BUFFER_SIZE);
            if (NVML_SUCCESS != result)
            { 
                printf("Failed to get process name of device %i process %i: %s\n", i, j, nvmlErrorString(result));
                goto Error;
            }
            printf("Process %d (PID: %d) has name: %s\n", j, processes[j].pid, process_name);
        }

        // Free the allocated arrays
        free(processes);
    }

    // Shutdown NVML library
    result = nvmlShutdown();
    if (NVML_SUCCESS != result)
    {
        printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));
        exit(1);
    }

    return;

Error:
    result = nvmlShutdown();
    if (NVML_SUCCESS != result)
        printf("Failed to shutdown NVML: %s\n", nvmlErrorString(result));

    exit(1);
}

int main()
{
    getGpuInfo();
    return 0;
}
