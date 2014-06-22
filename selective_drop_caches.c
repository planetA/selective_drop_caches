/*
 * Implement the manual selective drop pagecache function
 */

#include <linux/module.h>


#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/writeback.h>
#include <linux/sysctl.h>
#include <linux/gfp.h>
#include <linux/limits.h>
#include <linux/namei.h>

/* Defines the license for this LKM */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Selective pagecache drop module");
MODULE_AUTHOR("Maksym Planeta");

static void clean_mapping(struct dentry *dentry)
{
	struct inode *inode = dentry->d_inode;

	if (!inode)
		return;

	if ((inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) ||
	    (inode->i_mapping->nrpages == 0)) {
		return;
	}

	invalidate_mapping_pages(inode->i_mapping, 0, -1);
}

static void clean_all_dentries_locked(struct dentry *dentry)
{
	struct dentry *child;

	list_for_each_entry(child, &dentry->d_subdirs, d_u.d_child) {
		clean_all_dentries_locked(child);
	}

	clean_mapping(dentry);
}

static void clean_all_dentries(struct dentry *dentry)
{
	spin_lock_nested(&dentry->d_lock, DENTRY_D_LOCK_NESTED);
	clean_all_dentries_locked(dentry);
	spin_unlock(&dentry->d_lock);
}

static int scdrop_pagecache(const char * __user filename)
{
	unsigned int lookup_flags = LOOKUP_FOLLOW;
	struct path path;
	int error;

retry:
	error = user_path_at(AT_FDCWD, filename, lookup_flags, &path);
	if (!error) {
		/* clean */
		clean_all_dentries(path.dentry);
	}
	if (retry_estale(error, lookup_flags)) {
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
	return error;
}

static int scdrop_ctl_handler(struct ctl_table *table, int write,
			      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	char __user *pathname = buffer + *lenp - 1;

	put_user('\0', pathname);

	if (!write)
		return 0;

	return scdrop_pagecache(buffer);
}

static struct ctl_path vm_path[] = { { .procname = "vm", }, { } };
static struct ctl_table scdrop_ctl_table[] = {
	{
		.procname = "sdrop_caches",
		.mode = 0644,
		.proc_handler = scdrop_ctl_handler,
	},
	{ }
};

static struct ctl_table_header *scdrop_proc_entry;

/* Init function called on module entry */
int scdrop_init(void)
{
	int ret = 0;

	scdrop_proc_entry = register_sysctl_paths(vm_path, scdrop_ctl_table);

	if (scdrop_proc_entry == NULL) {
		ret = -ENOMEM;
		printk(KERN_INFO "%s: Couldn't create proc entry\n",
		       __FUNCTION__);
	} else {
		printk(KERN_INFO "%s: Module loaded.\n", __FUNCTION__);
	}

	return ret;
}

/* Cleanup function called on module exit */
void scdrop_cleanup(void)
{
	unregister_sysctl_table(scdrop_proc_entry);
	printk(KERN_INFO "%s called.  Module is now unloaded.\n", __FUNCTION__);
	return;
}

/* Declare entry and exit functions */
module_init(scdrop_init);
module_exit(scdrop_cleanup);
