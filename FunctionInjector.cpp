//
// Created by WangJingjin on 2018-12-11.
//

#include "FunctionInjector.h"

FunctionInjector::FunctionInjector(pid_t pid, char *tempdir) : target(pid), srcFilePath(NULL),
                                                                tempdir(tempdir), compiled_functions(NULL),
                                                               sources_to_compiled_functions() {
    init_target_useful_func_addrs(target, &this->func_addrs);
    char exe_name_buf[256];
    sprintf(exe_name_buf, "/proc/%d/exe", target);
    target_exe = new exe(exe_name_buf);
    linker_flags = target_exe->lookup_linker_flags();
}

FunctionInjector::~FunctionInjector() {
    delete target_exe;
    if (srcFilePath != NULL) {
        free(srcFilePath);
    }
    for (pair<const string, unordered_map<string, func_info*>> &source : sources_to_compiled_functions){
        for (pair<const string, func_info *> &elem : source.second) {
            delete elem.second;
        }
    }
}

void FunctionInjector::assign_source(char *srcFilePath) {
    if (!srcFilePath) {
        return;
    }
    string src_str = string(srcFilePath);
    auto search_res = sources_to_compiled_functions.find(src_str);
    if (search_res == sources_to_compiled_functions.end()) {
        sources_to_compiled_functions.emplace(src_str, unordered_map<string, func_info*>());
    }

    compiled_functions = &sources_to_compiled_functions[src_str];
    if (this->srcFilePath != NULL) {
        free(this->srcFilePath);
    }
    this->srcFilePath = strdup(srcFilePath);
}

char *FunctionInjector::compile_func(char *funcname) {
    if (!compiled_functions || !srcFilePath) {
        return NULL;
    }
    char *shared_obj_path = compile_func_in_file(srcFilePath, funcname, tempdir, &linker_flags);
    printf("shared_obj_path: %s\n", shared_obj_path);
    char *shared_obj_real_path = realpath(shared_obj_path, NULL);
    printf("shared_obj_real_path: %s\n", shared_obj_real_path);
    free(shared_obj_path);

    func_info *info = (func_info *)malloc(sizeof(func_info));
    info->shared_obj_path = shared_obj_real_path;
    info->handle = NULL;

    compiled_functions->emplace(funcname, info);
    return shared_obj_real_path;
}

#define INJECT_INIT \
if (!compiled_functions) { \
    return 1; \
} \
string lib_name(funcname);\
auto lib_info = compiled_functions->find(lib_name);\
if (lib_info == compiled_functions->end()) {\
    return 1; \
}\
func_info *info = lib_info->second;\
symaddr_t targetFuncAddr = target_exe->bin_dlsym(funcname);

#define INJECT_RETURN \
if (handle == NULL) {\
    return 3; \
}\
lib_info->second->handle = handle;\
return 0;

int FunctionInjector::inject_func(char *funcname, target_process_state type) {
    INJECT_INIT

    ptrace_attach(target);
    void *handle = inject_func_main(info, funcname, targetFuncAddr, type);
    ptrace_detach(target);

    INJECT_RETURN
}

int FunctionInjector::inject_func_under_ptrace(char *funcname, target_process_state type) {
    INJECT_INIT

    void *handle = inject_func_main(info, funcname, targetFuncAddr, type);

    INJECT_RETURN
}

void *FunctionInjector::inject_func_main(func_info *info, char *funcname, symaddr_t targetFuncAddr, target_process_state state) {
    if (info->handle) {
        int ret = pdlclose(target, info->handle, func_addrs.targetDlcloseAddr);
        if (ret == 1) {
            return NULL; //pdclose error.
        }
    }
    void *handle = NULL;
    if (state == STOPPED) {
        handle = pdlopen(target, info->shared_obj_path, funcname, func_addrs.targetMallocAddr,
                         func_addrs.targetDlopenAddr, func_addrs.targetFreeAddr, targetFuncAddr,
                         strlen(info->shared_obj_path) + 1, 1);
    } else if (state == RUNNING) {
        handle = pdlopen(target, info->shared_obj_path, funcname, func_addrs.targetMallocAddr,
                         func_addrs.targetDlopenAddr, func_addrs.targetFreeAddr, targetFuncAddr,
                         strlen(info->shared_obj_path) + 1, 2);
    }
    return handle;
}
