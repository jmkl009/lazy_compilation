//
// Created by WangJingjin on 2019-01-02.
//

#include "LibKeeper.h"

#include <chrono>
typedef std::chrono::high_resolution_clock Clock;


LibKeeper::LibKeeper() : lib_to_mem() {}

LibKeeper::~LibKeeper() {
    for (pair<const string, lib_info> &kv_pair : lib_to_mem) {
        shm_unlink(kv_pair.second.tmp_file_name);
        delete kv_pair.second.library;
    }
}

bool LibKeeper::compile(const char *srcPath, int outfd) {

//    int infd = open(srcPath, O_RDONLY);
//    if (infd == -1) {
//        return false;
//    }

    pid_t pid = fork();
    if (pid == -1) {
//        close(infd);
        return false;
    } else if (pid == 0) { //In child
//        int ret = dup2(infd, STDIN_FILENO);
//        if (ret == -1) {
//            exit(1);
//        }
        int ret = dup2(outfd, STDOUT_FILENO);
        if (ret == -1) {
            exit(1);
        }

        // "-w" flag suppresses warnings
//        execlp(C_COMPILER, C_COMPILER, "-x", "c", "-shared", "-fPIC", "-w", "-o", "/dev/stdout", "/dev/stdin", nullptr);
        execlp(C_COMPILER, C_COMPILER, "-x", "c", "-shared", "-fPIC", "-w", "-o", "/dev/stdout", srcPath, nullptr);

        exit(1);
    } else { //In parent
        int status;
        auto time1 = Clock::now();
        waitpid(pid, &status, 0);
        auto time2 = Clock::now();
        std::cout << "compilation took: "
                  << std::chrono::duration_cast<std::chrono::milliseconds>(time2 - time1).count()
                  << " miliseconds" << std::endl;
        if (!WIFEXITED(status) || WEXITSTATUS(status)) { //Exited not normally
//            close(infd);
            return false;
        }
    }

//    close(infd);
    return true;
}

#define SHARED_LIB_ERROR (LibKeeper::so_info){.fd = -1, .tmp_file_name = tmp_name}
//TODO: need to make sure that the name are truly not repeated
LibKeeper::so_info LibKeeper::compile_to_mem(const char *srcPath) {
    // tmp_name + 4 to eliminate the "/tmp" in the file name.
    const char *tmp_name = tmpnam(nullptr) + 4;
    // No O_EXCL here for performance concern, add O_EXCL for debug purpose.
    int fd = shm_open(tmp_name, O_RDWR | O_CREAT, S_IRWXU);
    if (fd == -1) {
        perror("error while trying to open a shared object");
        return SHARED_LIB_ERROR;
    }

    int success = compile(srcPath, fd);
    if (!success) {
        perror("error while trying to compile a shared object");
        return SHARED_LIB_ERROR;
    }

    return (LibKeeper::so_info){.fd = fd, .tmp_file_name = tmp_name};
}

LibKeeper::so_info LibKeeper::compile_to_fs(const char *srcPath) {
    // tmp_name + 4 to eliminate the "/tmp" in the file name.
    const char *tmp_name = tmpnam(nullptr) + 4;
    int fd = open("/home/william/tmpfile", O_RDWR | O_CREAT, S_IRWXU);
    if (fd == -1) {
        perror("error while trying to open a shared object");
        return SHARED_LIB_ERROR;
    }

    int success = compile(srcPath, fd);

    if (!success) {
        perror("error while trying to compile a shared object");
        return SHARED_LIB_ERROR;
    }

    return (LibKeeper::so_info){.fd = fd, .tmp_file_name = tmp_name};
}

exe *LibKeeper::add_to_lib(const char *srcPath) {
    so_info info = compile_to_mem(srcPath);
//    so_info info = compile_to_fs(srcPath);
    if (info.fd == -1) {
        shm_unlink(info.tmp_file_name);
        return nullptr;
    }

    exe *compiled_lib = new exe(info.fd);
    lib_to_mem.emplace(string(srcPath), (LibKeeper::lib_info){.library = compiled_lib,
                                                   .tmp_file_name = info.tmp_file_name});

    close(info.fd);
    return compiled_lib;
}

