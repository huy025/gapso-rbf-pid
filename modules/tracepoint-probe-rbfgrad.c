/*
 * tracepoint-probe-rbfgrad.c
 *
 * rbfgrad tracepoint probes.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/dcache.h>
#include <linux/mutex.h>
#include "tp-rbfgrad-trace.h"
/*qjl proc*/
#include <linux/proc_fs.h>
/*qjl kfifo*/
#include <linux/kfifo.h>

/*qjl proc*/
static struct proc_dir_entry *proc_entry;/*qjl proc*/
/* name of the proc entry */
#define	PROC_FIFO	"rbfgrad"

/*qjl kfifo*/
static struct kfifo fifo;
#define BUF_SIZE (128*1024)
#define MAX_TO_GET 10000 
/* lock for procfs read access */
static DEFINE_MUTEX(read_lock);
/* lock for procfs write access */
static DEFINE_MUTEX(write_lock);

struct tc_rbfgrad_qopt ctl_init = {
	.sampl_period = 0, 
	.q_ref = 0, 
	.p_max = 0, 
	.eta_p = 0, 
	.eta_i = 0, 
	.eta_d = 0, 
	.n = 0, 
	.m = 0, 
	.alpha = 0, 
	.eta = 0
};
struct tc_rbfgrad_qopt ctl_temp;

/*
 * Here the caller only guarantees locking for struct file and struct inode.
 * Locking must therefore be done in the probe to use the dentry.
 */
static void probe_rbfgrad_output(void *ignore,
			       struct tc_rbfgrad_qopt *ctl)
{
	unsigned long u;
	struct tc_rbfgrad_qopt ctl_temp = *ctl;

	printk(KERN_INFO "Ok,we are here\n");
	printk(KERN_INFO "sampl_period:%d  q_ref:%d  p_max:%e  eta_p:%e  eta_i:%e  eta_d:%e  n:%d  m:%d  alpha:%e  eta:%e\n",ctl->sampl_period,ctl->q_ref,ctl->p_max,ctl->eta_p,ctl->eta_i,ctl->eta_d,ctl->n,ctl->m,ctl->alpha,ctl->eta);
	printk(KERN_INFO "The following output is in hexadecimal, please converting them into decimal by using perl\n");
	u = (unsigned long)&(ctl->p_max);
	printk(KERN_INFO "p_max:%lx\n", u);
	u = (unsigned long)&(ctl->eta_p);
	printk(KERN_INFO "eta_p:%lx\n", u);
	u = (unsigned long)&(ctl->eta_i);
	printk(KERN_INFO "eta_i:%lx\n", u);
	u = (unsigned long)&(ctl->eta_d);
	printk(KERN_INFO "eta_d:%lx\n", u);
	u = (unsigned long)&(ctl->alpha);
	printk(KERN_INFO "alpha:%lx\n", u);
	u = (unsigned long)&(ctl->eta);
	printk(KERN_INFO "eta:%lx\n", u);


	ctl_temp.sampl_period = ctl->sampl_period;
	ctl_temp.q_ref = ctl->q_ref;
	ctl_temp.p_max = ctl->p_max;
	ctl_temp.eta_p = ctl->eta_p;
	ctl_temp.eta_i = ctl->eta_i;
	ctl_temp.eta_d = ctl->eta_d;
	ctl_temp.alpha = ctl->alpha;
	ctl_temp.eta = ctl->eta;
	

	if(kfifo_in(&fifo, &ctl_temp, sizeof(ctl_temp)))
	{
		printk(KERN_INFO "kfifo put ctl success\n");
		printk(KERN_INFO "Len after put ctl:%u\n", kfifo_len(&fifo));
	}
}

//用户空间写入proc文件
ssize_t rbfgrad_write( struct file *filp, const char __user *buff,
                        unsigned long len, void *data )
{
}
//用户空间读出proc文件  
/*int rbfgrad_read( char *page, char **start, off_t off,
                   int count, int *eof, void *data )
{
}*/

static void *rbfgrad_start(struct seq_file *s, loff_t *pos)
{
	ctl_temp = ctl_init;
	printk(KERN_INFO "rbfgrad_start\n");
	printk(KERN_INFO "Start position: %lld", (long long)*pos);
	if(*pos == 0)
	{
		printk(KERN_INFO "put headers into rbfgrad proc file\n");
		return &ctl_temp;
	}
	return NULL;
}

static void *rbfgrad_next(struct seq_file *s, void *v, loff_t *pos)
{
	int ret = 0;
	ctl_temp = ctl_init;
	printk(KERN_INFO "rbfgrad_next\n");
	(*pos)++;
	/*qjl kfifo get*/
	if(*pos >= MAX_TO_GET)
	{
		printk(KERN_INFO "Already Get the needed number!\n");
		return NULL;
	}

	printk(KERN_INFO "Len before get:%u\n", kfifo_len(&fifo));
	ret = kfifo_out(&fifo, &ctl_temp, sizeof(ctl_temp));
	if(!ret)
	{
		printk(KERN_INFO "Fail to get the next\n");
		return NULL;
	}
	else
	{
		printk(KERN_INFO "Success to get the next\n");
		printk(KERN_INFO "Len after get:%u\n", kfifo_len(&fifo));
		return &ctl_temp;
	}
}

static void rbfgrad_stop(struct seq_file *s, void *v)
{
	printk(KERN_INFO "rbfgrad parmeters taking is done!\n");
	return;
}

static int rbfgrad_show(struct seq_file *s, void *v)
{
	struct tc_rbfgrad_qopt *t = (struct tc_rbfgrad_qopt *)v;
	printk(KERN_INFO "rbfgrad_show\n");
	seq_printf(s, "%d\n%d\n%lld\n%lld\n%lld\n%lld\n%d\n%d\n%lld\n%lld\n",t->sampl_period, t->q_ref, *((long long *)&(t->p_max)), *((long long *)&(t->eta_p)), *((long long *)&(t->eta_i)), *((long long *)&(t->eta_d)), t->m, t->n, *((long long *)&(t->alpha)), *((long long *)&(t->eta)));
	
	return 0;
}

static struct seq_operations rbfgrad_seq_op = {
	.start = rbfgrad_start,
	.next = rbfgrad_next,
	.stop = rbfgrad_stop,
	.show = rbfgrad_show
};

static int rbfgrad_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &rbfgrad_seq_op); 
}

/*static ssize_t rbfgrad_read(struct file *file, char __user *buf,
						size_t count, loff_t *ppos)
{
	int ret;
	unsigned int copied;

	ctl_temp.sampl_period = 1;
	ctl_temp.q_ref = 1;
	ctl_temp.p_max = 0.1;
	ctl_temp.eta_p = 0.1;
	ctl_temp.eta_i = 0.1;
	ctl_temp.eta_d = 0.1;
	ctl_temp.alpha = 0.1;
	ctl_temp.eta = 0.1;
	

	if(kfifo_in(&fifo, &ctl_temp, 1 ))
	{
		printk(KERN_INFO "kfifo put ctl success\n");
		printk(KERN_INFO "Len after put ctl:%u\n", kfifo_len(&fifo));
	}

	ret = kfifo_out(&fifo, &ctl_temp, 1);
	printk(KERN_INFO "sampl_period:%d  q_ref:%d  p_max:%e  eta_p:%e  eta_i:%e  eta_d:%e  n:%d  m:%d  alpha:%e  eta:%e\n",ctl_temp.sampl_period,ctl_temp.q_ref,ctl_temp.p_max,ctl_temp.eta_p,ctl_temp.eta_i,ctl_temp.eta_d,ctl_temp.n,ctl_temp.m,ctl_temp.alpha,ctl_temp.eta);
	kfifo_in(&fifo, &ctl_temp, 1 );
	if (mutex_lock_interruptible(&read_lock))
		return -ERESTARTSYS;

	ret = kfifo_to_user(&fifo, buf, count, &copied);

	mutex_unlock(&read_lock);

	return ret ? ret : copied;
}
*/
static struct file_operations rbfgrad_proc_fops = {
	.owner = THIS_MODULE,
	.open = rbfgrad_open,
	.read = seq_read,
	//.read = rbfgrad_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __init tp_rbfgrad_trace_init(void)
{
	int ret;

	ret = register_trace_rbfgrad_output(probe_rbfgrad_output, NULL);
	WARN_ON(ret);
	/*qjl proc*/
	proc_entry = proc_create(PROC_FIFO, 0, NULL, &rbfgrad_proc_fops);
	if(proc_entry == NULL)
	{
		printk(KERN_INFO "rbfgrad: Couldn't create proc entry\n");
	}else
	{
		proc_entry->write_proc = rbfgrad_write;
		proc_entry->proc_fops = &rbfgrad_proc_fops;
		printk(KERN_INFO "rbfgrad: Proc Module loaded.\n");
	}

	/*qjl kfifo*/
	if(kfifo_alloc(&fifo, BUF_SIZE, GFP_KERNEL))
		printk(KERN_INFO "kfifo: Couldn't create kfifo\n");
	return 0;
}

module_init(tp_rbfgrad_trace_init);

static void __exit tp_rbfgrad_trace_exit(void)
{
	unregister_trace_rbfgrad_output(probe_rbfgrad_output, NULL);
	tracepoint_synchronize_unregister();
	/*qjl proc*/
	remove_proc_entry("rbfgrad", NULL);
	/*qjl kfifo*/
	kfifo_free(&fifo);
}

module_exit(tp_rbfgrad_trace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Tracepoint Probes Samples");
