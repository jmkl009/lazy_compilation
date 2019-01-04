#include "ptrace.h"

void ptrace_attach(pid_t target)
{
	int waitpidstatus;

	if(ptrace(PTRACE_ATTACH, target, NULL, NULL) == -1)
	{
		perror("ptrace(PTRACE_ATTACH) failed");
		exit(1);
	}

	if(waitpid(target, &waitpidstatus, WUNTRACED) != target)
	{
		fprintf(stderr, "waitpid(%d) failed\n", target);
		exit(1);
	}
}

void ptrace_seize(pid_t target)
{
    if(ptrace(PTRACE_SEIZE, target, NULL, NULL) == -1)
    {
        perror("ptrace(PTRACE_SEIZE) failed");
        exit(1);
    }
}

void ptrace_detach(pid_t target)
{
	if(ptrace(PTRACE_DETACH, target, NULL, NULL) == -1)
	{
		perror("ptrace(PTRACE_DETACH) failed");
		exit(1);
	}
}

void ptrace_getregs(pid_t target, struct REG_TYPE* regs)
{
	if(ptrace(PTRACE_GETREGS, target, NULL, regs) == -1)
	{
		perror("ptrace(PTRACE_GETREGS) failed");
		exit(1);
	}
}

void ptrace_cont(pid_t target)
{
	struct timespec* sleeptime = (struct timespec*)malloc(sizeof(struct timespec));

	sleeptime->tv_sec = 0;
	sleeptime->tv_nsec = 5000000;

	if(ptrace(PTRACE_CONT, target, NULL, NULL) == -1)
	{
		perror("ptrace(PTRACE_CONT) failed");
		free(sleeptime);
		exit(1);
	}

	nanosleep(sleeptime, NULL);

	// make sure the target process received SIGTRAP after stopping.
	checktargetsig(target);
	free(sleeptime);
}

void ptrace_letgo(pid_t target)
{
	if(ptrace(PTRACE_CONT, target, 0, 0) == -1)
	{
		perror("ptrace(PTRACE_CONT) failed");
		exit(1);
	}
}

void ptrace_setregs(pid_t target, struct REG_TYPE* regs)
{
	if(ptrace(PTRACE_SETREGS, target, NULL, regs) == -1)
	{
		perror("ptrace(PTRACE_SETREGS) failed");
		exit(1);
	}
}

siginfo_t ptrace_getsiginfo(pid_t target)
{
	siginfo_t targetsig;
	if(ptrace(PTRACE_GETSIGINFO, target, NULL, &targetsig) == -1)
	{
		perror("ptrace(PTRACE_GETSIGINFO) failed");
		exit(1);
	}
	return targetsig;
}

void ptrace_singlestep(pid_t target) {
    if(ptrace(PTRACE_SINGLESTEP, target, NULL, NULL) == -1) {
        perror("ptrace(PTRACE_SINGLESTEP) failed");
        exit(1);
    }
}
//
//void ptrace_stop(pid_t target) {
//	if(ptrace(PTRACE_INTERRUPT, target, NULL, SIGSTOP) == -1) {
//		perror("ptrace_stop failed");
//		exit(1);
//	}
//}

void ptrace_restart(pid_t target, int signal) {
    if(ptrace(PTRACE_CONT, target, NULL, signal) == -1) {
        perror("ptrace(PTRACE_restart) failed");
        exit(1);
    }
}

void ptrace_read(int pid, unsigned long addr, void *vptr, int len)
{
	int bytesRead = 0;
	int i = 0;
	long word = 0;
	long *ptr = (long *) vptr;

	while (bytesRead < len)
	{
		word = ptrace(PTRACE_PEEKTEXT, pid, addr + bytesRead, NULL);
		if(word == -1)
		{
			perror("ptrace(PTRACE_PEEKTEXT) failed");
			exit(1);
		}
		bytesRead += sizeof(word);
		ptr[i++] = word;
	}
}

void ptrace_write(int pid, unsigned long addr, void *vptr, int len)
{
	int byteCount = 0;
	long word = 0;

	while (byteCount < len)
	{
		memcpy(&word, vptr + byteCount, sizeof(word));
		word = ptrace(PTRACE_POKETEXT, pid, addr + byteCount, word);
		if(word == -1)
		{
			perror("ptrace(PTRACE_POKETEXT) failed");
			exit(1);
		}
		byteCount += sizeof(word);
	}
//
//	char buf[32];
//	sprintf(buf, "/proc/%d/mem", pid);
//	int fd = open(buf, O_WRONLY);
//	if (fd == -1) {
//        perror("ptrace_write");
//        return;
//	}
//
//	off_t ret = lseek(fd, addr, SEEK_SET);
//    if (ret == -1) {
//        perror("ptrace_write");
//        close(fd);
//        return;
//    }
//
//    ssize_t bytes_written = write(fd, vptr, (size_t)len);
//    if (bytes_written == -1) {
//        perror("ptrace_write");
//        close(fd);
//        return;
//    }
//
//	close(fd);
}

void checktargetsig(int pid)
{
	// check the signal that the child stopped with.
	siginfo_t targetsig = ptrace_getsiginfo(pid);

	// if it wasn't SIGTRAP, then something bad happened (most likely a
	// segfault).
	if(targetsig.si_signo != SIGTRAP)
	{
		fprintf(stderr, "instead of expected SIGTRAP, target stopped with signal %d: %s\n", targetsig.si_signo, strsignal(targetsig.si_signo));
		fprintf(stderr, "sending process %d a SIGSTOP signal for debugging purposes\n", pid);
//		int status;
//		waitpid(pid, &status, 0);
//		ptrace_cont(pid);
//		ptrace(PTRACE_CONT, pid, NULL, SIGSTOP);
        show_stack_trace(pid);
//        ptrace_cont(pid);
        ptrace(PTRACE_CONT, pid, NULL, NULL);
		exit(1);
	}
}

void restoreStates(pid_t target, unsigned long addr, void* backup, int datasize, struct REG_TYPE oldregs)
{
	ptrace_write(target, addr, backup, datasize);
	ptrace_setregs(target, &oldregs);
}
