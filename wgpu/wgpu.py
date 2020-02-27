import argparse
import os
import pwd
import sys

from pynvml import nvmlInit, NVMLError, nvmlShutdown, nvmlDeviceGetCount, nvmlDeviceGetHandleByIndex, \
    nvmlDeviceGetName, nvmlDeviceGetComputeRunningProcesses, nvmlSystemGetProcessName, nvmlDeviceGetUtilizationRates, \
    nvmlDeviceGetMemoryInfo, nvmlDeviceGetFanSpeed, nvmlDeviceGetTemperature, nvmlDeviceGetPowerUsage, \
    nvmlDeviceGetEnforcedPowerLimit


class TextStyles:
    SELECTED = '\033[31m'
    CLEAR = '\033[0m'


def get_gpu_pid_info():
    """Retrieves the process IDs of processes running on the GPU."""

    gpus = []
    device_count = -1

    try:
        nvmlInit()

        device_count = nvmlDeviceGetCount()

        gpus = [{}] * device_count

        for i in range(device_count):
            gpus[i] = {'id': i}
            handle = nvmlDeviceGetHandleByIndex(i)
            device_name = nvmlDeviceGetName(handle)

            gpus[i]['name'] = device_name

            try:
                util = nvmlDeviceGetUtilizationRates(handle)
                gpus[i]['utilization'] = util.gpu
            except NVMLError as err:
                print(f'Error while reading GPU utilization for GPU {i}: {err}', file=sys.stderr)

            try:
                mem_info = nvmlDeviceGetMemoryInfo(handle)
                gpus[i]['mem_total'] = mem_info.total
                gpus[i]['mem_used'] = mem_info.used
            except NVMLError as err:
                print(f'Error while reading memory utilization for GPU {i}: {err}', file=sys.stderr)

            try:
                fan_speed = nvmlDeviceGetFanSpeed(handle)
                gpus[i]['fan_speed'] = fan_speed
            except NVMLError as err:
                print(f'Error while reading fan speed for GPU {i}: {err}', file=sys.stderr)

            try:
                temp = nvmlDeviceGetTemperature(handle, 0)
                gpus[i]['temp'] = temp
            except NVMLError as err:
                print(f'Error while reading temperature for GPU {i}: {err}', file=sys.stderr)

            try:
                power_usage = nvmlDeviceGetPowerUsage(handle)
                gpus[i]['power_usage'] = round(power_usage / 1000.)
            except NVMLError as err:
                print(f'Error while reading power usage for GPU {i}: {err}', file=sys.stderr)

            try:
                power_limit = nvmlDeviceGetEnforcedPowerLimit(handle)
                gpus[i]['power_limit'] = round(power_limit / 1000.)
            except NVMLError as err:
                print(f'Error while reading power limit for GPU {i}: {err}', file=sys.stderr)

            gpus[i]['processes'] = []

            try:
                processes = nvmlDeviceGetComputeRunningProcesses(handle)

                for process in processes:
                    process_name = nvmlSystemGetProcessName(process.pid).decode()
                    gpus[i]['processes'].append({'pid': process.pid, 'name': process_name})

            except NVMLError as err:
                print(f'Error while reading processes for GPU {i}: {err}', file=sys.stderr)

    except NVMLError as err:
        print(f'Error while reading GPU information: {err}', file=sys.stderr)

    nvmlShutdown()

    return gpus, device_count


def get_user_info(gpus):
    for gpu in gpus:
        for process in gpu['processes']:
            pid = process['pid']
            process_stat_file = os.stat("/proc/%d" % pid)
            uid = process_stat_file.st_uid
            process['user'] = pwd.getpwuid(uid)[0]

    return gpus


def print_gpu_process_info(gpus, watch_gpu, username):
    spacer = ' ' * 3
    underline = '-'

    gpu_len = [len(gpu['name']) for gpu in gpus]
    max_gpu_len = 4 if len(gpus) == 0 else max(gpu_len)

    for gpu in gpus:
        gpu['highlight'] = gpu['id'] == watch_gpu or any([process['user'] == username for process in gpu['processes']])

    print(f'{"GPU":3}{spacer}{"Name":{max_gpu_len}}{spacer}{"Util":4}{spacer}{"Memory Usage":12}{spacer}'
          f'{"Memory Total":12}{spacer}{"Fan":3}{spacer}{"Temp":4}{spacer}{"Pwr:Usage/Cap"}{spacer}{"In Use":6}')
    print(f'{underline * 3}{spacer}{underline * max_gpu_len}{spacer}{underline * 4}{spacer}{underline * 12}{spacer}'
          f'{underline * 12}{spacer}{underline * 3}{spacer}{underline * 4}{spacer}{underline * 13}{spacer}'
          f'{underline * 6}')
    for gpu in gpus:
        print(f'{TextStyles.SELECTED if gpu["highlight"] else ""}'
              f'{gpu["id"]:3}{spacer}'
              f'{gpu["name"].decode():{max_gpu_len}}{spacer}'
              f'{str(gpu["utilization"]) + "%":>4}{spacer}'
              f'{str(int(gpu["mem_used"] / 1024 / 1024)) + " MiB":>12}{spacer}'
              f'{str(int(gpu["mem_total"] / 1024 / 1024)) + " MiB":>12}{spacer}'
              f'{str(gpu["fan_speed"]) + "%":>3}{spacer}'
              f'{str(gpu["temp"]) + "C":>4}{spacer}'
              f'{str(gpu["power_usage"]) + "W":>6} / '
              f'{str(gpu["power_limit"]) + "W":>4}{spacer}'
              f'{"Yes" if len(gpu["processes"]) > 0 else "No":>6}'
              f'{TextStyles.CLEAR if gpu["highlight"] else ""}')

    print()

    userlen = [len(process['user']) for gpu in gpus for process in gpu['processes']]
    userlen.append(4)
    max_userlen = max(userlen)

    processlen = [len(process['name']) for gpu in gpus for process in gpu['processes']]
    processlen.append(7)
    max_processlen = max(processlen)

    print(f'{"User":{max_userlen}}{spacer}{"GPU":3}{spacer}{"Process"}')
    print(f'{underline * max_userlen}{spacer}{underline * 3}{spacer}{underline * max_processlen}')
    for gpu in gpus:
        for process in gpu['processes']:
            print(f'{TextStyles.SELECTED if gpu["highlight"] else ""}'
                  f'{process["user"]:{max_userlen}}{spacer}'
                  f'{gpu["id"]:3}{spacer}'
                  f'{process["name"]}'
                  f'{TextStyles.CLEAR if gpu["highlight"] else ""}')


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('--gpu', help='Select GPU ID of GPU to highlight.', type=int)
    parser.add_argument('--username', help='Select user name of user to highlight.', type=str)

    args = parser.parse_args()

    gpu_id = args.gpu
    username = args.username

    gpus, device_count = get_gpu_pid_info()
    gpus = get_user_info(gpus)
    print_gpu_process_info(gpus, gpu_id, username)


if __name__ == '__main__':
    main()
