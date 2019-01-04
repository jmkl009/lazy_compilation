//
// Created by WangJingjin on 2018-12-12.
//

#ifndef C_CODE_HOT_SWAP_SIMPLEREADER_H
#define C_CODE_HOT_SWAP_SIMPLEREADER_H

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "libdwarf/dwarf.h"
#include "libdwarf/libdwarf.h"


void read_cu_list(Dwarf_Debug dbg);
void print_die_data(Dwarf_Debug dbg, Dwarf_Die print_me,int level);
void get_die_and_siblings(Dwarf_Debug dbg, Dwarf_Die in_die,int in_level);

#endif //C_CODE_HOT_SWAP_SIMPLEREADER_H
