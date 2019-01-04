//
// Created by WangJingjin on 2018-12-10.
//

#ifndef DDB_BIN_DLSYM_H
#define DDB_BIN_DLSYM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <elf.h>
#include <libelf.h>

#include <sys/types.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <vector>
#include <string>
#include <iostream>

using std::vector;
using std::string;

typedef struct handle {
    Elf64_Ehdr *ehdr; //Elf header
    Elf64_Phdr *phdr; //Program header
    Elf64_Shdr *shdr; //Section header
    uint8_t *mem;
    char *symname;
    Elf64_Addr symaddr;
    struct user_regs_struct pt_reg;
    char *exec;
} handle_t;

typedef struct {
    void * addr; //Virtual memory address
    uint16_t type; //Shared library or library
} symaddr_t;

typedef struct  {
    char *symbol;
    void * start_addr;
    size_t size;
} function_info;

typedef struct {
    Elf64_Sym *symtab;
    char *strtab;
    size_t curr_idx;
    size_t symtab_size;
} function_iterator;

class exe {
private:
//    int fd;
    handle_t h;
    size_t mem_len;
//    char *bin_name;
    function_iterator func_iter;
    function_info curr_func_info;
    Elf64_Addr lookup_symbol(const char *symname);
    void init(int fd);

public:

    /**
     * Consturct an exe object from an Elf file.
     * @param path The name of the Elf file
     */
    exe(const char *path);

    exe(unsigned pid); //The unsigned is actually pid_t

    exe(int fd);
//    exe(const char *name, const void *mem, size_t mem_len);
    ~exe();

    /**
     * Given a symbol (usually function name), output its address in memory. (Just like dlsym)
     * @param symbol The name of the symbol
     * @return a symaddr struct which indicates whether it is a shared library
     * or an excutable along with its place in memory.
     */
    symaddr_t bin_dlsym(const char *symbol);

    /**
     * Get the linker flags the executable was compiled with.
     * @return a vector of linker flags the executable was compiled with.
     */
    vector<string> lookup_linker_flags();

    void list_rela_sections();
    unsigned long find_rela_func_pltent_offset(const char *funcname);

    function_info *next_function();
    Elf64_Half exe_type();
};

#endif //DDB_BIN_DLSYM_H
