/*
 * net/sched/sch_rbfgrad.c	RBF-PID using Gradient descent method.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 *
 * Authors:	Junlong Qiao, <zheolong@126.com>
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <rbfgrad.h>
#include <rbfgrad_queue.h>
#include <asm/i387.h>   //to support the Floating Point Operation
#include <linux/rwsem.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <exp.h>
#include <fabs.h>
//#include "tp-rbfgrad-trace.h"//trace debug
//#include "tp-rbfgrad-vars-trace.h"//trace debug
//DEFINE_TRACE(rbfgrad_output);//trace debug
//DEFINE_TRACE(rbfgrad_vars_output);//trace debug
//#define TRACE_COUNT_MAX 10
//int trace_count;

//Queue Length Statistics
#define CYC_MAX 20 
int cyc_count;

struct task_struct *tsk_prob;
struct task_struct *tsk_stop_prob;


#ifndef SLEEP_MILLI_SEC  
#define SLEEP_MILLI_SEC(nMilliSec)\
do {\
long timeout = (nMilliSec) * HZ / 1000;\
while(timeout > 0)\
{\
timeout = schedule_timeout(timeout);\
}\
}while(0);
#endif

#define NaN -2251799813685248

#define PROB_TIME 55000 

int prob_number;
int pkg_number;
/*	Parameters, settable by user:
	-----------------------------

	limit		- bytes (must be > qth_max + burst)

	Hard limit on queue length, should be chosen >qth_max
	to allow packet bursts. This parameter does not
	affect the algorithms behaviour and can be chosen
	arbitrarily high (well, less than ram size)
	Really, this limit will never be reached
	if RED works correctly.
 */

struct rbfgrad_sched_data {
	struct timer_list 	ptimer;	

	u32			limit;		/* HARD maximal queue length */
	unsigned char		flags;

	struct rbfgrad_parms	parms;
	struct rbfgrad_stats	stats;
	struct Qdisc		*qdisc;
};

struct queue_show queue_show_base_rbfgrad[QUEUE_SHOW_MAX];EXPORT_SYMBOL(queue_show_base_rbfgrad);
int array_element_rbfgrad = 0;EXPORT_SYMBOL(array_element_rbfgrad);

static void __inline__ rbfgrad_mark_probability(struct Qdisc *sch);


static inline int rbfgrad_use_ecn(struct rbfgrad_sched_data *q)
{
	return q->flags & TC_RBFGRAD_ECN;
}

static inline int rbfgrad_use_harddrop(struct rbfgrad_sched_data *q)
{
	return q->flags & TC_RBFGRAD_HARDDROP;
}

static int rbfgrad_enqueue(struct sk_buff *skb, struct Qdisc *sch)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
    struct rbfgrad_parms *parms = &q->parms;
	struct Qdisc *child = q->qdisc;
	int ret;
	int i;

	pkg_number++;

	switch (rbfgrad_action(&q->parms)) {
	case RBFGRAD_DONT_MARK:
		
		//每CYC_MAX统计一次队列长度值
		//record queue length once every CYC_MAX
		cyc_count++;
		if(cyc_count==CYC_MAX){
			queue_show_base_rbfgrad[array_element_rbfgrad].length=sch->q.qlen;
			queue_show_base_rbfgrad[array_element_rbfgrad].numbers=array_element_rbfgrad;
			queue_show_base_rbfgrad[array_element_rbfgrad].mark_type=RBFGRAD_DONT_MARK;
			queue_show_base_rbfgrad[array_element_rbfgrad].p=*((long long *)(&parms->p_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].p=*((long long *)(&parms->pos[0][1]));
			queue_show_base_rbfgrad[array_element_rbfgrad].kp=*((long long *)(&parms->kp_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].kp=*((long long *)(&parms->pos[0][0]));
			queue_show_base_rbfgrad[array_element_rbfgrad].ki=*((long long *)(&parms->ki_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].ki=*((long long *)(&parms->vel[1][1]));
			queue_show_base_rbfgrad[array_element_rbfgrad].kd=*((long long *)(&parms->kd_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].kd=*((long long *)(&parms->vel[1][0]));
			queue_show_base_rbfgrad[array_element_rbfgrad].jacobian=*((long long *)(&parms->jacobian));
			queue_show_base_rbfgrad[array_element_rbfgrad].NetOut=*((long long *)(&parms->NetOut[parms->gbest_index]));
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k=parms->e_k;
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k_1=parms->e_k_1;
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k_2=parms->e_k_2;
			/*
			for(i=0;i<SAM_NUM;i++)
			{
				queue_show_base_rbfgrad[array_element_rbfgrad].SamIn[i]=*((long long *)(&parms->SamIn[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].SamOut[i]=*((long long *)(&parms->SamOut[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].proba[i]=*((long long *)(&parms->proba[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].queue_len[i]=*((long long *)(&parms->queue_len[i]));
			}
			*/

//--------------------------------------------------------------------------------------------
			//历史数据更新(很难保持数据同步，所以还是在入队列以后进行这个更新操作比较好)
			for(i=SAM_NUM-1;i>0;i--)
			{
				parms->proba[i]=parms->proba[i-1];
				parms->queue_len[i]=parms->queue_len[i-1];
			}
			parms->proba[0]=parms->p_k_1;//注意这里的更新操作，p_k_1作用于队列以后才得到q_k
			//每次包来的时候调用
			//parms->q_k = sch->q.qlen;
			parms->queue_len[0]=sch->q.qlen;
			//更新e（这里的e都是归一化的）
			parms->e_k_2 = parms->e_k_1;
			parms->e_k_1 = parms->e_k;
			parms->e_k = sch->q.qlen - parms->q_ref;

			if(array_element_rbfgrad < QUEUE_SHOW_MAX-1)  array_element_rbfgrad++;
			cyc_count = 0;
		}

		break;

	case RBFGRAD_PROB_MARK:
		
		//每CYC_MAX统计一次队列长度值
		//record queue length once every CYC_MAX
		cyc_count++;
		if(cyc_count==CYC_MAX){
			queue_show_base_rbfgrad[array_element_rbfgrad].length=sch->q.qlen;
			queue_show_base_rbfgrad[array_element_rbfgrad].numbers=array_element_rbfgrad;
			queue_show_base_rbfgrad[array_element_rbfgrad].mark_type=RBFGRAD_PROB_MARK;
			queue_show_base_rbfgrad[array_element_rbfgrad].p=*((long long *)(&parms->p_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].p=*((long long *)(&parms->pos[0][1]));
			queue_show_base_rbfgrad[array_element_rbfgrad].kp=*((long long *)(&parms->kp_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].kp=*((long long *)(&parms->pos[0][0]));
			queue_show_base_rbfgrad[array_element_rbfgrad].ki=*((long long *)(&parms->ki_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].ki=*((long long *)(&parms->vel[1][1]));
			queue_show_base_rbfgrad[array_element_rbfgrad].kd=*((long long *)(&parms->kd_k));
			//queue_show_base_rbfgrad[array_element_rbfgrad].kd=*((long long *)(&parms->vel[1][0]));
			queue_show_base_rbfgrad[array_element_rbfgrad].jacobian=*((long long *)(&parms->jacobian));
			queue_show_base_rbfgrad[array_element_rbfgrad].NetOut=*((long long *)(&parms->NetOut[parms->gbest_index]));
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k=parms->e_k;
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k_1=parms->e_k_1;
			queue_show_base_rbfgrad[array_element_rbfgrad].e_k_2=parms->e_k_2;
			/*
			for(i=0;i<SAM_NUM;i++)
			{
				queue_show_base_rbfgrad[array_element_rbfgrad].SamIn[i]=*((long long *)(&parms->SamIn[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].SamOut[i]=*((long long *)(&parms->SamOut[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].proba[i]=*((long long *)(&parms->proba[i]));
				queue_show_base_rbfgrad[array_element_rbfgrad].queue_len[i]=*((long long *)(&parms->queue_len[i]));
			}
			*/
//--------------------------------------------------------------------------------------------
			//历史数据更新(很难保持数据同步，所以还是在入队列以后进行这个更新操作比较好)
			for(i=SAM_NUM-1;i>0;i--)
			{
				parms->proba[i]=parms->proba[i-1];
				parms->queue_len[i]=parms->queue_len[i-1];
			}
			parms->proba[0]=parms->p_k_1;//注意这里的更新操作，p_k_1作用于队列以后才得到q_k
			//每次包来的时候调用
			//parms->q_k = sch->q.qlen;
			parms->queue_len[0]=sch->q.qlen;
			//更新e（这里的e都是归一化的）
			parms->e_k_2 = parms->e_k_1;
			parms->e_k_1 = parms->e_k;
			parms->e_k = sch->q.qlen - parms->q_ref;

			if(array_element_rbfgrad < QUEUE_SHOW_MAX-1)  array_element_rbfgrad++;
			cyc_count = 0;
		}

		sch->qstats.overlimits++;
		if (!rbfgrad_use_ecn(q) || !INET_ECN_set_ce(skb)) {
			q->stats.prob_drop++;
			goto congestion_drop;
		}


		q->stats.prob_mark++;
		break;
	}

	ret = qdisc_enqueue(skb, child);


//--------------------------------------------------------------------------------------------
	if (likely(ret == NET_XMIT_SUCCESS)) {
		sch->q.qlen++;

		sch->qstats.backlog += skb->len;/*2012-1-21*/
		//sch->qstats.bytes += skb->len;/*2012-1-21*/
		//sch->qstats.packets++;/*2012-1-21*/

	} else if (net_xmit_drop_count(ret)) {
		q->stats.pdrop++;
		sch->qstats.drops++;
	}
	

	return ret;

congestion_drop:
	qdisc_drop(skb, sch);
	return NET_XMIT_CN;
}

static struct sk_buff *rbfgrad_dequeue(struct Qdisc *sch)
{
	struct sk_buff *skb;
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;

	skb = child->dequeue(child);
	if (skb) {
		qdisc_bstats_update(sch, skb);

		sch->qstats.backlog -= skb->len;/*2012-1-21*/	

		sch->q.qlen--;
	} else {
	}
	return skb;
}

static struct sk_buff *rbfgrad_peek(struct Qdisc *sch)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;

	return child->ops->peek(child);
}

static unsigned int rbfgrad_drop(struct Qdisc *sch)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct Qdisc *child = q->qdisc;
	unsigned int len;

	if (child->ops->drop && (len = child->ops->drop(child)) > 0) {

		sch->qstats.backlog -= len;/*2012-1-21*/					

		q->stats.other++;
		sch->qstats.drops++;
		sch->q.qlen--;
		return len;
	}

	return 0;
}

static void rbfgrad_reset(struct Qdisc *sch)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);

	qdisc_reset(q->qdisc);

	sch->qstats.backlog = 0;/*2012-1-21*/	
	
	sch->q.qlen = 0;

	array_element_rbfgrad = 0;/*2012-1-21*/
	
	rbfgrad_restart(&q->parms);
}

static void rbfgrad_destroy(struct Qdisc *sch)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	
	//删除计时器，并将输出数据需要的array_element_rbfgrad置为0
    array_element_rbfgrad = 0;
/*
	if(tsk_prob!=NULL)
		kthread_stop(tsk_prob);
	if(tsk_stop_prob!=NULL)
		kthread_stop(tsk_stop_prob);
		*/
	kernel_fpu_end();//为了支持浮点运算

	printk("<1>prob_number:%d\n",prob_number);
	printk("<1>pkg_number:%d\n",pkg_number);

	qdisc_destroy(q->qdisc);

}

static const struct nla_policy rbfgrad_policy[TCA_RBFGRAD_MAX + 1] = {
	[TCA_RBFGRAD_PARMS]	= { .len = sizeof(struct tc_rbfgrad_qopt) },
	[TCA_RBFGRAD_STAB]	= { .len = RBFGRAD_STAB_SIZE },
};

/*qjl
缩写的一些说明：
Qdisc    		Queue discipline
rbfgrad      		rbf-pid gradint method
nlattr   		net link attributes
rbfgrad_sched_data   rbfgrad scheduler data
qdisc_priv           qdisc private（Qdisc中针对特定算法如RBFGRAD的数据）
tca			traffic controll attributes
nla 			net link attributes
*/
static int rbfgrad_change(struct Qdisc *sch, struct nlattr *opt)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct nlattr *tb[TCA_RBFGRAD_MAX + 1];
	struct tc_rbfgrad_qopt *ctl;
	struct Qdisc *child = NULL;
	int err;

	if (opt == NULL)
		return -EINVAL;

	err = nla_parse_nested(tb, TCA_RBFGRAD_MAX, opt, rbfgrad_policy);
	if (err < 0)
		return err;

	if (tb[TCA_RBFGRAD_PARMS] == NULL ||
	    tb[TCA_RBFGRAD_STAB] == NULL)
		return -EINVAL;
	
	/*求有效载荷的起始地址*/
	ctl = nla_data(tb[TCA_RBFGRAD_PARMS]);

	if (ctl->limit > 0) {
		child = fifo_create_dflt(sch, &bfifo_qdisc_ops, ctl->limit);
		if (IS_ERR(child))
			return PTR_ERR(child);
	}

	sch_tree_lock(sch);
	q->flags = ctl->flags;
	q->limit = ctl->limit;
	if (child) {
		qdisc_tree_decrease_qlen(q->qdisc, q->qdisc->q.qlen);
		qdisc_destroy(q->qdisc);
		q->qdisc = child;
	}

	//设置算法参数，此函数是在rbfgrad.h中定义的
	rbfgrad_set_parms(&q->parms, ctl->sampl_period, 
                             ctl->q_ref, ctl->p_init, ctl->p_min, ctl->p_max, 
                             ctl->kp_k, ctl->ki_k, ctl->kd_k, ctl->eta_p, ctl->eta_i, ctl->eta_d, 
				 ctl->n, ctl->m, ctl->alpha, ctl->eta, 
				 ctl->Scell_log, nla_data(tb[TCA_RBFGRAD_STAB]));

	//利用trace debug内核代码
	//trace_rbfgrad_output(ctl);


	array_element_rbfgrad = 0;/*2012-1-21*/

	sch_tree_unlock(sch);

	return 0;
}

int rbfgrad_thread_func(void* data)
{
	struct Qdisc *sch=(struct Qdisc *)data;
	//kernel_fpu_begin();//为了支持浮点运算
	do{
		sch=(struct Qdisc *)data;
		rbfgrad_mark_probability(sch);
		//SLEEP_MILLI_SEC(5);
		schedule_timeout(10*HZ);
	}while(!kthread_should_stop());
	//kernel_fpu_begin();//为了支持浮点运算
	return 0;
}
int rbfgrad_stop_prob_thread_func(void* data)
{
	SLEEP_MILLI_SEC(PROB_TIME);
	kthread_stop(tsk_prob);
	return 0;
}


double get_rbf_SSE(struct rbfgrad_parms *parms,int which)
{
	double *w=(parms->pos)[which];

	int i,j;
	double SSE;
	double sum[UNIT_NUM];
	double r[UNIT_NUM];		// 隐含层神经元值

		parms->NetOut[which] = 0;
	    for(i=0;i<UNIT_NUM;i++){
			sum[i] = 0;
			for(j = 0; j < SAM_NUM; j++)
				sum[i] = sum[i] + (parms->SamIn[j]-parms->c_k[i][j])*(parms->SamIn[j]-parms->c_k[i][j]);//求平方和
			r[i] = exp(- sum[i] / (2*parms->delta_k[i]*parms->delta_k[i]));//求隐含层神经元值
			parms->NetOut[which] = parms->NetOut[which] + w[i] * r[i];
		}

		SSE = (parms->queue_len[0]/6500.00-parms->NetOut[which])*(parms->queue_len[0]/6500.00-parms->NetOut[which])/2;

		/*
		SSE = 0;
		for(j=0; j<SAM_NUM; j++)
			SSE = SSE + (SamOut[j]-NetOut)*(SamOut[j]-NetOut); 
			*/
	
	return SSE;
}


static void __inline__ rbfgrad_mark_probability(struct Qdisc *sch)
{
    struct rbfgrad_sched_data *data = qdisc_priv(sch);
    struct rbfgrad_parms *parms = &data->parms;
	int i,j,k,index;
	double temp;

	//trace debug
	//struct trace_rbfgrad_parms trace_parms;

	------------------------------------------------------
	  variable   |   type       | meaning
	------------------------------------------------------
				 |                 |
	------------------------------------------------------			 
				 |                 |
	------------------------------------------------------
				 |                 |
	------------------------------------------------------
				 |                 |
	------------------------------------------------------

	double eta_p;	// PID比例参数kp的学习速率
	double eta_i;	// PID积分参数ki的学习速率
	double eta_d;	// PID微分参数kd的学习速率
	double alpha;	// RBF的梯度下降学习算法的动量因子，设置为0.01
	double eta;		// RBF的梯度下降学习算法的学习速率，设置为0.01

	double r[UNIT_NUM];		// 隐含层神经元值
	
	double e_k;
	double e_k_1;
	double e_k_2;
	double queue_len[SAM_NUM];

	//for will use
	int epoch;
	double sum[UNIT_NUM];
	double SSE[PARTICLE_NUM];
	double oldNetOut;

	//PSO
	double iw1;
	double iw2;
	double iwe;
	double ac1;
	double ac2;
	double mv;
	double mwav;
	double ergrd;
	double ergrdep;

	double iwt[MAX_EPOCH];
	double rannum1,rannum2;

	eta_p = parms->eta_p;
	eta_i = parms->eta_i;
	eta_d = parms->eta_d;
	eta = parms->eta;
	alpha = parms->alpha;
	parms->jacobian = 0;
	
	//标准化 归一化
	e_k = parms->e_k / 6500.00;
	e_k_1 = parms->e_k_1 / 6500.00;
	e_k_2 = parms->e_k_2 / 6500.00;
	for(i=0;i<SAM_NUM;i++){
		queue_len[i] = parms->queue_len[i] / 6500.00;
	}
    for(i=0;i<SAM_NUM;i++)
	{
		if(i<SAM_NUM/2)
			parms->SamIn[i] = parms->proba[i];
		else
			parms->SamIn[i] = queue_len[i-SAM_NUM/2];
	}

	//oldNetOut = parms->NetOut;

	iw1 = parms->iw1;
	iw2 = parms->iw2;
	iwe = parms->iwe;
	ac1 = parms->ac1;
	ac2 = parms->ac2;
	mv = parms->mv;
	mwav = parms->mwav;
	ergrd = parms->ergrd;
	ergrdep = parms->ergrdep;
//---------------------------原先的梯度学习方法调整RBF参数-------------------------------------------------
	/*
	for(epoch=0; epoch < parms->MaxEpoch; epoch++)
	{
		parms->NetOut = 0;
	    for(i=0;i<UNIT_NUM;i++){
			sum[i] = 0;
			for(j = 0; j < SAM_NUM; j++)
				sum[i] = sum[i] + (parms->SamIn[j]-parms->c_k[i][j])*(parms->SamIn[j]-parms->c_k[i][j]);//求平方和
			r[i] = exp(- sum[i] / (2*parms->delta_k[i]*parms->delta_k[i]));//求隐含层神经元值
			parms->NetOut = parms->NetOut + parms->w_k[i] * r[i];
		}

		SSE = (queue_len[0]-parms->NetOut)*(queue_len[0]-parms->NetOut)/2;
		
	    for(i=0;i<UNIT_NUM;i++){
			parms->w_k_2[i]=parms->w_k_1[i];
			parms->w_k_1[i=parms->w_k[i];
			parms->delta_k_2[i]=parms->delta_k_1[i];
			parms->delta_k_1[i]=parms->delta_k[i];
			for(j=0;j<SAM_NUM;j++){
				parms->c_k_2[i][j]=parms->c_k_1[i][j];
				parms->c_k_1[i][j]=parms->c_k[i][j];
			}
		}

	    for(i=0;i<UNIT_NUM;i++){
			parms->w_k[i]=parms->w_k_1[i]+ \
				   alpha*(parms->w_k_1[i]-parms->w_k_2[i])+ \
				   eta*(queue_len[0]-parms->NetOut)*r[i];
			parms->delta_k[i]=parms->delta_k_1[i]+ \
					   alpha*(parms->delta_k_1[i]-parms->delta_k_2[i])+ \
					   eta*(queue_len[0]-parms->NetOut)*parms->w_k[i]*r[i]*sum[i]/(parms->delta_k_1[i]*parms->delta_k[i]*parms->delta_k[i]);
	        for(j=0;j<SAM_NUM;j++){
				parms->c_k[i][j]=parms->c_k_1[i][j] + \
						  alpha*(parms->c_k_1[i][j]-parms->c_k_2[i][j])+ \
						  eta*(queue_len[0]-parms->NetOut)*parms->w_k[i]*r[i]*(parms->SamIn[j]-parms->c_k_1[i][j])/(parms->delta_k[i]*parms->delta_k[i]);
			}
		}
	}
	*/
//---------------------------现在的GAPSO学习方法调整RBF参数-----------------------------------------------
	//initial pbest positions vals
	//初始化pbest
	for(i=0;i<PARTICLE_NUM;i++)
		for(j=0;j<UNIT_NUM;j++)
			parms->pbest[i][j]=parms->pos[i][j];
	//得到误差的方差和，也就是粒子的评价指标值，初始化pbestval
	for(i=0; i<PARTICLE_NUM; i++)
		parms->pbestval[i] = get_rbf_SSE(parms,i);
	//初始化parms->gbestval，index为pbestval最小的粒子索引
	parms->gbestval = parms->pbestval[0];
	index = 0;
	for(j=0;j<PARTICLE_NUM;j++){
		if(parms->gbestval>parms->pbestval[j]){
			parms->gbestval = parms->pbestval[j];
			index = j;
		}
	}
	//初始化gbest
	for(k=0;k<UNIT_NUM;k++)	
		parms->gbest[k] = parms->pbest[index][k];
	//tr[0] = parms->gbestval; //keep track of global best SSE

	//主循环开始
	for(i=0; i < MAX_EPOCH; i++){
		//对每个粒子求评价指标（SSE）值，根据该值更新每个粒子的pbest和pbestval
		for(j=0; j<PARTICLE_NUM; j++){
			SSE[j] = get_rbf_SSE(parms,j);//use Wj
			//update pbest
			if(parms->pbestval[j]>SSE[j]){
				parms->pbestval[j]=SSE[j];
				for(k=0;k<UNIT_NUM;k++)	
					parms->pbest[j][k] = parms->pos[j][k];
			}
		}
		//update gbest
		parms->gbestval = parms->pbestval[0];
		index = 0;
		for(j=0;j<PARTICLE_NUM;j++){
			if(parms->gbestval>parms->pbestval[j]){
				parms->gbestval = parms->pbestval[j];
				index = j;
			}
		}
		for(k=0;k<UNIT_NUM;k++)	
			parms->gbest[k] = parms->pbest[index][k];
		
		//tr[i+1] = parms->gbestval; //keep track of global best SSE
		//te = i;//this will return the epoch number to calling program when done
		
		//get inertia weight ,just a linear funct w.r.t epoch parameter iwe
		if(i<iwe)
			iwt[i]=((iw2-iw1)/(iwe-1))*(i)+iw1;
		else
			iwt[i]=iw2;

		//this for loop is the heart of the PSO algorithm,updates position an velocity across dimension D
		//此for循环是PSO算法的核心，更新粒子位置和速度
		for(j=0; j<PARTICLE_NUM; j++){
			for(k=0; k<UNIT_NUM; k++){
				rannum1 = (double)random32()/(RAND_MAX+1.0);
				rannum2 = (double)random32()/(RAND_MAX+1.0);
				//update velocity for each dimension of each particle
				//更新速度
				parms->vel[j][k] = iwt[i]*parms->vel[j][k] + ac1 *rannum1 * (parms->pbest[j][k] - parms->pos[j][k]) + ac2 *rannum2 * (parms->gbest[k] - parms->pos[j][k]);

				if((parms->vel[j][k])>mv)
					parms->vel[j][k]=mv;
				if((parms->vel[j][k])<-mv)
					parms->vel[j][k]=-mv;
			}
			//update position for each particle
			//更新位置
			for(k=0; k<UNIT_NUM; k++)
				parms->pos[j][k] = parms->pos[j][k] + parms->vel[j][k]; 
		}

		//check for stopping criterion based on speed of convergence to desired error
		//如果上一次的parms->gbestval和此次的相差
		/*
		if(abs(tr[i]-parms->gbestval)>ergrd){
			cnt2=0;
		}
		else{
			cnt2++;
			if(cnt2>=ergrdep) break;
		}
		*/
	}
	//主循环结束
	
	//更新RBF的权重参数w_k
	for(j=0;j<UNIT_NUM;j++) parms->w_k[j] = parms->gbest[j];	

	//printk(KERN_INFO "%d\n",index);
	parms->gbest_index = index;
//--------------------------------------------------------------------------------------------
	//计算parms->jacobian信息
	//printk(KERN_INFO "-------------------jacobian---------------\n");
	if(parms->gbestval < parms->E0)
	{
	for(i = 0; i < UNIT_NUM; i++)
	{
		temp = 0;
		for(j = 0; j < SAM_NUM; j++)
			temp = temp + (parms->SamIn[j]-parms->c_k[i][j])*(parms->SamIn[j]-parms->c_k[i][j]);//求平方和
				
		r[i] = exp(- temp / (2*parms->delta_k[i]*parms->delta_k[i]));//求隐含层神经元值

		//parms->jacobian = parms->jacobian + parms->w_k[i]*r[i]*(parms->c_k[i][0]-(parms->p_k-parms->p_k_1)) / (parms->delta_k[i]*parms->delta_k[i]); //求jacobian信息，注意
		parms->jacobian = parms->jacobian + parms->w_k[i]*r[i]*(parms->c_k[i][0]-parms->p_k) / (parms->delta_k[i]*parms->delta_k[i]); //求jacobian信息，注意

	}
	//计算PID三参数
		parms->kp_k_1 = parms->kp_k;
		parms->ki_k_1 = parms->ki_k;
		parms->kd_k_1 = parms->kd_k;

		parms->kp_k = parms->kp_k_1 + eta_p * e_k * parms->jacobian * (e_k - e_k_1);
		parms->ki_k = parms->ki_k_1 + eta_i * e_k * parms->jacobian * e_k;
		parms->kd_k = parms->kd_k_1 + eta_d * e_k * parms->jacobian * (e_k - 2 * e_k_1 + e_k_2);

		if(parms->kp_k < 0)
			parms->kp_k = 0;
		if(parms->ki_k < 0)
			parms->ki_k = 0;
		if(parms->kd_k < 0)
			parms->kd_k = 0;
	}

	//计算丢弃概率
	parms->p_k_1 = parms->p_k;

	parms->p_k = parms->p_k_1 + \
                    parms->kp_k * (e_k - e_k_1) + \
                    parms->ki_k * e_k + \
					parms->kd_k * (e_k - 2 * e_k_1 + e_k_2);

	//确保p_k在p_min和p_max之间
    if (parms->p_k > parms->p_max)
       	parms->p_k = parms->p_max;
    if (parms->p_k < parms->p_min)
       	parms->p_k = parms->p_min;

	prob_number++;
}


static int rbfgrad_init(struct Qdisc *sch, struct nlattr *opt)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	int ret;

	
	kernel_fpu_begin();//为了支持浮点运算
	array_element_rbfgrad = 0;/*2012-1-21*/

	//trace_count = 0;//trace debug
	cyc_count = 0;

	prob_number = 0;
	pkg_number = 0;

	//declare semaphore
	//semaphore ----- current queue length (parms->q_k) 
	//init_rwsem(&current_q_sem);
	//semaphore ----- current proba (parms->p_k) 
	init_rwsem(&current_p_sem);
	/*
	//semaphore ----- queue length samples array (parms->queue_len)
	init_rwsem(&queue_len_samples_array_sem);
	//semaphore ----- proba samples array (parms->proba)
	init_rwsem(&proba_samples_array_sem);
	*/
	q->qdisc = &noop_qdisc;
	//printk("<1>start to change");
	ret=rbfgrad_change(sch, opt);
	//Initialize the timer

	tsk_prob=kthread_run(rbfgrad_thread_func,(void*)sch,"rbfgrad");
	tsk_stop_prob=kthread_run(rbfgrad_stop_prob_thread_func,(void*)sch,"rbfgrad_stop_prob");

	return ret;
}

static int rbfgrad_dump(struct Qdisc *sch, struct sk_buff *skb)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct nlattr *opts = NULL;
	struct tc_rbfgrad_qopt opt = {
		.limit		= q->limit,
		.sampl_period	= q->parms.sampl_period,
		.q_ref		= q->parms.q_ref,
		.p_max		= q->parms.p_max,
		.kp_k		= q->parms.kp_k,
		.ki_k		= q->parms.ki_k,
		.kd_k		= q->parms.kd_k,
		.eta_p		= q->parms.eta_p,
		.eta_i		= q->parms.eta_i,
		.eta_d		= q->parms.eta_d,
		.n		= q->parms.n,
		.m		= q->parms.m,
		.alpha		= q->parms.alpha,
		.eta		= q->parms.eta,
	};

	sch->qstats.backlog = q->qdisc->qstats.backlog;
	opts = nla_nest_start(skb, TCA_OPTIONS);
	if (opts == NULL)
		goto nla_put_failure;
	NLA_PUT(skb, TCA_RBFGRAD_PARMS, sizeof(opt), &opt);
	return nla_nest_end(skb, opts);

nla_put_failure:
	nla_nest_cancel(skb, opts);
	return -EMSGSIZE;
}

static int rbfgrad_dump_stats(struct Qdisc *sch, struct gnet_dump *d)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	struct tc_rbfgrad_xstats st = {
		.early	= q->stats.prob_drop + q->stats.forced_drop,
		.pdrop	= q->stats.pdrop,
		.other	= q->stats.other,
		.marked	= q->stats.prob_mark + q->stats.forced_mark,
	};

	return gnet_stats_copy_app(d, &st, sizeof(st));
}

static int rbfgrad_dump_class(struct Qdisc *sch, unsigned long cl,
			  struct sk_buff *skb, struct tcmsg *tcm)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);

	tcm->tcm_handle |= TC_H_MIN(1);
	tcm->tcm_info = q->qdisc->handle;
	return 0;
}

static int rbfgrad_graft(struct Qdisc *sch, unsigned long arg, struct Qdisc *new,
		     struct Qdisc **old)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);

	if (new == NULL)
		new = &noop_qdisc;

	sch_tree_lock(sch);
	*old = q->qdisc;
	q->qdisc = new;
	qdisc_tree_decrease_qlen(*old, (*old)->q.qlen);
	qdisc_reset(*old);
	sch_tree_unlock(sch);
	return 0;
}

static struct Qdisc *rbfgrad_leaf(struct Qdisc *sch, unsigned long arg)
{
	struct rbfgrad_sched_data *q = qdisc_priv(sch);
	return q->qdisc;
}

static unsigned long rbfgrad_get(struct Qdisc *sch, u32 classid)
{
	return 1;
}

static void rbfgrad_put(struct Qdisc *sch, unsigned long arg)
{
}

static void rbfgrad_walk(struct Qdisc *sch, struct qdisc_walker *walker)
{
	if (!walker->stop) {
		if (walker->count >= walker->skip)
			if (walker->fn(sch, 1, walker) < 0) {
				walker->stop = 1;
				return;
			}
		walker->count++;
	}
}

static const struct Qdisc_class_ops rbfgrad_class_ops = {
	.graft		=	rbfgrad_graft,
	.leaf		=	rbfgrad_leaf,
	.get		=	rbfgrad_get,
	.put		=	rbfgrad_put,
	.walk		=	rbfgrad_walk,
	.dump		=	rbfgrad_dump_class,
};

static struct Qdisc_ops rbfgrad_qdisc_ops __read_mostly = {
	.id		=	"rbfgrad",
	.priv_size	=	sizeof(struct rbfgrad_sched_data),
	.cl_ops	=	&rbfgrad_class_ops,
	.enqueue	=	rbfgrad_enqueue,
	.dequeue	=	rbfgrad_dequeue,
	.peek		=	rbfgrad_peek,
	.drop		=	rbfgrad_drop,
	.init		=	rbfgrad_init,
	.reset		=	rbfgrad_reset,
	.destroy	=	rbfgrad_destroy,
	.change	=	rbfgrad_change,
	.dump		=	rbfgrad_dump,
	.dump_stats	=	rbfgrad_dump_stats,
	.owner		=	THIS_MODULE,
};

static int __init rbfgrad_module_init(void)
{
	return register_qdisc(&rbfgrad_qdisc_ops);
}

static void __exit rbfgrad_module_exit(void)
{
	unregister_qdisc(&rbfgrad_qdisc_ops);
}

module_init(rbfgrad_module_init)
module_exit(rbfgrad_module_exit)

MODULE_LICENSE("GPL");
