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

#if 0
/* A global variable is a bit ugly, but it keeps the code simple */
int sysctl_drop_caches;

static void drop_pagecache_sb(struct super_block *sb, void *unused)
{
	struct inode *inode, *toput_inode = NULL;

	spin_lock(&inode_sb_list_lock);
	list_for_each_entry(inode, &sb->s_inodes, i_sb_list) {
		spin_lock(&inode->i_lock);
		if ((inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) ||
		    (inode->i_mapping->nrpages == 0)) {
			spin_unlock(&inode->i_lock);
			continue;
		}
		__iget(inode);
		spin_unlock(&inode->i_lock);
		spin_unlock(&inode_sb_list_lock);
		invalidate_mapping_pages(inode->i_mapping, 0, -1);
		iput(toput_inode);
		toput_inode = inode;
		spin_lock(&inode_sb_list_lock);
	}
	spin_unlock(&inode_sb_list_lock);
	iput(toput_inode);
}

static void drop_slab(void)
{
	int nr_objects;
	struct shrink_control shrink = {
		.gfp_mask = GFP_KERNEL,
	};

	nodes_setall(shrink.nodes_to_scan);
	do {
		nr_objects = shrink_slab(&shrink, 1000, 1000);
	} while (nr_objects > 10);
}

int drop_caches_sysctl_handler(struct ctl_table *table, int write,
	void __user *buffer, size_t *length, loff_t *ppos)
{
	int ret;

	ret = proc_dointvec_minmax(table, write, buffer, length, ppos);
	if (ret)
		return ret;
	if (write) {
		static int stfu;

		if (sysctl_drop_caches & 1) {
			iterate_supers(drop_pagecache_sb, NULL);
			count_vm_event(DROP_PAGECACHE);
		}
		if (sysctl_drop_caches & 2) {
			drop_slab();
			count_vm_event(DROP_SLAB);
		}
		if (!stfu) {
			pr_info("%s (%d): drop_caches: %d\n",
				current->comm, task_pid_nr(current),
				sysctl_drop_caches);
		}
		stfu |= sysctl_drop_caches & 4;
	}
	return 0;
}

#endif

/* static char scdrop_path[PATH_MAX]; */

static int scdrop_pagecache(const char * __user filename)
{
	unsigned int lookup_flags = LOOKUP_FOLLOW;
	struct path path;
	int error;

	printk(KERN_INFO "WAH %s %d\n", __FUNCTION__, __LINE__);
retry:
	error = user_path_at(AT_FDCWD, filename, lookup_flags, &path);
	printk(KERN_INFO "WAH %s %d err %d\n", __FUNCTION__, __LINE__, error);
	if (!error) {
		/* clean */
		struct inode *inode = path.dentry->d_inode;
		printk(KERN_INFO "WAH %s %d\n", __FUNCTION__, __LINE__);
		spin_lock(&inode->i_lock);
		if ((inode->i_state & (I_FREEING|I_WILL_FREE|I_NEW)) ||
		    (inode->i_mapping->nrpages == 0)) {
			printk(KERN_INFO "WAH %s %d\n", __FUNCTION__, __LINE__);
			spin_unlock(&inode->i_lock);
			return 0;
		}
		ihold(inode);
		spin_unlock(&inode->i_lock);
		invalidate_mapping_pages(inode->i_mapping, 0, -1);
		iput(inode);
	}
	printk(KERN_INFO "WAH %s %d\n", __FUNCTION__, __LINE__);
	if (retry_estale(error, lookup_flags)) {
		printk(KERN_INFO "WAH %s %d", __FUNCTION__, __LINE__);
		lookup_flags |= LOOKUP_REVAL;
		goto retry;
	}
	printk(KERN_INFO "WAH %s %d\n", __FUNCTION__, __LINE__);
	return error;
}

static int scdrop_ctl_handler(struct ctl_table *table, int write,
			      void __user *buffer, size_t *lenp, loff_t *ppos)
{
	/* int ret; */

	char __user *pathname = buffer + *lenp - 1;

	put_user('\0', pathname);

	/* if ((ret = proc_dostring(table, write, buffer, lenp, ppos))) { */
	/* 	printk(KERN_INFO "%s: Failed to fetch file path\n", */
	/* 	       __FUNCTION__); */
	/* 	return ret; */
	/* } */

	if (!write)
		return 0;

	/* printk(KERN_INFO "%s: Got this name: \"%s\"\n", */
	/*        __FUNCTION__, scdrop_path); */

	return scdrop_pagecache(buffer);
}

static struct ctl_path vm_path[] = { { .procname = "vm", }, { } };
static struct ctl_table scdrop_ctl_table[] = {
	{
		.procname = "sdrop_caches",
		/* .data = scdrop_path, */
		/* .maxlen = PATH_MAX, */
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
