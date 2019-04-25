#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/version.h>

#include "vfs_change_consts.h"
#include "vfs_change_uapi.h"
#include "vfs_change.h"

#ifndef MAX_VFS_CHANGE_MEM
#define MAX_VFS_CHANGE_MEM	(1<<20)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
#define TIMESTRUCT timeval
#else
#define TIMESTRUCT timespec64
#endif

typedef struct __vfs_change__ {
	struct TIMESTRUCT ts;
	unsigned int size;
	char *path;


	struct list_head list;
} vfs_change;

#define REMOVE_ENTRY(p, vc) {\
	list_del(p);\
	total_memory -= strlen(vc->path);\
	kfree(vc);\
	cur_changes--;\
}

//static const char* action_names[] = {"file_write",};

static LIST_HEAD(vfs_changes);
static LIST_HEAD(monitor_path);
static int discarded = 0, total_changes = 0, cur_changes = 0, total_memory = 0;
static DEFINE_SPINLOCK(sl_changes);

static wait_queue_head_t wq_vfs_changes;
static atomic_t wait_vfs_changes_count;
static atomic_t vfs_changes_is_open;

static void wait_vfs_changes_timer_callback(
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
		unsigned long data
#else
		struct timer_list *t
#endif
		)
{
	atomic_set(&wait_vfs_changes_count, 0);
	wake_up_interruptible(&wq_vfs_changes);
}


#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
static DEFINE_TIMER(wait_vfs_changes_timer, wait_vfs_changes_timer_callback, 0, 0);
#else
static DEFINE_TIMER(wait_vfs_changes_timer, wait_vfs_changes_timer_callback);
#endif

static int open_vfs_changes(struct inode* si, struct file* filp)
{
	if (atomic_cmpxchg(&vfs_changes_is_open, 0, 1) == 1) {
		return -EBUSY;
	}

	struct TIMESTRUCT* tv = kzalloc(sizeof(struct TIMESTRUCT), GFP_KERNEL);
	if (unlikely(tv == 0)) {
		atomic_set(&vfs_changes_is_open, 0);
		return -ENOMEM;
	}

	filp->private_data = tv;
	return 0;
}

static int release_vfs_changes(struct inode* si, struct file* filp)
{
	kfree(filp->private_data);
	atomic_set(&vfs_changes_is_open, 0);
	atomic_set(&wait_vfs_changes_count, -1);;
	del_timer(&wait_vfs_changes_timer);

	return 0;
}

static int hour_shift = 8;
module_param(hour_shift, int, 0644);
MODULE_PARM_DESC(hour_shift, "hour shift for displaying /proc/vfs_changes line item time");

#define MIN_LINE_SIZE	50

static ssize_t copy_vfs_changes(struct TIMESTRUCT *last, char* buf, size_t size)
{
	size_t total = 0;
	vfs_change* vc;
	list_for_each_entry(vc, &vfs_changes, list) {
		char temp[MIN_LINE_SIZE];
		snprintf(temp, sizeof(temp), "changed: %s ",vc->path);

		memcpy(buf+total, temp, strlen(temp));
		total += strlen(temp);
		buf[total++] = '\n';
	}
	return total;
}

// give string format output for debugging purpose (cat /proc/vfs_changes)
// format: YYYY-MM-DD hh:mm:ss.SSS $action $src $dst, let's require at least 50 bytes
static ssize_t read_vfs_changes(struct file* filp, char __user* buf, size_t size, loff_t* offset)
{
	if (size < MIN_LINE_SIZE)
		return -EINVAL;

	char* kbuf = kmalloc(size, GFP_KERNEL);
	if (kbuf == 0)
		return -ENOMEM;

	struct TIMESTRUCT *last = (struct TIMESTRUCT*)filp->private_data;
	spin_lock(&sl_changes);
	ssize_t r = copy_vfs_changes(last, kbuf, size);
	spin_unlock(&sl_changes);
	if (r > 0 && copy_to_user(buf, kbuf, r))
		r = -EFAULT;

	kfree(kbuf);
	return r;
}

static long move_vfs_changes(ioctl_rd_args __user* ira)
{
	if (atomic_cmpxchg(&wait_vfs_changes_count, -1, INT_MAX) >= 0) {
		return -EBUSY;
	}

	ioctl_rd_args kira;
	if (copy_from_user(&kira, ira, sizeof(kira)) != 0) {
		atomic_set(&wait_vfs_changes_count, -1);
		return -EFAULT;
	}

	char *kbuf = kmalloc(kira.size, GFP_KERNEL);
	if (kbuf == 0) {
		atomic_set(&wait_vfs_changes_count, -1);
		return -ENOMEM;
	}

	int total_bytes = 0, total_items = 0;

	spin_lock(&sl_changes);
	struct list_head *p, *next;
	list_for_each_safe(p, next, &vfs_changes) {
		vfs_change* vc = list_entry(p, vfs_change, list);
		int this_len =strlen(vc->path) + 1;
		if (this_len + total_bytes > kira.size)
			break;

		// src and dst are adjacent when allocated
		memcpy(kbuf + total_bytes, vc->path, this_len);
		total_bytes += this_len;

		total_items++;
		REMOVE_ENTRY(p, vc);
	}
	spin_unlock(&sl_changes);

	if (total_bytes && copy_to_user(kira.data, kbuf, total_bytes)) {
		kfree(kbuf);
		atomic_set(&wait_vfs_changes_count, -1);
		return -EFAULT;
	}
	kfree(kbuf);

	kira.size = total_items;
	if (copy_to_user(ira, &kira, sizeof(kira)) != 0) {
		atomic_set(&wait_vfs_changes_count, -1);
		return -EFAULT;
	}

	atomic_set(&wait_vfs_changes_count, -1);

	return 0;
}

static long read_stats(ioctl_rs_args __user* irsa)
{
	ioctl_rs_args kirsa = {
		.total_changes = total_changes,
		.cur_changes = cur_changes,
		.discarded = discarded,
		.cur_memory = total_memory,
	};
	if (copy_to_user(irsa, &kirsa, sizeof(kirsa)) != 0)
		return -EFAULT;
	return 0;
}

static long wait_vfs_changes(ioctl_wd_args __user* ira)
{
	if (atomic_cmpxchg(&wait_vfs_changes_count, -1, 0) >= 0)
		return -EBUSY;

	ioctl_wd_args kira;
	if (copy_from_user(&kira, ira, sizeof(kira)) != 0)
		return -EFAULT;

	if(kira.condition_timeout<=0)
		return -EINVAL;


	atomic_set(&wait_vfs_changes_count, INT_MAX);

	mod_timer(&wait_vfs_changes_timer, jiffies + HZ * kira.condition_timeout / 1000);
	if(wait_event_interruptible(wq_vfs_changes,cur_changes>=atomic_read(&wait_vfs_changes_count)!=0))
			return -EAGAIN;

				//if (timer_pending(&wait_vfs_changes_timer) == 0)

	atomic_set(&wait_vfs_changes_count, -1);
	del_timer(&wait_vfs_changes_timer);

	return 0;
}
static long add_list_item(ioctl_item_args __user *iia){
	//TODO multi user??
	//ioctl_item_args kiia;
	//if(copy_from_user(&kiia,iia,sizeof(kiia))!=0){
	//	return -EFAULT;
	//}

	pr_info("try to alloc %d memory\n",iia->size);
	char *buff=kmalloc(iia->size,GFP_KERNEL);
	vfs_change *nitem=kmalloc(sizeof(vfs_change),GFP_KERNEL);
	pr_info("try copy from user\n");
	if(copy_from_user(buff,iia->data,iia->size)!=0){
		return -EFAULT;
	}
	pr_info("success copy\n");
	nitem->path=buff;
	list_add(&nitem->list,&monitor_path);
	pr_info("add:%s\n",nitem->path);
	return 0;

}
static long remve_list_item(ioctl_item_args __user *iia){
	struct list_head *p,*next;
	list_for_each_safe(p,next,&monitor_path){
		vfs_change *en=list_entry(p,vfs_change,list);
		//if(strcmp(en->path,kiia.data)==0){
		pr_info("delete %s\n",en->path);
			list_del(p);
			//break;
		//}
	}
	return 0;
}

static long ioctl_vfs_changes(struct file* filp, unsigned int cmd, unsigned long arg)
{
	switch(cmd) {
		case VC_IOCTL_READDATA:
			return move_vfs_changes((ioctl_rd_args*)arg);
		case VC_IOCTL_READSTAT:
			return read_stats((ioctl_rs_args*)arg);
		case VC_IOCTL_WAITDATA:
			return wait_vfs_changes((ioctl_wd_args*)arg);
		case VC_IOCTL_ADDITEM:
			return add_list_item((ioctl_item_args *)arg);
		case VC_IOCTL_DELETEITEM:
			return remve_list_item((ioctl_item_args *)arg);


		default:
			return -EINVAL;
	}
}

static struct file_operations procfs_ops = {
	.owner = THIS_MODULE,
	.open = open_vfs_changes,
	.read = read_vfs_changes,
	.unlocked_ioctl = ioctl_vfs_changes,
	.llseek = no_llseek,
	//.llseek = generic_file_llseek,
	.release = release_vfs_changes,
};

int __init init_vfs_changes(void)
{
	struct proc_dir_entry* procfs_entry = proc_create(PROCFS_NAME, 0666, 0, &procfs_ops);
	if (procfs_entry == 0) {
		pr_warn("%s already exists?\n", PROCFS_NAME);
		return -1;
	}

	init_waitqueue_head(&wq_vfs_changes);
	atomic_set(&vfs_changes_is_open, 0);
	atomic_set(&wait_vfs_changes_count, -1);

	return 0;
}

// c_v_c is also called when doing rollback in module-init phase, so it cant be marked as __exit
void cleanup_vfs_changes(void)
{
	remove_proc_entry(PROCFS_NAME, 0);
	// remove all dynamically allocated memory
	struct list_head *p, *next;
	spin_lock(&sl_changes);
	list_for_each_safe(p, next, &vfs_changes) {
		vfs_change* vc = list_entry(p, vfs_change, list);
		REMOVE_ENTRY(p, vc);
	}
	spin_unlock(&sl_changes);
}

static void remove_oldest(void)
{
	spin_lock(&sl_changes);
	struct list_head *p, *next;
	while (total_memory > MAX_VFS_CHANGE_MEM && !list_empty(&vfs_changes)) {
		list_for_each_safe(p, next, &vfs_changes) {
			vfs_change* vc = list_entry(p, vfs_change, list);
			// if (printk_ratelimit())
			 	pr_warn("vfs-change discarded:%s\n",vc->path);
			REMOVE_ENTRY(p, vc);
			discarded++;
			break;
		}
	}
	spin_unlock(&sl_changes);
}


typedef int (*merge_action_fn)(struct list_head* entry, vfs_change* cur);

static int merge_actions = 1;
module_param(merge_actions, int, 0664);
MODULE_PARM_DESC(merge_actions, "merge actions to minimize data transfer and fs tree change");


void vfs_changed(int act, const char* root, const char* src, const char* dst)
{
	remove_oldest();
	vfs_change *p = kmalloc(sizeof(vfs_change), GFP_ATOMIC);
	if (unlikely(p == 0)) {
		pr_info("vfs_changed_1: src: %s, dst: %s, proc: %s[%d]\n",
				src, dst, current->comm, current->pid);
		return;
	}

	vfs_change *item;
	spin_lock(&sl_changes);
	list_for_each_entry(item,&monitor_path,list){
		int l1=strlen(item->path);
		int l2=strlen(src)-strlen(dst);
		pr_info("compare %s with %s\n",src,item->path);
		if(l2>=l1&&strncmp(item->path,src,l1)==0){
		pr_info("get this:%s\n",src);
      		p->path=kmalloc(strlen(src),GFP_ATOMIC);
      		strcpy(p->path,src);
      		list_add_tail(&p->list,&vfs_changes);
      		total_changes++;
      		cur_changes++;
      		total_memory += strlen(p->path);

			break;
		}
	}
	spin_unlock(&sl_changes);


		vfs_change* vc = (vfs_change*)p;
#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 0, 0)
	do_gettimeofday(&vc->ts);
#else
	ktime_get_real_ts64(&vc->ts);
#endif
	int wvcc = atomic_read(&wait_vfs_changes_count);
	if (wvcc > 0 && cur_changes >= wvcc) {
		wake_up_interruptible(&wq_vfs_changes);
	}
}

