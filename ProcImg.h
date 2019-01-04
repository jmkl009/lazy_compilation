//
// Created by WangJingjin on 2018-12-30.
//

#ifndef LAZY_COMPILATION_PROCIMGMAP_H
#define LAZY_COMPILATION_PROCIMGMAP_H

#include <unistd.h>
#include <cstdio>
#include <unordered_map>
#include <map>
#include <string>
#include "libs/bin_dlsym.hpp"

using std::unordered_map;
using std::map;
using std::string;
using std::pair;
using std::cout;
using std::endl;


//TODO: Think about how to deal with local symbols (e.g. static variables)
class ProcImg {
private:

    int vmfd;
    FILE *memMapFile;
    unordered_map<string, void *> libAddrs;
    unordered_map<string, void *> func_to_addr;
    map<void *, function_info> addr_to_func;
    exe bin;


public:
    ProcImg(pid_t pid);
    ~ProcImg();

    //TODO: check whether the assumption that it is the text segment of the main executable is correct.
    long lowest_executable_addr();
    long getlibcaddr();
    void *get_lib_addr(char *libname);
    void *pdlsym(char *libname, char *symbol);
    void *get_plt_entry_addr(char *lib_path, char *undefined_symbol);
    ssize_t vm_write(unsigned long addr, void *vptr, int len);
    ssize_t vm_read(unsigned long addr, void *vptr, int len);
    void register_func_to_addr_bijection(char *libpath);
    void list_functions();
    function_info lookup_addr(void *addr);
};


#endif //LAZY_COMPILATION_PROCIMGMAP_H
