/*
 *  Copyright (C) 2000 by Anders Gavare.  All rights reserved.
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
 *  vfs/vfs_stat.c  --  get stat data for a file
 *
 *	vfs_stat ()
 *
 *  History:
 *	5 Feb 2000	first version
 *	26 Jul 2000	beginning a complete rewrite
 */


#include "../config.h"
#include <string.h>
#include <sys/malloc.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/vfs.h>
#include <sys/std.h>
#include <sys/proc.h>
#include <sys/limits.h>



extern struct mountinstance *firstmountinstance;



int vfs_stat (struct proc *p, char *filename, struct mountinstance **mi,
	struct stat *ss)
  {
    /*
     *	vfs_stat ()
     *	-----------
     *
     *	Returns 0 on success, errno on failure.
     */

    char *fname;
    inode_t current_inode;
    struct mountinstance *current_mi, *tmp_mi;
    int res;
    int do_loop;
    int i_fname;		/*  Current index into fname  */
    int i_tmp, last_inode_was_nondir;
    char tmpchar;

    if (!p || !filename || !mi || !ss)
	return EINVAL;

    if (filename[0]=='\0')
	return ENOENT;

    res = strlen(filename)+2;
    fname = (char *) malloc (res);
    if (!fname)
	return ENOMEM;

    memcpy (fname, filename, res);

#if DEBUGLEVEL>=4
    printk ("vfs_stat: fname='%s' rootdir=0x%x cwd=0x%x", fname,
	p->rootdir_vnode, p->currentdir_vnode);
#endif

    /*  No current dir? Then assume currentdir = rootdir  */
    if (!p->currentdir_vnode && p->rootdir_vnode)
      {
	p->currentdir_vnode = p->rootdir_vnode;
	p->rootdir_vnode->refcount ++;
      }

    current_inode = ROOT_INODE;
    current_mi = firstmountinstance;
    if (!current_mi)
      panic ("vfs_stat: no root mountinstance");

    /*  Start at the root directory, or at the current directory?  */
    if (fname[0]=='/')
      {
	if (p->rootdir_vnode)
	  {
	    current_inode = p->rootdir_vnode->ss.st_ino;
	    current_mi    = p->rootdir_vnode->mi;
	  }
      }
    else
      {
	if (p->currentdir_vnode)
	  {
	    current_inode = p->currentdir_vnode->ss.st_ino;
	    current_mi    = p->currentdir_vnode->mi;
	  }
      }

    if (current_inode == ROOT_INODE)
      current_inode = current_mi->superblock->root_inode;

    i_fname = 0;
    while (fname[i_fname]=='/')
	i_fname ++;

    if (fname[i_fname]==0)
      {
	res = current_mi->fs->istat (current_mi,
		(inode_t) current_mi->superblock->root_inode, ss, p);
	free (fname);
	*mi = current_mi;
	return res;
      }


    /*  TODO: should "/bin/" equal "/bin" or "/bin/." ?
	BSD/sysv behavior???  This work as "/bin/." for now...  */

    if (fname[strlen(fname)-1]=='/')
      strlcpy (fname+strlen(fname), ".", 2);


    /*  Stat loop:  */
    last_inode_was_nondir = 0;
    do_loop = 1;

    while (do_loop)
      {
	if (last_inode_was_nondir)
	  {
	    free (fname);
	    return ENOTDIR;
	  }

	/*
	 *  See if the directory we're about to scan (using namestat())
	 *  actually has another mountinstance mounted there. If so, then
	 *  we switch to that mountinstance's root directory.
	 *
	 *  TODO:  This will become awfully slow if there are many
	 *	mountinstances... time increases linearly...
	 */
	tmp_mi = firstmountinstance;
	while (tmp_mi)
	  {
	    if (tmp_mi->parent == current_mi &&
		tmp_mi->mounted_at->ss.st_ino == current_inode)
	      {
		current_mi = tmp_mi;
		current_inode = tmp_mi->superblock->root_inode;
		tmp_mi=NULL;
	      }
	    else
	      tmp_mi=tmp_mi->next;
	  }

	/*
	 *  Remove any heading '/' characters:
	 */
	while (fname[i_fname]=='/')
		i_fname ++;

	/*
	 *  Isolate the first part of fname, starting at index i_fname.
	 */
	i_tmp = i_fname;
	while (fname[i_tmp] && fname[i_tmp]!='/')
		i_tmp ++;

	tmpchar = fname[i_tmp];
	fname[i_tmp] = 0;

	/*
	 *  Now, there is a (sub)string at fname+i_fname containing no slashes.
	 *  TODO: the strlen could be cached in a variable...
	 *
	 *  Stat it using the mountinstance' filesystem's namestat() function.
	 */

	res = current_mi->fs->namestat (current_mi, current_inode,
		fname+i_fname, ss, p);

#if DEBUGLEVEL>=4
	printk ("  stat(): find{'%s' in directory %i}=%i, newinode=%i",
		fname+i_fname, (int)current_inode, res, (int)ss->st_ino);
#endif

	/*
	 *  If the file was not found, or there was some other error, then we
	 *  return at once.
	 */
	if (res)
	  {
	    free (fname);
	    return res;
	  }

	/*
	 *  Go to ".." beyond this mountinstance?
	 */
	if (current_inode == current_mi->superblock->root_inode
		&& !strcmp(fname+i_fname, "..") && current_mi->parent)
	  {
	    current_inode = current_mi->parent_inode;
	    current_mi = current_mi->parent;

	    res = current_mi->fs->namestat (current_mi, current_inode, ".", ss, p);
	    if (res)
	      {
		free (fname);
		return res;
	      }
	  }

	/*
	 *  Otherwise, if the file we stat:ed was a directory,
	 *  then we set current_inode to the stat:ed directory's inode to
	 *  iterate deeper into the filesystem tree.
	 */

	if ((ss->st_mode & 0170000) == S_IFDIR)
	  current_inode = ss->st_ino;
	else
	  last_inode_was_nondir = 1;

	/*
	 *  Undo any changes to fname.
	 */
	fname[i_tmp] = tmpchar;
	i_fname = i_tmp;

	if (!fname[i_fname])
	  do_loop = 0;
      }


    /*
     *	If we're stating a directory, say "/proc", we actually want to
     *	return data from "/proc/." instead.
     */

    if ((ss->st_mode & 0170000) == S_IFDIR)
      {
	tmp_mi = firstmountinstance;
	while (tmp_mi)
	  {
	    if (tmp_mi->parent == current_mi && tmp_mi->mounted_at &&
		tmp_mi->mounted_at->ss.st_ino == ss->st_ino)
	      {
printk ("!!!!!! fname='%s'", fname);
		free (fname);

		current_mi = tmp_mi;
		*mi = current_mi;

		res = current_mi->fs->istat (current_mi,
		    (inode_t) current_mi->superblock->root_inode, ss, p);
		return res;
	      }
	    else
	      tmp_mi=tmp_mi->next;
	  }
      }


    *mi = current_mi;
    free (fname);
    return 0;
  }

