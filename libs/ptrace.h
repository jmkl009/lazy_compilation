#ifdef ARM
	#define REG_TYPE user_regs
#else
	#define REG_TYPE user_regs_struct
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <wait.h>
#include <time.h>
#include "utils.h"
#include <sys/types.h>

/**
 *
 * Use ptrace() to attach to a process. This requires calling waitpid() to
 * determine when the process is ready to be traced.
 * @param target pid of the process to attach to
 */
void ptrace_attach(pid_t target);

/**
 *
 * Detach from a process that is being ptrace()d. Unlike ptrace_cont(), this
 * completely ends our relationship with the target process.
 *
 * @param target pid of the process to detach from. this process must already be
 *   ptrace()d by us in order for this to work.
 */
void ptrace_detach(pid_t target);

/**
 *
 * Use ptrace() to seize a process, similar to ptrace_attach, but without
 * blocking the execution of the process.
 *
 * @param target pid of the process to seize
 */
void ptrace_seize(pid_t target);

/**
 *
 * Use ptrace() to restart a process blocked by a signal.
 *
 * @param target pid of the process to restarting from blocking
 * @param signal the signal to send inplace of the blocking signal
 */
void ptrace_restart(pid_t target, int signal);

/**
 *
 * Continue the execution of a process being traced using ptrace(). Note that
 * this is different from ptrace_cont(): we do not check for signals.
 *
 * @param target pid of the target process
 */
void ptrace_letgo(pid_t target);

/**
 *
 * Advance the execution of a process being traced using ptrace() by a
 * single instruction before stopping again.
 *
 * @param target pid of the target process
 */
void ptrace_singlestep(pid_t target);

/**
 *
 * Use ptrace() to get a process' current register state.  Uses REG_TYPE
 * preprocessor macro in order to allow for both ARM and x86/x86_64
 * functionality.
 *
 * @param target pid of the target process
 * @param regs a struct (either user_regs_struct or user_regs,
 *   depending on architecture) to store the resulting register data in
 */
void ptrace_getregs(pid_t target, struct REG_TYPE* regs);

/**
 *
 * Continue the execution of a process being traced using ptrace(). Note that
 * this is different from ptrace_detach(): we still retain control of the
 * target process after this call.
 *
 * @param target pid of the target process
 */
void ptrace_cont(pid_t target);

/**
 *
 * Use ptrace() to set the target's register state.
 *
 * @param target pid of the target process
 * @param regs a struct (either user_regs_struct or user_regs,
 *   depending on architecture) containing the register state to be set in the
 *   target process
 */
void ptrace_setregs(pid_t target, struct REG_TYPE* regs);

/**
 *
 *
 * Use ptrace() to determine what signal was most recently raised by the target
 * process. This is primarily used for to determine whether the target process
 * has segfaulted.
 *
 * @param target pid of the target process
 * @return a siginfo_t containing information about the most recent signal raised by
 *   the target process
 */
siginfo_t ptrace_getsiginfo(pid_t target);

/**
 * Use ptrace() to read the contents of a target process' address space.
 *
 * @param pid pid of the target process
 * @param addr long addr: the address to start reading from
 * @param vptr a pointer to a buffer to read data into
 * @param len the amount of data to read from the target
 */
void ptrace_read(int pid, unsigned long addr, void *vptr, int len);

/**
 * Use ptrace() to write to the target process' address space.
 *
 * @param pid pid of the target process
 * @param addr the address to start writing to
 * @param vptr a pointer to a buffer containing the data to be written to the target's address space
 * @param len the amount of data to write to the target
 */
void ptrace_write(int pid, unsigned long addr, void *vptr, int len);

/**
 *
 * Check what signal was most recently returned by the target process being
 * ptrace()d. We expect a SIGTRAP from the target process, so raise an error
 * and exit if we do not receive that signal. The most likely non-SIGTRAP
 * signal for us to receive would be SIGSEGV.
 * (Modified to print stacktrace and let the program runs through its course without interrupting the signal delivery.)
 *
 * @param pid pid of the target process
 */
void checktargetsig(int pid);

/**
 *
 * restore the process' backed-up data and register state.
 *
 * @param target pid of the target process
 * @param addr address within the target's address space to write
 * @param backup a buffer pointing to the backed-up data
 * @param datasize the amount of backed-up data to write
 * @param oldregs backed-up register state to restore
 */
void restoreStates(pid_t target, unsigned long addr, void* backup, int datasize, struct REG_TYPE oldregs);
