//
// Created by WangJingjin on 2018/11/3.
//

#ifndef BIN_DLSYM
#define BIN_DLSYM

#include "bin_dlsym.hpp"

Elf64_Addr exe::lookup_symbol(const char *symname) {
    int i, j;
    char *strtab;
    Elf64_Sym *symtab;
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_SYMTAB) {
            strtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
            symtab = (Elf64_Sym *)&h.mem[h.shdr[i].sh_offset];
            for (j = 0; (unsigned)j < h.shdr[i].sh_size / sizeof(Elf64_Sym); j++, symtab++) {
                if(strcmp(&strtab[symtab->st_name], symname) == 0)
                    return (symtab->st_value);
            }
        }
    }
    return 0;
}

void exe::init(int fd) {
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        exit(-1);
    }

    mem_len = (size_t)st.st_size;
    h.mem = (uint8_t *)mmap(nullptr, mem_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (h.mem == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    close(fd);

    h.ehdr = (Elf64_Ehdr *)h.mem;
    h.phdr = (Elf64_Phdr *)(h.mem + h.ehdr->e_phoff);
    h.shdr = (Elf64_Shdr *)(h.mem + h.ehdr->e_shoff);

    if (h.mem[0] != 0x7f && !strcmp((char *)&h.mem[1], "ELF")) {
        fprintf(stderr, "This is not an ELF file\n");
        exit(-1);
    }

    if (h.ehdr->e_shstrndx == 0 || h.ehdr->e_shoff == 0 || h.ehdr->e_shnum == 0) {
        fprintf(stderr, "Section header table not found\n");
        exit(-1);
    }

    func_iter.symtab = nullptr;
}

#define CHECK_OPEN_ERROR(filename)\
int fd = open(filename, O_RDONLY);\
if (fd < 0) {\
perror("open");\
exit(-1);\
}

exe::exe(const char * path) {
//    bin_name = strdup(path);
//    h.exec = bin_name;
    CHECK_OPEN_ERROR(path)

    init(fd);
}

exe::exe(unsigned pid) {
    char buf[32];
    sprintf(buf, "/proc/%d/exe", pid);

    CHECK_OPEN_ERROR(buf)

    init(fd);
}

exe::exe(int fd) {
    init(fd);
}

//exe::exe(const char *name, const void *mem, size_t mem_len) : fd(-1), mem_len(mem_len) {
//    if (mem == nullptr) {
//        fprintf(stderr, "mem is null\n");
//        exit(-1);
//    }
//
//    h.mem = (uint8_t *)mem;
//    init();
//}

exe::~exe() {
    munmap(h.mem, mem_len);
//    if (fd != -1) {
//        close(fd);
//    }
//    free(bin_name);
}

symaddr_t exe::bin_dlsym(const char *symbol) {
    h.symname = (char *)symbol;
    if ((h.symaddr = lookup_symbol(h.symname)) == 0) {
        printf("Unable to find symbol: %s not found in library\n", h.symname);
        exit(-1);
    }

    return (symaddr_t){.addr = (void *)h.symaddr, .type = h.ehdr->e_type};
}

vector<string> exe::lookup_linker_flags() {
    vector<string> linker_flags;
    int i, j;
    char *strtab;
    Elf64_Dyn *dyntab;
    char buffer[256];
    strncpy(buffer, "-l", 2);
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_DYNAMIC) {
            dyntab = (Elf64_Dyn *)&h.mem[h.shdr[i].sh_offset];
            strtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
            for (j = 0; (unsigned)j < h.shdr[i].sh_size / sizeof(Elf64_Dyn); j++) {
                if (dyntab[j].d_tag == DT_NEEDED) {
//                        printf("%s\n", strtab + dyntab[j].d_un.d_val);
                    char *libname = strtab + dyntab[j].d_un.d_val;
                    char *realname_start = strstr(libname, "lib");
                    if (!realname_start) {
                        continue;
                    }
                    realname_start += 3;
                    char *realname_end = strstr(realname_start, ".so");
                    size_t libname_size = realname_end - realname_start;
                    strncpy(buffer + 2, realname_start, libname_size);
                    buffer[libname_size + 2] = '\0';
//                        printf("%s\n", buffer);
                    linker_flags.emplace_back(string(buffer));
                }
            }
            break;
        }
    }
    return linker_flags;
}

//TODO: try to incorporate addend and REL section into consideration
void exe::list_rela_sections() {
    int i, j;
    char *shstrtab = (char *)&h.mem[h.shdr[h.ehdr->e_shstrndx].sh_offset];

    Elf64_Sym *dynsymtab;
    char *dynsymstrtab;
//    Elf64_Xword strtab_size;
    //We find the .dynsym section first
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_DYNSYM) {
            dynsymtab = (Elf64_Sym *)&h.mem[h.shdr[i].sh_offset];
            dynsymstrtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
//            strtab_size = h.shdr[h.shdr[i].sh_link].sh_size;
            break;
        }
    }

    if (i >= h.ehdr->e_shnum) { //.dynsym section not found
        return;
    }
//    for (i = 1; i < strtab_size; i += printf("%s\n", dynsymstrtab + i)) {
//        printf("at idx = %d, ", i);
//    }

    //Then we are looking for .rela.plt section
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_RELA && strcmp(shstrtab + h.shdr[i].sh_name, ".rela.plt") == 0) {
            Elf64_Shdr rela_shdr = h.shdr[i];
            Elf64_Rela *plt_symbols = (Elf64_Rela *)&h.mem[rela_shdr.sh_offset];
            Elf64_Xword rela_num = rela_shdr.sh_size / rela_shdr.sh_entsize;
//            printf("rela_num = %lx\n", rela_num);
            for (j = 0; j < rela_num; j++) {
                printf("%s\n", dynsymstrtab + dynsymtab[ELF64_R_SYM(plt_symbols[j].r_info)].st_name);
            }

            break;
        }
    }
//    return 0;
}

unsigned long exe::find_rela_func_pltent_offset(const char *funcname) {
    int i;
    char *shstrtab = (char *)&h.mem[h.shdr[h.ehdr->e_shstrndx].sh_offset];

    Elf64_Sym *dynsymtab;
    char *dynsymstrtab;

    //We find the .dynsym section first
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_DYNSYM) {
            dynsymtab = (Elf64_Sym *)&h.mem[h.shdr[i].sh_offset];
            dynsymstrtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
            break;
        }
    }

    if (i >= h.ehdr->e_shnum) { //.dynsym section not found
        return (unsigned long)-1;
    }

    //Then we are looking for .rela.plt section
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_RELA && strcmp(shstrtab + h.shdr[i].sh_name, ".rela.plt") == 0) {
            break;
        }
    }

    if (i >= h.ehdr->e_shnum) {
        return (unsigned long)-1; //No .rela.plt section found
    }

    //Then we look for the entry of the function in the .rela.plt section
    Elf64_Shdr rela_shdr = h.shdr[i];
    Elf64_Rela *rela_plt_symbols = (Elf64_Rela *)&h.mem[rela_shdr.sh_offset];
    Elf64_Xword rela_num = rela_shdr.sh_size / rela_shdr.sh_entsize;
    for (i = 0; i < rela_num; i++) {
        if (!strcmp(dynsymstrtab + dynsymtab[ELF64_R_SYM(rela_plt_symbols[i].r_info)].st_name, funcname)) {
            break;
        }
    }

    unsigned func_ent_idx = i;

    if (i >= rela_num) {
        return (unsigned long)-1; //No target function found in the .rela.plt section
    }

    //we then find the .plt section
    for (i = 0; i < h.ehdr->e_shnum; i++) {
        if (h.shdr[i].sh_type == SHT_PROGBITS && strcmp(shstrtab + h.shdr[i].sh_name, ".plt") == 0) {
            break;
        }
    }

    if (i >= h.ehdr->e_shnum) {
        return (unsigned long)-1; //No .plt section found
    }

    //rely on the fact that the order of PLT relocations matches the order of the stubs in .plt section

    return h.shdr[i].sh_offset + h.shdr[i].sh_entsize * (func_ent_idx + 1);
}

function_info *exe::next_function() {
    if (!func_iter.symtab) {
        int i;
        for (i = 0; i < h.ehdr->e_shnum; i++) {
            if (h.shdr[i].sh_type == SHT_SYMTAB) {
                func_iter.strtab = (char *)&h.mem[h.shdr[h.shdr[i].sh_link].sh_offset];
                func_iter.symtab_size = h.shdr[i].sh_size / sizeof(Elf64_Sym);
                func_iter.symtab = (Elf64_Sym *)&h.mem[h.shdr[i].sh_offset];
                func_iter.curr_idx = 0;
                break;
            }
        }
        if (i >= h.ehdr->e_shnum) { //No symbol table found.
            return nullptr;
        }
    }

    for (; func_iter.curr_idx < func_iter.symtab_size; func_iter.curr_idx++) {
        Elf64_Sym curr_symbol = func_iter.symtab[func_iter.curr_idx];
        if (ELF64_ST_TYPE(curr_symbol.st_info) == STT_FUNC && curr_symbol.st_size > 0) {
            Elf64_Sym valid_symbol = func_iter.symtab[func_iter.curr_idx];
            curr_func_info.symbol = &func_iter.strtab[valid_symbol.st_name];
            curr_func_info.size = valid_symbol.st_size;
            curr_func_info.start_addr = (void *)valid_symbol.st_value;
            func_iter.curr_idx++;
            return &curr_func_info;
        }
    }

    func_iter.curr_idx = 0;
    return nullptr;
}

Elf64_Half exe::exe_type() {
    return h.ehdr->e_type;
}

#endif
