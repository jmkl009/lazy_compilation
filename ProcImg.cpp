//
// Created by WangJingjin on 2018-12-30.
//

#include "ProcImg.h"



ProcImg::ProcImg(pid_t pid) : libAddrs(), bin((unsigned)pid) {
    char filename[32];
    int bytes_printed = sprintf(filename, "/proc/%d/maps", pid);
    memMapFile = fopen(filename, "r");
    if (!memMapFile) {
        perror("process memory map open error");
        exit(1); //Just Exit the program for now.
    }
    memcpy(filename + bytes_printed - 3, "em", 3); //Get "/proc/%d/mem"
    vmfd = open(filename, O_RDWR);
    if (vmfd == -1) {
        perror("process virtual memory open error");
        exit(1); //Just Exit the program for now.
    }
}

ProcImg::~ProcImg() {
    if (memMapFile) {
        fclose(memMapFile);
        memMapFile = nullptr;
    }
    close(vmfd);
}

long ProcImg::lowest_executable_addr() {
    rewind(memMapFile);
    char line[850];
    long addr;
    char perms[5];
    while(fgets(line, 850, memMapFile) != nullptr) {
        sscanf(line, "%lx-%*lx %s %*s %*s %*d", &addr, perms);

        if(strstr(perms, "x") != nullptr) {
            break;
        }
    }
    return addr;
}

long ProcImg::getlibcaddr() {
    rewind(memMapFile);
    char line[850];
    long addr;
    while(fgets(line, 850, memMapFile) != nullptr) {
        sscanf(line, "%lx-%*lx %*s %*s %*s %*d", &addr);
        if(strstr(line, "libc-") != nullptr) {
            break;
        }
    }
    return addr;
}

void *ProcImg::get_lib_addr(char *libname) {
    rewind(memMapFile);
    char line[850];
    unsigned long sectionStartAddr = 0;
    auto addrSearchRes = libAddrs.find(libname);
    if (addrSearchRes == libAddrs.end()) {
        while(fgets(line, 850, memMapFile) != nullptr) {
            sscanf(line, "%lx-%*lx %*s %*s %*s %*d", &sectionStartAddr);
            if(strstr(line, libname) != nullptr) {
                libAddrs.emplace(string(libname), (void *)sectionStartAddr);
                break;
            }
        }
    } else {
        sectionStartAddr = (unsigned long)addrSearchRes->second;
    }

    if (!sectionStartAddr) { //lib not found
        return nullptr;
    }

    return (void *)sectionStartAddr;
}

#define FIND_LIB_AND_PREP_EXE(libname) \
void *sectionStartAddr = get_lib_addr(libname);\
if (!sectionStartAddr) {\
return nullptr;\
}

#define CHECK_NULL_RETURN(func_call, ret_val_name) \
void *ret_val_name = func_call;\
if (!ret_val_name) {\
return nullptr;\
}

//TODO: decide not to leave this function as is
void *ProcImg::pdlsym(char *libname, char *symbol) {
    CHECK_NULL_RETURN(get_lib_addr(libname), sectionStartAddr)

//    if (!bin) {
//        bin = new exe(libname);
//    }

    exe tmp_bin(libname);
    symaddr_t symaddr = tmp_bin.bin_dlsym(symbol);
    return (void *)((unsigned long)sectionStartAddr + (unsigned long)symaddr.addr);
}

void *ProcImg::get_plt_entry_addr(char *lib_path, char *undefined_symbol) {
    CHECK_NULL_RETURN(get_lib_addr(lib_path), sectionStartAddr)

    void *libAddr = get_lib_addr(lib_path);
    if (!libAddr) {
//        fprintf(stderr, "get_lib_addr returns null");
        return nullptr;
    }

    exe lib(lib_path);
    unsigned long offset = lib.find_rela_func_pltent_offset(undefined_symbol);
    if (offset == (unsigned long)-1) {
//        fprintf(stderr, "find_rela_func_pltent_offset returns null");
        return nullptr;
    }

    return (void *)((unsigned long)libAddr + offset);
}

#define CHECK_ERROR(func_call) \
if (func_call == -1) { \
    return -1;\
}

ssize_t ProcImg::vm_write(unsigned long addr, void *vptr, int len) {

    CHECK_ERROR(lseek(vmfd, addr, SEEK_SET))

//    ssize_t bytes_written = write(vmfd, vptr, (size_t)len);

    return write(vmfd, vptr, (size_t)len);
}

ssize_t ProcImg::vm_read(unsigned long addr, void *vptr, int len) {
    CHECK_ERROR(lseek(vmfd, addr, SEEK_SET))

    return read(vmfd, vptr, (size_t)len);
}

void ProcImg::register_func_to_addr_bijection(char *libpath) {
    if (!libpath) { //register the functions in the main executable
        function_info *curr_func = bin.next_function();
        auto offset = bin.exe_type() == ET_DYN ? (unsigned long)lowest_executable_addr() : (unsigned long)0;

        while (curr_func) {
            void *func_addr = curr_func->start_addr + offset;
            func_to_addr.emplace(curr_func->symbol, func_addr);
            curr_func->start_addr = func_addr;
            addr_to_func.emplace(func_addr, *curr_func);
            curr_func = bin.next_function();
        }
    }
}

void ProcImg::list_functions() {
    for (pair<const string, void *> &func_addr_pair : func_to_addr) {
        cout << func_addr_pair.first << ": " << func_addr_pair.second << endl;
        auto result = addr_to_func.find(func_addr_pair.second);
        if (result != addr_to_func.end()) {
            cout << "size: " << result->second.size << endl;
        }
    }
}

#define EMPTY_FUNCTION_INFO (function_info){nullptr, nullptr, 0}
#define RETURN_IF_IN_RANGE(function_information, addr) {function_info info = function_information;\
if (addr < info.start_addr + info.size) return info;\
return EMPTY_FUNCTION_INFO;}

function_info ProcImg::lookup_addr(void *addr) {
    auto result = addr_to_func.upper_bound(addr);
    //The upper_bound has the lowest address so the address is not within the range of any function
    if (result == addr_to_func.begin()) {
        return EMPTY_FUNCTION_INFO;
    }
    //Not found, so we try to see whether it is within the size of the function with the highest address
    if (result == addr_to_func.end()) {
        RETURN_IF_IN_RANGE(addr_to_func.rbegin()->second, addr);
    }
    RETURN_IF_IN_RANGE((--result)->second, addr)
}