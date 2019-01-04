//
// Created by WangJingjin on 2018-12-10.
//

#ifndef DDB_UTILS_H
#define DDB_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <libunwind-ptrace.h>
#include "bin_dlsym.hpp"

#define INTEL_RET_INSTRUCTION 0xc3
#define INTEL_INT3_INSTRUCTION 0xcc

/**
 *
 * Given an absolute path to a file, return the pointer to the start of the
 * filename in the path
 *
 * @param filePath
 * @return The filename
 */
char *get_filename_from_path(char *filePath);

/**
 *
 * Given the name of a process, try to find its PID by searching through /proc
 * and reading /proc/[pid]/exe until we find a process whose name matches the
 * given process.
 *
 * @param processName processName: name of the process whose pid to find
 * @return a pid_t containing the pid of the process (or -1 if not found)
 */
pid_t findProcessByName(const char* processName);

/**
 *
 * Search the target process' /proc/pid/maps entry and find an executable
 * region of memory that we can use to run code in.
 *
 * @param pid pid of process to inspect
 * @return a long containing the address of an executable region of memory inside the
 *   specified process' address space.
 */
long freespaceaddr(pid_t pid);

/**
 *
 * Gets the base address of libc.so inside a process by reading /proc/pid/maps.
 *
 * @param pid the pid of the process whose libc.so base address we should
 *   find
 * @return a long containing the base address of libc.so inside that process
 */
long getlibcaddr(pid_t pid);

/**
 * * checkloaded()
 *
 * Given a process ID and the name of a shared library, check whether that
 * process has loaded the shared library by reading entries in its
 * /proc/[pid]/maps file. If so, return the address of the symbol in the library.
 *
 * @param pid the pid of the process to check
 * @param libname the library to search /proc/[pid]/maps for
 * @param symbol the function to look for in the shared library
 * @return an void * pointer point to the address of the dynamically linked function
 * a NULL is returned if the library is not found;
 */
void * checkloaded(pid_t pid, char* libname, char *symbol);

/**
 *
 * Find the address of a function within our own loaded copy of libc.so.
 *
 * @param funcName funcName: name of the function whose address we want to find
 * @return a long containing the address of that function
 */
long getFunctionAddress(char* funcName);

/**
 *
 * Starting at an address somewhere after the end of a function, search for the
 * "ret" instruction that ends it. We do this by searching for a 0xc3 byte, and
 * assuming that it represents that function's "ret" instruction. This should
 * be a safe assumption. Function addresses are word-aligned, and so there's
 * usually extra space at the end of a function. This space is always padded
 * with "nop"s, so we'll end up just searching through a series of "nop"s
 * before finding our "ret". In other words, it's unlikely that we'll run into
 * a 0xc3 byte that corresponds to anything other than an actual "ret"
 * instruction.
 *
 * Note that this function only applies to x86 and x86_64, and not ARM.
 *
 * @param endAddr the ending address of the function whose final "ret"
 *   instruction we want to find
 * @return an unsigned char* pointing to the address of the final "ret" instruction
 *   of the specified function
 */
unsigned char* findRet(void* endAddr);

/**
 *
 * Print program usage and exit.
 *
 * @param name the name of the executable we're running out of
 */
void usage(char* name);

/**
 *
 * Get the smallest virtual memory address (base) in the target process.
 *
 * @param pid the pid of the process to check
 * @return A void pointer representing the base address
 */
void *get_base_addr(pid_t pid);

/**
 *
 * Show the stack trace of the target process, while it is stopped.
 *
 * @param pid the pid of the process to check
 */
void show_stack_trace(pid_t pid);

#endif //DDB_UTILS_H
