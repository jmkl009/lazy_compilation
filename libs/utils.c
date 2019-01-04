//
// Credit to gaffe32
//

#ifndef HOT_SWAP_UTILS_H
#define HOT_SWAP_UTILS_H

#include "utils.h"

char *get_filename_from_path(char *filePath) {
  char *currPtr = filePath;
  while (*filePath != '\0') {
    if (*filePath == '/') {
      currPtr = filePath + 1;
    }
    filePath ++;
  }
  return currPtr;
}

pid_t findProcessByName(const char* processName)
{
    if(processName == NULL) {
        return -1;
    }

    struct dirent *procDirs;

    DIR *directory = opendir("/proc/");

    if (directory) {
        while ((procDirs = readdir(directory)) != NULL) {
            if (procDirs->d_type != DT_DIR)
                continue;

            pid_t pid = atoi(procDirs->d_name);

            int exePathLen = 10 + strlen(procDirs->d_name) + 1;
            char* exePath = (char *)malloc(exePathLen * sizeof(char));

            if(exePath == NULL) {
                continue;
            }

            sprintf(exePath, "/proc/%s/exe", procDirs->d_name);
            exePath[exePathLen-1] = '\0';

            char* exeBuf = (char *)malloc(PATH_MAX * sizeof(char));
            if(exeBuf == NULL) {
                free(exePath);
                continue;
            }
            ssize_t len = readlink(exePath, exeBuf, PATH_MAX - 1);

            if(len == -1) {
                free(exePath);
                free(exeBuf);
                continue;
            }

            exeBuf[len] = '\0';

            char* exeName = NULL;
            char* exeToken = strtok(exeBuf, "/");
            while(exeToken) {
                exeName = exeToken;
                exeToken = strtok(NULL, "/");
            }

            if(strcmp(exeName, processName) == 0) {
                free(exePath);
//                if (exeNameBuf) {
//                    *exeNameBuf = exeBuf;
//                } else {
                    free(exeBuf);
//                }

                closedir(directory);
                return pid;
            }

            free(exePath);
            free(exeBuf);
        }

        closedir(directory);
    }

    return -1;
}

long freespaceaddr(pid_t pid)
{
    FILE *fp;
    char filename[30];
    char line[850];
    long addr;
    char perms[5];
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        exit(1);
    while(fgets(line, 850, fp) != NULL)
    {
        sscanf(line, "%lx-%*lx %s %*s %*s %*d", &addr, perms);

        if(strstr(perms, "x") != NULL)
        {
            break;
        }
    }
    fclose(fp);
    return addr;
}

long getlibcaddr(pid_t pid)
{
    FILE *fp;
    char filename[30];
    char line[850];
    long addr;
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        exit(1);
    while(fgets(line, 850, fp) != NULL)
    {
        sscanf(line, "%lx-%*lx %*s %*s %*s %*d", &addr);
        if(strstr(line, "libc-") != NULL)
        {
            break;
        }
    }
    fclose(fp);
    return addr;
}

void * checkloaded(pid_t pid, char* libname, char *symbol)
{
    FILE *fp;
    char filename[30];
    char line[850];
    long section_start_addr;
    sprintf(filename, "/proc/%d/maps", pid);
    fp = fopen(filename, "r");
    if(fp == NULL)
        exit(1);
    while(fgets(line, 850, fp) != NULL) {
        sscanf(line, "%lx-%*lx %*s %*s %*s %*d", &section_start_addr);
        if(strstr(line, libname) != NULL) {
            fclose(fp);
            exe bin(libname);
            symaddr_t symaddr = bin.bin_dlsym(symbol);
            return section_start_addr + symaddr.addr;
        }
    }
    fclose(fp);
    return NULL;
}

long getFunctionAddress(char* funcName)
{
    void* self = dlopen("libc.so.6", RTLD_LAZY);
    void* funcAddr = dlsym(self, funcName);
    return (long)funcAddr;
}

unsigned char* findRet(void* endAddr)
{
    unsigned char* retInstAddr = (unsigned char *)endAddr;
    while(*retInstAddr != INTEL_RET_INSTRUCTION)
    {
        retInstAddr--;
    }
    return retInstAddr;
}

void usage(char* name)
{
    printf("usage: %s [-n process-name] [-p pid] [-f source-file-name] \n", name);
}

void *get_base_addr(pid_t pid) {
    char filename[32];
    sprintf(filename, "/proc/%d/maps", pid);
    FILE * mem_map = fopen(filename, "r");
    if (!mem_map) {
        perror("Open Memory Map:");
        exit(-1);
    }

    void *base = NULL;
    fscanf(mem_map, "%p", &base);
    fclose(mem_map);
    return base;
}

void show_stack_trace(pid_t pid) {
    unw_addr_space_t as = unw_create_addr_space(&_UPT_accessors, 0);
    void *context = _UPT_create(pid);
    unw_cursor_t cursor;
    unw_cursor_t cursor_copy;
    if (unw_init_remote(&cursor, as, context) != 0) {
        printf("ERROR: cannot initialize cursor for remote unwinding\n");
        exit(1);
    }
    cursor_copy = cursor;

    char func[1024];
    unw_get_proc_name(&cursor, func, sizeof(func), NULL);
    do {
        unw_word_t offset, pc;
        char sym[1024];
        if (unw_get_reg(&cursor, UNW_REG_IP, &pc)) {
            printf("ERROR: cannot read program counter\n");
            exit(1);
        }

        printf("0x%lx: ", pc);

        if (unw_get_proc_name(&cursor, sym, sizeof(sym), &offset) == 0)
            printf("(%s+0x%lx)\n", sym, offset);
        else
            printf("-- no symbol name found\n");
    } while (unw_step(&cursor) > 0);
//    unw_step(&cursor_copy);
//    unw_step(&cursor_copy);
//    unw_step(&cursor_copy);
//    unw_resume(&cursor_copy);
    _UPT_destroy(context);
}


#endif
