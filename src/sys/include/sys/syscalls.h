/*
 *  Copyright (C) 2000,2001 by Anders Gavare.  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 *  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 *  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 *  OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 *  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 *  OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 *  SUCH DAMAGE.
 */

/*
 *  sys/syscalls.h  --  the virtual file system
 */

#ifndef	__SYS__SYSCALLS_H
#define	__SYS__SYSCALLS_H

#include <sys/defs.h>
#include <sys/proc.h>
#include <sys/vfs.h>


struct proc;
struct sigcontext;
struct timespec;
struct timeval;


void syscall_init ();

/*  sys_fork.c:  */
int sys_fork (ret_t *res, struct proc *p);

/*  sys_fd.c:  */
int sys_open (ret_t *res, struct proc *p, char *filename, int flags, mode_t mode);
int sys_close (ret_t *res, struct proc *p, int d);
int sys_read (ret_t *res, struct proc *p, int d, void *buf, size_t len);
int sys_write (ret_t *res, struct proc *p, int d, void *buf, size_t len);
int sys_writev (ret_t *res, struct proc *p, int d, struct iovec *iov, int iovcnt);
int sys_ioctl (ret_t *res, struct proc *p, int d, unsigned long req, unsigned long arg1);
int sys_fstat (ret_t *res, struct proc *p, int d, struct stat *ss);
int sys_dup2 (ret_t *res, struct proc *p, int dold, int dnew);
int sys_fchdir (ret_t *res, struct proc *p, int fd);
int sys_lseek (off_t *res, struct proc *p, int fd, off_t offset, int whence);
int sys_fcntl (ret_t *res, struct proc *p, int fd, int cmd, int arg);

/*  sys_file.c:  */
int sys_access (ret_t *res, struct proc *p, char *name, int mode);
int sys_chdir (ret_t *res, struct proc *p, char *name);
int sys_chroot (ret_t *res, struct proc *p, char *name);
int sys_stat (ret_t *res, struct proc *p, char *name, struct stat *ss);
int sys_fstatfs (ret_t *res, struct proc *p, int fd, struct statfs *ss);
int sys_umask (ret_t *res, struct proc *p, int newmask);
int sys_getdirentries (ret_t *res, struct proc *p, int fd, char *buf, int nbytes, long *basep);
int sys_readlink (ret_t *res, struct proc *p, char *path, char *buf, size_t bufsize);
int sys_mount (ret_t *res, struct proc *p, char *type, char *dir, int flags, void *data);

/*  sys_proc.c:  */
int sys_exit (ret_t *res, struct proc *p, int exitcode);
int sys_wait4 (ret_t *res, struct proc *p, pid_t wpid, int *status, int options, void *rusage);
int sys_getpid (ret_t *res, struct proc *p);
int sys_getppid (ret_t *res, struct proc *p);
int sys_getpgrp (ret_t *res, struct proc *p);
int sys_setpgid (ret_t *res, struct proc *p, pid_t pid, pid_t grp);
int sys_getuid (ret_t *res, struct proc *p);
int sys_geteuid (ret_t *res, struct proc *p);
int sys_getgid (ret_t *res, struct proc *p);
int sys_getegid (ret_t *res, struct proc *p);
int sys_setuid (ret_t *res, struct proc *p, uid_t u);
int sys_seteuid (ret_t *res, struct proc *p, uid_t u);
int sys_setgid (ret_t *res, struct proc *p, gid_t g);
int sys_setegid (ret_t *res, struct proc *p, gid_t g);
int sys_issetugid (ret_t *res, struct proc *p);
int sys_nanosleep (ret_t *res, struct proc *p, const struct timespec *rqtp, struct timespec *rmtp);

/*  sys_execve.c:  */
int sys_execve (ret_t *retval, struct proc *p, char *path, char *argv[], char *envp[]);

/*  sys_mmap.c:  */
int sys_mmap (ret_t *res, struct proc *p, void *addr, size_t len, int prot, int flags, int fd, off_t offset);
int sys_munmap (ret_t *res, struct proc *p, void *addr, size_t len);
int sys_mprotect (ret_t *res, struct proc *p, void *addr, size_t len, int prot);
int sys_break (ret_t *res, struct proc *p, char *nsize);

/*  sys_sig.c:  */
int sys_sigsuspend (ret_t *res, struct proc *p, int sigmask);
int sys_sigprocmask (ret_t *res, struct proc *p, int how, sigset_t *set, sigset_t *oset);
int sys_sigaction (ret_t *res, struct proc *p, int sig, struct sigaction *act, struct sigaction *oact);
int sys_sigreturn (ret_t *res, struct proc *p, struct sigcontext *scp);
int sys_kill (ret_t *res, struct proc *p, pid_t pid, int sig);

/*  sys_time.c:  */
int sys_gettimeofday (ret_t *res, struct proc *p, struct timeval *tp, void *timezoneptr);

/*  sys_misc.c:  */
int sys_reboot (ret_t *res, struct proc *p, int how);

/*  sys_socket.c:  */
int sys_pipe (ret_t *res, struct proc *p, int *fds);


#endif	/*  __SYS__SYSCALLS_H  */

