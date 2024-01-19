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
 *  modules/exec/openbsd_aout/openbsd_aout_syscalls.c  --  OpenBSD syscalls
 *
 *	Appart from simply directing syscall numbers to the corresponding
 *	syscall handlers (sys_xxxxx() functions), this file also contains the
 *	neccessary "wrapper code" to convert between the OpenBSD view of
 *	the universe, and the way things work internally in Yoctix.
 *
 *	One of the reasons for this is to make things more portable, but
 *	the way it works now it is not quite as portable as I would like :)
 *
 *	For example, it should be possible to change the size of time_t
 *	internally in Yoctix without affecting OpenBSD binary emulation.
 *	Therefore, we need to convert between "hardcoded" varialble types
 *	(for example s_int32_t) and "internal" variable types (time_t).
 */

#include <sys/time.h>


extern char *compile_info;
extern char *compile_generation;


struct openbsd_aout__timespec
      {
	s_int32_t	tv_sec;
	s_int32_t	tv_nsec;
      };


struct openbsd_aout__timeval
      {
	s_int32_t	tv_sec;
	s_int32_t	tv_usec;
      };


struct openbsd_stat
      {
	u_int32_t	st_dev;
	u_int32_t	st_ino;
	u_int16_t	st_mode;
	u_int16_t	st_nlink;
	u_int32_t	st_uid;
	u_int32_t	st_gid;
	u_int32_t	st_rdev;
	u_int32_t	st_atime;
	u_int32_t	st_atimensec;
	u_int32_t	st_mtime;
	u_int32_t	st_mtimensec;
	u_int32_t	st_ctime;
	u_int32_t	st_ctimensec;
	u_int64_t	st_size;
	u_int64_t	st_blocks;
	u_int32_t	st_blksize;
	u_int32_t	st_flags;
	u_int32_t	st_gen;
	u_int32_t	st_lspare;
	u_int64_t	st_qspare[2];
      };



int openbsd_aout__stat (ret_t *res, struct proc *p, char *name,
	struct openbsd_stat *os)
  {
    struct stat ss;
    int ret = 0;
    size_t blen, slen;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, os, sizeof(struct
		openbsd_stat), VMREGION_WRITABLE))
	  return EFAULT;

	blen = vm_prot_maxmemrange (p, name, VMREGION_READABLE);
	if (blen==0)
	  return EFAULT;

	slen = vm_prot_stringlen (p, name, blen);
	if (slen==0)
	  return ENOENT;

	p->sc_params_ok = 1;
      }

    os = (struct openbsd_stat *) ((int)os + userland_startaddr);
    memset (os, 0, sizeof(struct openbsd_stat));

    ret = sys_stat (res, p, name, (struct stat *)
		((size_t)(&ss) - userland_startaddr));
    if (ret)
      return ret;

    os->st_dev       = ss.st_dev;
    os->st_ino       = ss.st_ino;
    os->st_mode      = ss.st_mode;
    os->st_nlink     = ss.st_nlink;
    os->st_uid       = ss.st_uid;
    os->st_gid       = ss.st_gid;
    os->st_rdev      = (u_int32_t) ss.st_rdev;
    os->st_atime     = ss.st_atime;
    os->st_atimensec = ss.st_atimensec;
    os->st_mtime     = ss.st_mtime;
    os->st_mtimensec = ss.st_mtimensec;
    os->st_ctime     = ss.st_ctime;
    os->st_ctimensec = ss.st_ctimensec;
    os->st_size      = ss.st_size;
    os->st_blocks    = ss.st_blocks;
    os->st_blksize   = ss.st_blksize;
    os->st_flags     = ss.st_flags;
    os->st_gen       = ss.st_gen;

    return 0;
  }



int openbsd_aout__fstat (ret_t *res, struct proc *p, int d,
	struct openbsd_stat *os)
  {
    struct stat ss;
    int ret = 0;

    if (!p->sc_params_ok)
      {
	if (!vm_prot_accessible (p, os, sizeof(struct
		openbsd_stat), VMREGION_WRITABLE))
	  return EFAULT;

	p->sc_params_ok = 1;
      }

    os = (struct openbsd_stat *) ((int)os + userland_startaddr);
    memset (os, 0, sizeof(struct openbsd_stat));

    ret = sys_fstat (res, p, d, (struct stat *)
		((int)(&ss) - userland_startaddr));
    if (ret)
      return ret;

    os->st_dev       = ss.st_dev;
    os->st_ino       = ss.st_ino;
    os->st_mode      = ss.st_mode;
    os->st_nlink     = ss.st_nlink;
    os->st_uid       = ss.st_uid;
    os->st_gid       = ss.st_gid;
    os->st_rdev      = (u_int32_t) ss.st_rdev;
    os->st_atime     = ss.st_atime;
    os->st_atimensec = ss.st_atimensec;
    os->st_mtime     = ss.st_mtime;
    os->st_mtimensec = ss.st_mtimensec;
    os->st_ctime     = ss.st_ctime;
    os->st_ctimensec = ss.st_ctimensec;
    os->st_size      = ss.st_size;
    os->st_blocks    = ss.st_blocks;
    os->st_blksize   = ss.st_blksize;
    os->st_flags     = ss.st_flags;
    os->st_gen       = ss.st_gen;

    return 0;
  }



int openbsd_aout___sysctl (ret_t *res, struct proc *p,
	int *name, unsigned int namelen, void *oldp, size_t *oldlenp,
	void *newp, size_t newlen)
  {
    int i;
    int retint, *intp;
    char *retstr = NULL;

    if (!res || !p)
	return EINVAL;

    if (newp)
	return EINVAL;

    /*  TODO:  check memory ranges etc!  */

    name    = (int *) ((size_t)name + userland_startaddr);
    oldp    = (void *) ((size_t)oldp + userland_startaddr);
    oldlenp = (size_t *) ((size_t)oldlenp + userland_startaddr);
    newp    = (void *) ((size_t)newp + userland_startaddr);

    if (namelen < 2)
	return EINVAL;

/*  TODO:  this is not a very good way of handling sysctl calls, but
	at least it does something:  */

    retstr = NULL;

    if (name[0]==1 && name[1]==1)
	retstr = OS_NAME;

    if (name[0]==1 && name[1]==2)
	retstr = OS_VERSION;

    if (name[0]==1 && name[1]==4)
	retstr = compile_info;

    if (name[0]==1 && name[1]==10)
	retstr = nodename;

    if (name[0]==1 && name[1]==27)
	retstr = compile_generation;

    if (name[0]==6 && name[1]==1)
	retstr = OS_ARCH;

    if (name[0]==6 && name[1]==7)
      {
	retint = PAGESIZE;
	intp = (int *) oldp;
	if (*oldlenp < sizeof(retint))
	  return ENOMEM;
	*intp = retint;
	return 0;
      }

    if (retstr)
      {
	strlcpy (oldp, retstr, *oldlenp);
	if (*oldlenp < strlen(retstr)+1)
	  return ENOMEM;
	return 0;
      }

    printk ("openbsd_aout__sysctl, unimplemented name:");
    for (i=0; i<namelen; i++)
      {
	printk ("  name[%i] = %i", i, name[i]);
      }

    return EOPNOTSUPP;
  }



int openbsd_aout__nanosleep (ret_t *res, struct proc *p,
	struct openbsd_aout__timespec *oa_rqtp,
	struct openbsd_aout__timespec *oa_rmtp)
  {
    struct timespec rqtp;
    struct timespec rmtp;
    struct timespec *rqtp_ptr;
    struct timespec *rmtp_ptr;
    int retv;

    if (!res || !p)
	return EINVAL;

    /*  TODO: check arguments!  */

    oa_rqtp = (struct openbsd_aout__timespec *) ((byte *)oa_rqtp+userland_startaddr);
    if (oa_rmtp)
	oa_rmtp = (struct openbsd_aout__timespec *) ((byte *)oa_rmtp+userland_startaddr);

    rqtp.tv_sec = oa_rqtp->tv_sec;
    rqtp.tv_nsec = oa_rqtp->tv_nsec;

    rqtp_ptr = (struct timespec *) ((byte *)&rqtp - userland_startaddr);
    rmtp_ptr = (struct timespec *) ((byte *)&rmtp - userland_startaddr);

    retv = sys_nanosleep (res, p, (const struct timespec *) rqtp_ptr, rmtp_ptr);

    if (oa_rmtp)
	oa_rmtp->tv_sec = rmtp.tv_sec;
	oa_rmtp->tv_nsec = rmtp.tv_nsec;

    return retv;
  }



int openbsd_aout__gettimeofday (ret_t *res, struct proc *p,
	struct openbsd_aout__timeval *oa_tvp, void *timezonep)
  {
    struct timeval tv;
    int retv;
    int return_tp=1, return_tz=1;

    if (!res || !p)
      return EINVAL;

    if (!oa_tvp)
      return_tp = 0;

    if (!timezonep)
      return_tz = 0;

    if (!p->sc_params_ok)
      {
	if (return_tp)
	  if (!vm_prot_accessible (p, oa_tvp, sizeof(struct
		openbsd_aout__timeval), VMREGION_WRITABLE))
	    return EFAULT;

/*  TODO: timezone size!!! */

/*
        if (return_tz)
          if (!vm_prot_accessible (p, timezoneptr, sizeof(int),
                                VMREGION_WRITABLE))
            return EFAULT;
  */

	p->sc_params_ok = 1;
      }

    oa_tvp = (struct openbsd_aout__timeval *)
		((byte *)oa_tvp + userland_startaddr);

    retv = sys_gettimeofday (res, p, (struct timeval *)
		((byte *)&tv - userland_startaddr), NULL);

    if (return_tp)
	oa_tvp->tv_sec = tv.tv_sec;
	oa_tvp->tv_usec = tv.tv_usec;

    return retv;
  }



int openbsd_aout___syscall (s_int64_t *res, struct proc *p,
	s_int64_t nr, int p1, int p2, int p3, int p4, int p5, int p6,
	int p7, int p8, int p9, int p10, int p11, int p12)
  {
    /*
     *	Quad syscall (64-bit return value).
     */

    int (*func)();

    if (nr<0 || nr>=NR_OF_SYSCALLS)
	return EINVAL;

    func = openbsd_aout_emul->syscall[nr];
    if (!func)
      {
	printk ("TODO: openbsd_aout quad syscall nr=%i", (int)nr);
	return ENOSYS;
      }

    return func (res, p, p1,p2,p3,p4,p5,p6,p7,p8,p9,p10,p11,p12);
  }



int openbsd_aout__mmap (s_int64_t *res, struct proc *p,
	void *addr, size_t len, int prot, int flags, int fd,
	long PAD, off_t pos)
  {
    /*
     *	OpenBSD has a 32-bit "pad". (Weird.)
     */

    return sys_mmap (res, p, addr, len, prot, flags, fd, pos);
  }



int openbsd_aout__lseek (s_int64_t *res, struct proc *p,
	int fd, int pad, off_t offset, int whence)
  {
    /*
     *	OpenBSD has a 32-bit "pad" between the fd and the offset.
     *	(Weird.)
     */

    return sys_lseek (res, p, fd, offset, whence);
  }



int openbsd_aout__sigprocmask (ret_t *res, struct proc *p, int how, sigset_t set)
  {
    /*
     *	Internally, OpenBSD receives the new set as a sigset_t, not as
     *	a pointer to it which is the way sigprocmask(2) wants it. Yoctix
     *	implements sigprocmask() using pointers to sigset_t's.
     *
     *	OpenBSD also returns the old set via res, not by writing to userland.
     */

    sigset_t *sptr, *soptr;
    sigset_t oset;
    int er;

    sptr = (sigset_t *) ((byte *)(&set) - userland_startaddr);
    soptr = (sigset_t *) ((byte *)(&oset) - userland_startaddr);

    p->sc_params_ok = 1;

    er = sys_sigprocmask (res, p, how, sptr, soptr);
    if (er)
	return er;

    *res = oset;
    return 0;
  }



int openbsd_aout__break (ret_t *res, struct proc *p, char *addr)
  {
    /*
     *	OpenBSD actually returns 0 for the break(2) syscall internally,
     *	and it is then up to the OpenBSD libc to convert the 0 to addr
     *	(unless there was an error, of course).
     */

    ret_t tmpres;

    *res = 0;
    return sys_break (&tmpres, p, addr);
  }



void openbsd_aout_syscalls_init (void **s)
  {
    /*
     *	OpenBSD a.out syscalls
     *	----------------------
     *
     *	These entries point to functions handling the system calls. In a few
     *	cases, special conversion or tweaks are neccessary to translate
     *	between the internal Yoctix representation of structs and the way
     *	OpenBSD binaries expect things to be.
     */

    s [  1] = sys_exit;
    s [  2] = sys_fork;
    s [  3] = sys_read;
    s [  4] = sys_write;
    s [  5] = sys_open;
    s [  6] = sys_close;
    s [  7] = sys_wait4;
    /*   8    sys_ocreat	(4.3BSD)  */
    /*   9    sys_link;  */
    /*  10    sys_unlink  */
    /*  11    sys_execv		(obsolete)  */
    s [ 12] = sys_chdir;
    s [ 13] = sys_fchdir;
    /*  14    sys_mknod  */
    /*  15    sys_chmod  */
    /*  16    sys_chown  */
    s [ 17] = openbsd_aout__break;
    /*  18    sys_ogetfsstat  */
    /*  19    sys_olseek	(4.3BSD)  */
    s [ 20] = sys_getpid;
    s [ 21] = sys_mount;
    /*  22    sys_umount  */
    s [ 23] = sys_setuid;
    s [ 24] = sys_getuid;
    s [ 25] = sys_geteuid;

    s [ 33] = sys_access;

    /*  36    sys_sync  */
    s [ 37] = sys_kill;

    s [ 39] = sys_getppid;

    s [ 43] = sys_getegid;

    s [ 46] = sys_sigaction;
    s [ 47] = sys_getgid;
    s [ 48] = openbsd_aout__sigprocmask;

    s [ 54] = sys_ioctl;
    s [ 55] = sys_reboot;
    /*  56    sys_revoke  */
    /*  57    sys_symlink  */
    s [ 58] = sys_readlink;
    s [ 59] = sys_execve;
    s [ 60] = sys_umask;
    s [ 61] = sys_chroot;

    s [ 73] = sys_munmap;
    s [ 74] = sys_mprotect;
    /*  75    sys_madvice  */

    /*  78    sys_mincore  */
    /*  79    sys_getgroups  */
    /*  80    sys_setgroups  */

    s [ 81] = sys_getpgrp;
    s [ 82] = sys_setpgid;

    /*  83    sys_setitimer  */

    /*  85    sys_swapon  */
    /*  86    sys_getitimer  */

    s [ 90] = sys_dup2;

    s [ 92] = sys_fcntl;
    /*  93    sys_select  */

    /*  95    sys_fsync  */
    /*  96    sys_setpriority  */
    /*  97    sys_socket  */
    /*  98    sys_connect  */

    /* 100    sys_getpriority  */

    s [103] = sys_sigreturn;
    /* 104    sys_bind  */
    /* 105    sys_setsockopt  */
    /* 106    sys_listen  */

    s [111] = sys_sigsuspend;

    s [116] = openbsd_aout__gettimeofday;

    /* 120    sys_readv  */
    s [121] = sys_writev;
    /* 122    sys_settimeofday  */
    /* 123    sys_fchown  */
    /* 124    sys_fchmod  */

    /* 128    sys_rename  */

    /* 136    sys_mkdir  */
    /* 137    sys_rmdir  */

    s [181] = sys_setgid;
    s [182] = sys_setegid;
    s [183] = sys_seteuid;

    s [188] = openbsd_aout__stat;
    s [189] = openbsd_aout__fstat;
    s [190] = openbsd_aout__stat;		/*  TODO: actually implement lstat!!!  */

    s [196] = sys_getdirentries;
    s [197] = openbsd_aout__mmap;
    s [198] = openbsd_aout___syscall;
    s [199] = openbsd_aout__lseek;

    s [202] = openbsd_aout___sysctl;

    s [240] = openbsd_aout__nanosleep;

    s [253] = sys_issetugid;

    s [262] = sys_fstatfs;
    s [263] = sys_pipe;
  }

