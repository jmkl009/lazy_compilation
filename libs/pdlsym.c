//
// Credit to resilar, gist: https://gist.github.com/resilar/24bb92087aaec5649c9a2afc0b4350c8
//
#ifndef PDLSYM
#define PDLSYM

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <unistd.h>

struct elf {
    pid_t pid;
    uintptr_t base;
    uint8_t class, data;
    uint16_t type;
    int W;

    long (*getN)(pid_t pid, const void *addr, void *buf, long len);

    struct {
        uintptr_t offset;
        uint16_t size, num;
    } phdr;

    uintptr_t symtab, syment;
    uintptr_t strtab, strsz;
};

static long readN(pid_t pid, const void *addr, void *buf, long len)
{
    errno = 0;
    if (!pid) {
        memmove(buf, addr, len);
        return 1;
    }
    while (len > 0) {
        int i, j;
        if ((i = ((unsigned long)addr % sizeof(long))) || len < sizeof(long)) { //TODO: (unsigned long)addr % sizeof(long) -> (unsigned long)addr & (sizeof(long) - 1)?
            union {
                long value;
                unsigned char buf[sizeof(long)];
            } data;
            data.value = ptrace(PTRACE_PEEKDATA, pid, (char *)addr - i, 0);
            if (errno) {
//                fprintf(stderr, "ptrace in readN failed\n");
                perror("ptrace in readN failed:");
                return 0;
            }
            for (j = i; j < sizeof(long) && j-i < len; j++) {
                ((char *)buf)[j-i] = data.buf[j];
            }
            addr = (char *)addr + (j-i);
            buf = (char *)buf + (j-i);
            len -= j-i;
        } else {
            for (i = 0, j = len/sizeof(long); i < j; i++) { //TODO: (len / sizeof(long)) -> len >> ffs(sizeof(long))?
                *(long *)buf = ptrace(PTRACE_PEEKDATA, pid, addr, 0);
                if (errno) return 0;
                addr = (char *)addr + sizeof(long);
                buf = (char *)buf + sizeof(long);
                len -= sizeof(long);
            }
        }
    }
    return 1;
}

static long Ndaer(pid_t pid, const void *addr, void *buf, long len)
{
    if (readN(pid, addr, buf, len)) {
        int i, j;
        for (i = 0, j = len-1; i < j; i++, j--) {
            char tmp = ((char *)buf)[i];
            ((char *)buf)[i] = ((char *)buf)[j];
            ((char *)buf)[j] = tmp;
        }
        return 1;
    }
    fprintf(stderr, "Ndaer failed\n");
    return 0;
}

static uint8_t get8(pid_t pid, const void *addr)
{
    uint8_t ret;
    return readN(pid, addr, &ret, sizeof(uint8_t)) ? ret : 0;
}
static uint16_t get16(struct elf *elf, const void *addr)
{
    uint16_t ret;
    return elf->getN(elf->pid, addr, &ret, sizeof(uint16_t)) ? ret : 0;
}
static uint32_t get32(struct elf *elf, const void *addr)
{
    uint32_t ret;
    return elf->getN(elf->pid, addr, &ret, sizeof(uint32_t)) ? ret : 0;
}
static uint64_t get64(struct elf *elf, const void *addr)
{
    uint64_t ret;
    return elf->getN(elf->pid, addr, &ret, sizeof(uint64_t)) ? ret : 0;
}

static uintptr_t getW(struct elf *elf, const void *addr)
{
    return (elf->class == 0x01) ? (uintptr_t)get32(elf, addr)
                                : (uintptr_t)get64(elf, addr);
}

static int loadelf(pid_t pid, const char *base, struct elf *elf)
{
    uint32_t magic;
    int i, j, loads;

    /**
     * ELF header.
     */
    elf->pid = pid;
    elf->base = (uintptr_t)base;
    if (readN(pid, base, &magic, 4) && !memcmp(&magic, "\x7F" "ELF", 4)
            && ((elf->class = get8(pid, base+4)) == 1 || elf->class == 2)
            && ((elf->data = get8(pid, base+5)) == 1 || elf->data == 2)
            && get8(pid, base+6) == 1) {
        union { uint16_t value; char buf[2]; } data; //Determine the Endian-ness
        data.value = (uint16_t)0x1122;
        elf->getN = (data.buf[0] & elf->data) ? Ndaer : readN;
        elf->type = get16(elf, base + 0x10);
        elf->W = (2 << elf->class);
    } else {
        fprintf(stderr, "bad elf:");
//        if (!readN(pid, base, &magic, 4)) {
//            fprintf(stderr, " !readN(pid, base, &magic, 4)");
//        }
        if (memcmp(&magic, "\x7F" "ELF", 4)) {
            fprintf(stderr, " memcmp(&magic, \"\\x7F\" \"ELF\", 4)");
        }
        if (elf->class != 1 && elf->class != 2) {
            fprintf(stderr, " elf->class != 1 or 2");
        }
        if (elf->data != 1 && elf->data != 2) {
            fprintf(stderr, " elf->data != 1 or 2");
        }
        if (get8(pid, base+6) != 1) {
            fprintf(stderr, " get8(pid, base+6) != 1");
        }
        fprintf(stderr, "\n");
        /* Bad ELF */
        return 0;
    }

    /**
     * Program headers.
     */
    loads = 0;
    elf->strtab = elf->strsz = elf->symtab = elf->syment = 0;
    elf->phdr.offset = getW(elf, base + 0x18 + elf->W);
    elf->phdr.size = get16(elf, base + 0x18 + elf->W*3 + 0x6);
    elf->phdr.num = get16(elf, base + 0x18 + elf->W*3 + 0x8);
    for (i = 0; i < elf->phdr.num; i++) {
        uint32_t phtype;
        uintptr_t offset, vaddr, filesz, memsz;
        const char *ph = base + elf->phdr.offset + i*elf->phdr.size;

        phtype = get32(elf, ph);
        if (phtype != 1 /* PT_LOAD */ && phtype != 2 /* PT_DYNAMIC */)
            continue;

        offset = getW(elf, ph + elf->W);
        vaddr  = getW(elf, ph + elf->W*2);
        filesz = getW(elf, ph + elf->W*4);
        memsz  = getW(elf, ph + elf->W*5);
        if (vaddr < offset || memsz < filesz) {
            fprintf(stderr, "vaddr < offset || memsz < filesz\n");
            return 0;
        }

        if (phtype == 1) { /* PT_LOAD */
            if (elf->type == 2) { /* ET_EXEC */
                if (vaddr - offset < elf->base) {
                    /* this is not the lowest base of the ELF */
                    errno = EFAULT;
                    perror("Not the lowest base of the ELF:");
                    return 0;
                }
            }
            loads++;
        } else if (phtype == 2) { /* PT_DYNAMIC */
            const char *tag;
            uintptr_t type, value;
            tag = (char *)((elf->type == 2) ? 0 : base) + vaddr;
            for (j = 0; 2*j*elf->W < memsz; j++) {
                if ((type = getW(elf, tag + 2*elf->W*j))) {
                    value = getW(elf, tag + 2*elf->W*j + elf->W);
                    switch (type) {
                    case 5: elf->strtab = value; break; /* DT_STRTAB */
                    case 6: elf->symtab = value; break; /* DT_SYMTAB */
                    case 10: elf->strsz = value; break; /* DT_STRSZ */
                    case 11: elf->syment = value; break; /* DT_SYMENT */
                    default: break;
                    }
                } else {
                    /* DT_NULL */
                    break;
                }
            }
        }
    }

    fprintf(stderr, "loads:%d\n",  loads);
    fprintf(stderr, "elf->strtab:%p\n",  elf->strtab);
    fprintf(stderr, "elf->strsz :%p\n", elf->strsz );
    fprintf(stderr, "elf->symtab:%p\n", elf->symtab);
    fprintf(stderr, "elf->syment:%p\n", elf->syment);

    return loads && elf->strtab && elf->strsz && elf->symtab && elf->syment;
}

int elf_sym_iter(struct elf *elf, int i, uint32_t *stridx, uintptr_t *value)
{
    if ((i+1)*elf->syment-1 < elf->strtab - elf->symtab) {
        const char *sym = (char *)elf->symtab + elf->syment*i;
        if (elf->symtab < elf->base)
            sym += elf->base;
        if ((*stridx = get32(elf, sym)) < elf->strsz) {
            if ((*value = getW(elf, sym + elf->W)) && elf->type != 2)
                *value += elf->base;
            return 1;
        }
    }
    fprintf(stderr, "elf sym iter reaches the end\n");
    return 0;
}

void *pdlsym(pid_t pid, void *base, const char *symbol)
{
    struct elf elf;
    if (loadelf((pid == getpid()) ? 0 : pid, base, &elf)) {
        int i, j;
        uint32_t stridx;
        uintptr_t value;
        const char *pstrtab = (char *)elf.strtab;
        fprintf(stderr, "elf.base:%p\n", elf.base);
        if (elf.strtab < elf.base)
            pstrtab += elf.base;
        for (i = 0; elf_sym_iter(&elf, i, &stridx, &value); i++) {
            if (value) {
                for (j = 0; stridx+j < elf.strsz; j++) {
                    if (symbol[j] != (char)get8(pid, pstrtab + stridx+j))
                        break;
                    if (!symbol[j])
                        return (void *)value;
                }
                for (j = 0; stridx+j < elf.strsz; j++) {
                    printf("%c", (char)get8(pid, pstrtab + stridx+j));
                }
                printf("\n");
            }
        }
    }

    return NULL;
}

#endif