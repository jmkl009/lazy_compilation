#include <iostream>
#include "libs/ptrace.h"
#include "libs/bin_dlsym.hpp"
#include "libs/utils.h"
#include "ProcImg.h"
#include "LibKeeper.h"
#include <limits.h>
#include <time.h>
#include <chrono>
#include "LibKeeper.h"

#include <dlfcn.h>


typedef std::chrono::high_resolution_clock Clock;
using std::cout;


void test() {
    auto time1 = Clock::now();
    pid_t target_pid = findProcessByName("target");
    auto time2 = Clock::now();

    ProcImg targetProc(target_pid);

    char *func1_bin_path = realpath("../libfunc1.so", nullptr);

    exe bin(func1_bin_path);
    void * func1_addr = bin.bin_dlsym("func1").addr + (unsigned long)targetProc.get_lib_addr(func1_bin_path);

    void *plt_entry_addr = targetProc.get_plt_entry_addr("/tmp/tmp.K1e6d9EQ26/libfunc2.so", "func1");
    auto time3 = Clock::now();

    ptrace_attach(target_pid);

    char buf[4 + sizeof(void *)];
    memcpy(buf, "\x48\xb8", 2 * sizeof(char));
    memcpy(buf + 0x02, &func1_addr, sizeof(void *));
    memcpy(buf + 0x0a, "\xff\xe0", 2 * sizeof(char));

//    ptrace_write(target_pid, (unsigned long)plt_entry_addr, buf, 4 + sizeof(void *));
    targetProc.vm_write((unsigned long)plt_entry_addr, buf, 4 + sizeof(void *));

    ptrace_detach(target_pid);
    auto time4 = Clock::now();
    auto time5 = Clock::now();

    std::cout << "finding process takes: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count()
              << " nanoseconds" << std::endl;

    std::cout << "gathering info takes: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time3 - time2).count()
              << " nanoseconds" << std::endl;

    std::cout << "ptrace takes: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time4 - time3).count()
              << " nanoseconds" << std::endl;

    std::cout << "timing takes: "
              << std::chrono::duration_cast<std::chrono::nanoseconds>(time5 - time4).count()
              << " nanoseconds" << std::endl;
}

int main() {

    pid_t target_pid = findProcessByName("target");
    ProcImg targetProc(target_pid);
    targetProc.register_func_to_addr_bijection(nullptr);
    targetProc.list_functions();
    function_info info = targetProc.lookup_addr((void *)(0x565365d857e0 + 1));
    printf("%s: %p, size: %lu\n", info.symbol, info.start_addr, info.size);
//    cout << info.symbol << ": " << info.start_addr << ", size: " << info.size << endl;
//    exe bin("../libfunc1.so");
//    function_info *curr_func = bin.next_function();
//    while (curr_func) {
//        cout << curr_func->symbol << ": " << curr_func->start_addr << ", size: " << curr_func->size << endl;
//        curr_func = bin.next_function();
//    }
//    free(func1_bin_path);
//    LibKeeper keeper;

//    auto time1 = Clock::now();
//    exe *bin = keeper.add_to_lib("../func2.c");
//    auto time2 = Clock::now();

//    if (bin == nullptr) {
//        return 1;
//    }
//    bin->list_rela_sections();

//    std::cout << "add to lib took: "
//              << std::chrono::duration_cast<std::chrono::nanoseconds>(time2 - time1).count()
//              << " nanoseconds" << std::endl;


    return 0;
}