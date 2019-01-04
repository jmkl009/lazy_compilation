//
// Created by WangJingjin on 2019-01-02.
//

#ifndef LAZY_COMPILATION_COMPILER_H
#define LAZY_COMPILATION_COMPILER_H

#include <unordered_map>
#include <unistd.h>
#include <cstdio>

#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>

#include "libs/bin_dlsym.hpp"

#define C_COMPILER "gcc"

using std::unordered_map;
using std::string;
using std::pair;

class LibKeeper {

private:
    typedef struct {
        int fd;
        const char *tmp_file_name;
    } so_info;

    typedef struct {
        exe *library;
        const char *tmp_file_name;
    } lib_info;

    unordered_map<string, lib_info> lib_to_mem;

    bool compile(const char *srcPath, int outfd);
    so_info compile_to_mem(const char *srcPath);
    so_info compile_to_fs(const char *srcPath);

public:
    LibKeeper();
    ~LibKeeper();

    exe *add_to_lib(const char *srcPath);
};


#endif //LAZY_COMPILATION_COMPILER_H
