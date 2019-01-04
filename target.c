//
// Created by WangJingjin on 2019-01-01.
//
#include <dlfcn.h>
#include <stdio.h>

typedef void *(*arbitrary)(void);

void func3() {
    __asm("int $3\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n"
          "nop\n");
}

void func4() {
//    __asm("mov 0xffffffffffffffff, %eax\n"
//          "jmp *%eax");
    ((arbitrary)(0x123456782234))();
//    func3();
//return;
}

int main() {
    void *func1_handle = dlopen("./libfunc1.so", RTLD_LAZY);
    void *func2_handle = dlopen("./libfunc2.so", RTLD_LAZY);
    if (func1_handle == NULL) {
//        func4();
        printf(dlerror());
        return 1;
    }
    if (func2_handle == NULL) {
//        func4();
        printf(dlerror());
        return 1;
    }

    char *line = NULL;
    size_t cap = 0;
    getline(&line, &cap, stdin);
    func3();
//    func4();
    getline(&line, &cap, stdin);

    arbitrary func1 = (arbitrary)dlsym(func1_handle, "func1");
    arbitrary func2 = (arbitrary)dlsym(func2_handle, "func2");

    func1();
    func2();
}