/*
 * tracepoint-probe-rbfgrad_vars.c
 *
 * rbfgrad vars tracepoint probes.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/file.h>
#include <linux/dcache.h>
#include <linux/mutex.h>
#include "tp-rbfgrad-vars-trace.h"
/*qjl proc*/
#include <linux/proc_fs.h>
/*qjl kfifo*/
#include <linux/kfifo.h>

/*qjl proc*/
static struct proc_dir_entry *proc_entry;/*qjl proc*/
/* name of the proc entry */
#define	PROC_FIFO	"rbfgrad_vars"

/*qjl kfifo*/
static struct kfifo fifo;
#define BUF_SIZE (10*1024)
#define MAX_TO_GET 10000 
/* lock for procfs read access */
static DEFINE_MUTEX(read_lock);
/* lock for procfs write access */
static DEFINE_MUTEX(write_lock);

struct trace_rbfgrad_parms parms_init = {
	/*Parameters*/
	.sampl_period = 0, 
	.q_ref = 0, 
	.p_max = 0, 
	.eta_p = 0, 
	.eta_i = 0, 
	.eta_d = 0, 
	.n = 0, 
	.m = 0, 
	.alpha = 0, 
	.eta = 0,
	/*Variables*/
	//queue control
	.p_k = 0,
	.p_k_1 = 0,
	.e_k = 0,
	.e_k_1 = 0,
	.e_k_2 = 0,
	//PID
	.kp_k = 0,
	.kp_k_1 = 0,
	.ki_k = 0,
	.ki_k_1 = 0,
	.kd_k = 0,
	.kd_k_1 = 0,
};
struct trace_rbfgrad_parms parms_temp;

/*
 * Here the caller only guarantees locking for struct file and struct inode.
 * Locking must therefore be done in the probe to use the dentry.
 */
static void probe_rbfgrad_vars_output(void *ignore,
			       struct trace_rbfgrad_parms *parms)
{
/*	struct trace_rbfgrad_parms parms_temp = *parms;

	printk(KERN_INFO "Ok,we are here\n");

	parms_temp.sampl_period = parms->sampl_period;
	parms_temp.q_ref = parms->q_ref;
	parms_temp.p_max = parms->p_max;
	parms_temp.eta_p = parms->eta_p;
	parms_temp.eta_i = parms->eta_i;
	parms_temp.eta_d = parms->eta_d;
	parms_temp.alpha = parms->alpha;
	parms_temp.eta = parms->eta;

	parms_temp.p_k = parms->p_k;
	parms_temp.p_k_1 = parms->p_k_1;
	parms_temp.e_k = parms->e_k;
	parms_temp.e_k_1 = parms->e_k_1;
	parms_temp.e_k_2 = parms->e_k_2;

	parms_temp.kp_k = parms->kp_k;
	parms_temp.kp_k_1 = parms->kp_k_1;
	parms_temp.ki_k = parms->ki_k;
	parms_temp.ki_k_1 = parms->ki_k_1;
	parms_temp.kd_k = parms->kd_k;
	parms_temp.kd_k_1 = parms->kd_k_1;
	

	//if(kfifo_in(&fifo, &parms_temp, sizeof(parms_temp)))
	//{
		printk(KERN_INFO "kfifo put parms success\n");
		printk(KERN_INFO "Len after put parms:%u\n", kfifo_len(&fifo));
	//}
	*/
}

//用户空间写入proc文件
/*ssize_t rbfgrad_vars_write( struct file *filp, const char __user *buff,
                        unsigned long len, void *data )
{
}*/
//用户空间读出proc文件  
/*int rbfgrad_read( char *page, char **start, off_t off,
                   int count, int *eof, void *data )
{
}*/

static void *rbfgrad_vars_start(struct seq_file *s, loff_t *pos)
{
	parms_temp = parms_init;
	printk(KERN_INFO "rbfgrad_vars_start\n");
	printk(KERN_INFO "Start position: %lld", (long long)*pos);
	if(*pos == 0)
	{
		printk(KERN_INFO "put headers into rbfgrad_vars proc file\n");
		return &parms_temp;
	}
	return NULL;
}

static void *rbfgrad_vars_next(struct seq_file *s, void *v, loff_t *pos)
{
	int ret = 0;
	parms_temp = parms_init;
	printk(KERN_INFO "rbfgrad_vars_next\n");
	(*pos)++;
	/*qjl kfifo get*/
	if(*pos >= MAX_TO_GET)
	{
		printk(KERN_INFO "Already Get the needed number!\n");
		return NULL;
	}

	printk(KERN_INFO "Len before get:%u\n", kfifo_len(&fifo));
	ret = kfifo_out(&fifo, &parms_temp, sizeof(parms_temp));
	if(!ret)
	{
		printk(KERN_INFO "Fail to get the next\n");
		return NULL;
	}
	else
	{
		printk(KERN_INFO "Success to get the next\n");
		printk(KERN_INFO "Len after get:%u\n", kfifo_len(&fifo));
		return &parms_temp;
	}
}

static void rbfgrad_vars_stop(struct seq_file *s, void *v)
{
	printk(KERN_INFO "rbfgrad parmeters taking is done!\n");
	return;
}

static int rbfgrad_vars_show(struct seq_file *s, void *v)
{
	struct trace_rbfgrad_parms *t = (struct trace_rbfgrad_parms *)v;
	printk(KERN_INFO "rbfgrad_vars_show\n");
	seq_printf(s, "%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n%lld\n",
			*((long long *)&(t->p_k)), 
			*((long long *)&(t->p_k_1)), 
			*((long long *)&(t->e_k)), 
			*((long long *)&(t->e_k_1)), 
			*((long long *)&(t->e_k_1)),
			*((long long *)&(t->kp_k)),
			*((long long *)&(t->kp_k_1)),
			*((long long *)&(t->ki_k)),
			*((long long *)&(t->ki_k_1)),
			*((long long *)&(t->kd_k)),
			*((long long *)&(t->kd_k_1)));
	return 0;
}

static struct seq_operations rbfgrad_seq_op = {
	.start = rbfgrad_vars_start,
	.next = rbfgrad_vars_next,
	.stop = rbfgrad_vars_stop,
	.show = rbfgrad_vars_show
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

	parms_temp.sampl_period = 1;
	parms_temp.q_ref = 1;
	parms_temp.p_max = 0.1;
	parms_temp.eta_p = 0.1;
	parms_temp.eta_i = 0.1;
	parms_temp.eta_d = 0.1;
	parms_temp.alpha = 0.1;
	parms_temp.eta = 0.1;
	

	if(kfifo_in(&fifo, &parms_temp, 1 ))
	{
		printk(KERN_INFO "kfifo put ctl success\n");
		printk(KERN_INFO "Len after put ctl:%u\n", kfifo_len(&fifo));
	}

	ret = kfifo_out(&fifo, &parms_temp, 1);
	printk(KERN_INFO "sampl_period:%d  q_ref:%d  p_max:%e  eta_p:%e  eta_i:%e  eta_d:%e  n:%d  m:%d  alpha:%e  eta:%e\n",parms_temp.sampl_period,parms_temp.q_ref,ctl_temp.p_max,ctl_temp.eta_p,ctl_temp.eta_i,ctl_temp.eta_d,ctl_temp.n,ctl_temp.m,ctl_temp.alpha,ctl_temp.eta);
	kfifo_in(&fifo, &parms_temp, 1 );
	if (mutex_lock_interruptible(&read_lock))
		return -ERESTARTSYS;

	ret = kfifo_to_user(&fifo, buf, count, &copied);

	mutex_unlock(&read_lock);

	return ret ? ret : copied;
}
*/
static struct file_operations rbfgrad_vars_proc_fops = {
	.owner = THIS_MODULE,
	.open = rbfgrad_open,
	.read = seq_read,
	//.read = rbfgrad_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

static int __init tp_rbfgrad_vars_trace_init(void)
{
	int ret;

	ret = register_trace_rbfgrad_vars_output(probe_rbfgrad_vars_output, NULL);
	WARN_ON(ret);
	/*qjl proc*/
	proc_entry = proc_create(PROC_FIFO, 0, NULL, &rbfgrad_vars_proc_fops);
	if(proc_entry == NULL)
	{
		printk(KERN_INFO "rbfgrad vars: Couldn't create proc entry\n");
	}else
	{
		//proc_entry->write_proc = rbfgrad_vars_write;
		proc_entry->proc_fops = &rbfgrad_vars_proc_fops;
		printk(KERN_INFO "rbfgrad vars: Proc Module loaded.\n");
	}

	/*qjl kfifo*/
	if(kfifo_alloc(&fifo, BUF_SIZE, GFP_KERNEL))
		printk(KERN_INFO "kfifo: Couldn't create kfifo\n");
	return 0;
}

module_init(tp_rbfgrad_vars_trace_init);

static void __exit tp_rbfgrad_vars_trace_exit(void)
{
	unregister_trace_rbfgrad_vars_output(probe_rbfgrad_vars_output, NULL);
	tracepoint_synchronize_unregister();
	/*qjl proc*/
	remove_proc_entry("rbfgrad_vars", NULL);
	/*qjl kfifo*/
	kfifo_free(&fifo);
}

module_exit(tp_rbfgrad_vars_trace_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mathieu Desnoyers");
MODULE_DESCRIPTION("Tracepoint Probes Samples");
