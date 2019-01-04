//
// Created by WangJingjin on 2018-12-11.
//

#ifndef C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
#define C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H

#include "hot_swap.hpp"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>
using std::unordered_map;
using std::pair;

typedef struct func_info {
    char *shared_obj_path;
    void *handle;

    ~func_info() {
        if (shared_obj_path != NULL) {
            free(shared_obj_path);
        }
    }
} func_info;

typedef enum {
    STOPPED,
    RUNNING
} target_process_state;

class FunctionInjector {
private:
    pid_t target;
    char *tempdir;
    char *srcFilePath;
    exe *target_exe;
    TargetUsefulFuncAddrs func_addrs;
    vector<string> linker_flags;
    unordered_map<string, unordered_map<string, func_info*>> sources_to_compiled_functions;
    unordered_map<string, func_info*> *compiled_functions;

    void *inject_func_main(func_info *info, char *funcname, symaddr_t targetFuncAddr, target_process_state state);


public:

    /**
     * Construct a function injector from pid and tempdir.
     *
     * @param pid the target process to inject
     * @param tempdir the temporary directory to store the compiled files
     */
    FunctionInjector(pid_t pid, char *tempdir);
    ~FunctionInjector();

    /**
     * Assigning a source file to the function injector for it later to compile a function in.
     *
     * @param srcFilePath the absolute path of the source file
     */
    void assign_source(char *srcFilePath);

    /**
     *
     * Compile a function found in the source file assigned to the injector earlier.
     *
     * @param funcname the name of the function to compile
     * @return
     */
    char *compile_func(char *funcname);

    /**
     *
     * Perform function injection!!!!!! (Including using ptrace and everything!)
     *
     * @param funcname the name of the function to inject.
     * @param state the state of the target process, either RUNNING or STOPPED.
     * (Right, we need to deal with the two situations separately.)
     * @return 0 for success, otherwise a error ocurred.
     */
    int inject_func(char *funcname, target_process_state state);

    /**
     *
     * Perform function injection without calling ptrace, if the user has already called ptrace.
     *
     * @param funcname the name of the function to inject.
     * @param state the state of the target process, either RUNNING or STOPPED.
     * (Right, we need to deal with the two situations separately.)
     * @return 0 for success, otherwise a error ocurred.
     */
    int inject_func_under_ptrace(char *funcname, target_process_state type);
};


#endif //C_CODE_HOT_SWAP_FUNCTIONINJECTOR_H
