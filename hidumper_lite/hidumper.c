#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "securec.h"

#define HIDUMPER_DEVICE  "/dev/hidumper"
#define USER_FAULT_ADDR  0x3
#define USER_FAULT_VALUE 0x4
#define PATH_MAX_LEN     256

enum MemDumpType {
    DUMP_TO_STDOUT,
    DUMP_REGION_TO_STDOUT,
    DUMP_TO_FILE,
    DUMP_REGION_TO_FILE
};

struct MemDumpParam {
    enum MemDumpType type;
    unsigned long long start;
    unsigned long long size;
    char filePath[PATH_MAX_LEN];
};

#define HIDUMPER_IOC_BASE            'd'
#define HIDUMPER_DUMP_ALL            _IO(HIDUMPER_IOC_BASE, 1)
#define HIDUMPER_CPU_USAGE           _IO(HIDUMPER_IOC_BASE, 2)
#define HIDUMPER_MEM_USAGE           _IO(HIDUMPER_IOC_BASE, 3)
#define HIDUMPER_TASK_INFO           _IO(HIDUMPER_IOC_BASE, 4)
#define HIDUMPER_INJECT_KERNEL_CRASH _IO(HIDUMPER_IOC_BASE, 5)
#define HIDUMPER_DUMP_FAULT_LOG      _IO(HIDUMPER_IOC_BASE, 6)
#define HIDUMPER_MEM_DATA            _IOW(HIDUMPER_IOC_BASE, 7, struct MemDumpParam)

static void Usage(void)
{
    printf("Usage:\n"
        "    hidumper                              dump cpu usage, memory usage and all tasks.\n"
        "or:\n"
        "    hidumper -dc                          dump the cpu usage.\n"
        "    hidumper -df                          dump the fault logs.\n"
        "    hidumper -dm                          dump the memory usage.\n"
        "    hidumper -dt                          dump all the tasks.\n"
        "    hidumper -h                           help text for the tool.\n"
        "    hidumper -ikc                         inject kernel crash for debug version.\n"
        "    hidumper -iuc                         inject user crash for debug version.\n");
    printf("    hidumper -m                           dump memory to stdout in hex format for debug version.\n"
        "    hidumper -m filepath                  dump memory to filepath in the device in hex format for "
        "debug version.\n"
        "    hidumper -m memstart memsize          dump memory with starting address memstart(hex) and\n"
        "                                          size memsize(hex) to stdout in hex format for debug version.\n"
        "    hidumper -m memstart memsize filepath dump memory with starting address memstart(hex) and\n"
        "                                          size memsize(hex) to filepath in hex format for debug version.\n");
}

static void ExecAction(int fd, unsigned int cmd)
{
    if (fd < 0) {
        printf("Invalid fd: %d\n", fd);
        return;
    }

    int ret = ioctl(fd, cmd, NULL);
    if (ret < 0) {
        printf("Failed to ioctl [%s], error [%s]\n", cmd, strerror(errno));
    }
}

static void DumpALLInfo(int fd)
{
    ExecAction(fd, HIDUMPER_DUMP_ALL);
}

static void DumpCpuUsage(int fd)
{
    ExecAction(fd, HIDUMPER_CPU_USAGE);
}

static void DumpMemUsage(int fd)
{
    ExecAction(fd, HIDUMPER_MEM_USAGE);
}

static void DumpTaskInfo(int fd)
{
    ExecAction(fd, HIDUMPER_TASK_INFO);
}

static void DumpMemData(int fd, struct MemDumpParam *param)
{
    int ret = ioctl(fd, HIDUMPER_MEM_DATA, param);
    if (ret < 0) {
        printf("Failed to ioctl [%s], error [%s]\n", HIDUMPER_DEVICE, strerror(errno));
    }
}

static void InjectKernelCrash(int fd)
{
    ExecAction(fd, HIDUMPER_INJECT_KERNEL_CRASH);
}

static void DumpFaultLog(int fd)
{
    ExecAction(fd, HIDUMPER_DUMP_FAULT_LOG);
}

static void InjectUserCrash(void)
{
    *((int *)USER_FAULT_ADDR) = USER_FAULT_VALUE;
}

int main(int argc, char *argv[])
{
    int fd;
    struct MemDumpParam param;
    fd = open(HIDUMPER_DEVICE, O_RDONLY);
    if (fd < 0) {
        printf("Failed to open [%s], error [%s]\n", HIDUMPER_DEVICE, strerror(errno));
        return -1;
    }

    if (argc == 1) {
        DumpALLInfo(fd);
    } else if (argc == 2 && strcmp(argv[1], "-dc") == 0) {
        DumpCpuUsage(fd);
    } else if (argc == 2 && strcmp(argv[1], "-df") == 0) {
        DumpFaultLog(fd);
    } else if (argc == 2 && strcmp(argv[1], "-dm") == 0) {
        DumpMemUsage(fd);
    } else if (argc == 2 && strcmp(argv[1], "-dt") == 0) {
        DumpTaskInfo(fd);
    } else if (argc == 2 && strcmp(argv[1], "-ikc") == 0) {
        InjectKernelCrash(fd);
    } else if (argc == 2 && strcmp(argv[1], "-iuc") == 0) {
        InjectUserCrash();
    }  else if (argc == 2 && strcmp(argv[1], "-m") == 0) {
        param.type = DUMP_TO_STDOUT;
        DumpMemData(fd, &param);
    } else if (argc == 3 && strcmp(argv[1], "-m") == 0) {
        param.type = DUMP_TO_FILE;
        (void)strncpy_s(param.filePath, sizeof(param.filePath), argv[2], sizeof(param.filePath) - 1);
        DumpMemData(fd, &param);
    } else if (argc == 4 && strcmp(argv[1], "-m") == 0) {
        param.type = DUMP_TO_STDOUT;
        param.start = strtoull(argv[2], NULL, 16);
        param.size = strtoull(argv[3], NULL, 16);
        DumpMemData(fd, &param);
    }  else if (argc == 5 && strcmp(argv[1], "-m") == 0) {
        param.type = DUMP_TO_FILE;
        param.start = strtoull(argv[2], NULL, 16);
        param.size = strtoull(argv[3], NULL, 16);
        (void)strncpy_s(param.filePath, sizeof(param.filePath), argv[4], sizeof(param.filePath) - 1);
        DumpMemData(fd, &param);
    } else {
        Usage();
    }
    close(fd);

    return 0;
}