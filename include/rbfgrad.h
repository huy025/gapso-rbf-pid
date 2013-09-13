/*qjl*/
#ifndef __NET_SCHED_RBFGRAD_H
#define __NET_SCHED_RBFGRAD_H

#include <linux/types.h>
#include <net/pkt_sched.h>
#include <net/inet_ecn.h>
#include <net/dsfield.h>
#include <asm/i387.h>   //为了支持浮点运算
/* The largest number rand will return (same as INT_MAX).  */
#define	RAND_MAX	2147483647
/* 浮点类型的一些常量 */
#define DBL_DIG         15                      /* # of decimal digits of precision */  
#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */  
#define DBL_MANT_DIG    53                      /* # of bits in mantissa */  
#define DBL_MAX         1.7976931348623158e+308 /* max value */  
#define DBL_MAX_10_EXP  308                     /* max decimal exponent */  
#define DBL_MAX_EXP     1024                    /* max binary exponent */  
#define DBL_MIN         2.2250738585072014e-308 /* min positive value */  
#define DBL_MIN_10_EXP  (-307)                  /* min decimal exponent */  
#define DBL_MIN_EXP     (-1021)                 /* min binary exponent */  
#define _DBL_RADIX      2                       /* exponent radix */  
#define _DBL_ROUNDS     1                       /* addition rounding: near */  
#define FLT_DIG         6                       /* # of decimal digits of precision */  
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */  
#define FLT_GUARD       0  
#define FLT_MANT_DIG    24                      /* # of bits in mantissa */  
#define FLT_MAX         3.402823466e+38F        /* max value */  
#define FLT_MAX_10_EXP  38                      /* max decimal exponent */  
#define FLT_MAX_EXP     128                     /* max binary exponent */  
#define FLT_MIN         1.175494351e-38F        /* min positive value */  
#define FLT_MIN_10_EXP  (-37)                   /* min decimal exponent */  
#define FLT_MIN_EXP     (-125)                  /* min binary exponent */  
#define FLT_NORMALIZE   0  
#define FLT_RADIX       2                       /* exponent radix */  
#define FLT_ROUNDS      1                       /* addition rounding: near */

//declare semaphore
//semaphore ----- current queue length (parms->q_k) 
//DECLARE_RWSEM(current_q_sem);	
//semaphore ----- current proba (parms->p_k) 
DECLARE_RWSEM(current_p_sem);	
/*
//semaphore ----- queue length samples array (parms->queue_len)
DECLARE_RWSEM(queue_len_samples_array_sem);	
//semaphore ----- proba samples array (parms->proba)
DECLARE_RWSEM(proba_samples_array_sem);
*/

/*	RBF-PID algorithm(using Gradient decent method).
	=======================================

*/
#define RBFGRAD_STAB_SIZE	256
#define RBFGRAD_STAB_MASK	(RBFGRAD_STAB_SIZE - 1)


#define SAM_NUM  10
#define UNIT_NUM  6
#define PARTICLE_NUM 90 
#define MAX_EPOCH 10

#define rand_my(min,max)\
	((double)((max-min)*random32())/(RAND_MAX+1.0)+min)


struct rbfgrad_stats {
	u32		prob_drop;	/* Early probability drops */
	u32		prob_mark;	/* Early probability marks */
	u32		forced_drop;	/* Forced drops, qavg > max_thresh */
	u32		forced_mark;	/* Forced marks, qavg > max_thresh */
	u32		pdrop;          /* Drops due to queue limits */
	u32		other;          /* Drops due to drop() calls */
};

struct rbfgrad_parms {
	/* Parameters */
	int 	sampl_period;   //采样时间

	u32		Scell_max;
	u8		Scell_log;
	u8		Stab[RBFGRAD_STAB_SIZE];

	/* Parameters */
	//队列控制
	int q_ref;//参考队列长度
	double p_init;//初始丢弃概率
	double p_min;//最小丢弃概率
	double p_max;//最大丢弃概率
	//PID
	double eta_p;//比例 学习速率
	double eta_i;//积分 学习速率
	double eta_d;//微分 学习速率
	//RBF
	int SamNum;  //number of samples in one set
	int n;       //input layer dimensions
	int m;       //hidden layer dimensions
	double alpha;//
	double eta;  //
	int MaxEpoch;//max training times for one set of samples
	double E0;   //training precision
	//PSO    2013-9-8
	double iw1;
	double iw2;
	double iwe;
	double ac1;
	double ac2;
	double mv;
	double mwav;
	double ergrd;
	double ergrdep;
		
	/* Variables */
	//队列控制
	double p_k;	/* Packet current marking probability */
	double p_k_1; /* Packet last marking probability */
    int e_k;
	int e_k_1;
	int e_k_2;
	//PID
	double kp_k;
	double kp_k_1;
	double ki_k;
	double ki_k_1;
	double kd_k;
	double kd_k_1;
	//RBF
	double w_k[UNIT_NUM];
	double w_k_1[UNIT_NUM];
	double w_k_2[UNIT_NUM];
	double c_k[UNIT_NUM][SAM_NUM];
	double c_k_1[UNIT_NUM][SAM_NUM];
	double c_k_2[UNIT_NUM][SAM_NUM];
	double delta_k[UNIT_NUM];
	double delta_k_1[UNIT_NUM];
	double delta_k_2[UNIT_NUM];
	double jacobian;
	double NetOut[PARTICLE_NUM];
	//PSO 2013-9-8
	double w_pso[PARTICLE_NUM][UNIT_NUM];//to seperate with w
	double pos[PARTICLE_NUM][UNIT_NUM];
	double vel[PARTICLE_NUM][UNIT_NUM];
	double pbest[PARTICLE_NUM][UNIT_NUM];
	double pbestval[PARTICLE_NUM];
	double gbest[UNIT_NUM];
	double gbestval;
	int gbest_index;
	

	int q_k;
	double proba[SAM_NUM];//proba array:record the last SamNum proba
	int queue_len[SAM_NUM];//queue length array:record the last SamNum queue length 
	double SamIn[SAM_NUM];
	double SamOut[SAM_NUM];
};

static inline void rbfgrad_set_parms(struct rbfgrad_parms *p, int sampl_period, 
                             int q_ref, double p_init, double p_min, double p_max, 
                             double kp_k, double ki_k, double kd_k, double eta_p, double eta_i, double eta_d, 
				 int n, int m, double alpha, double eta, 
				 u8 Scell_log, u8 *stab)
{
	int i,j;
	double e_max,e_min; 
	/* Reset average queue length, the value is strictly bound
	 * to the parameters below, reseting hurts a bit but leaving
	 * it might result in an unreasonable qavg for a while. --TGR
	 */
	p->sampl_period	= sampl_period;

	p->q_ref		= q_ref;	// 参考队列长度 设置为300
	p->p_init		= p_init;	// 初始丢弃/标记概率  设置为0
	p->p_min		= p_min;	// 最小丢弃/标记概率  设置为0
	p->p_max		= p_max;	// 最大丢弃/标记概率  设置为1

	p->eta_p		= eta_p;	// PID比例参数kp的学习速率
	p->eta_i		= eta_i;	// PID积分参数ki的学习速率
	p->eta_d		= eta_d;	// PID微分参数kd的学习速率

	p->SamNum		= 10;		// RBF输入层的样本数量，即输入层维度，设置为10
	p->n			= n;		//  
	p->m			= m;		// RBF隐含层神经元个数，即隐含层维度，设置为10
	p->alpha		= alpha;	// RBF的梯度下降学习算法的动量因子，设置为0.01
	p->eta			= eta;		// RBF的梯度下降学习算法的学习速率，设置为0.01
	p->MaxEpoch		= 1;		// RBF最大迭代次数     
	p->E0			= 0.000001;		// RBF辨识器的精度

	kernel_fpu_begin();//为了支持浮点运算
	//初始化rbfgrad_parms中非参数的变量，这些变量基本上都是记录算法以前的信息，
	//例如上一次的丢弃概率，上一次的误差，上一次的PID参数，上一次和上上一次的RBF参数
	//队列控制
	p->p_k = p_init;	// 当前丢弃/标记概率，初始化为0 
    p->e_k = 0.0;		// 当前队列长度与参考队列长度的差，初始化为0
	p->e_k_1 = 0.0;		// 上一次队列长度与参考队列长度的差，初始化为0
	p->e_k_2 = 0.0;		// 上一次队列长度与参考队列长度的差，初始化为0
	//PID
	p->kp_k = kp_k;		// PID当前比例参数
	p->ki_k = ki_k;		// PID当前积分参数
	p->kd_k = kd_k;		// PID当前微分参数
	//RBF;
	e_max = 1;
	e_min = -1;

	for(i = 0; i < UNIT_NUM; i++){
		p->w_k[i] = rand_my(-1,1);
		p->w_k_1[i] = rand_my(-1,1);
		for(j = 0; j < SAM_NUM; j++){
			p->c_k[i][j] = 0;//e_min+(i-1)*(e_max-e_min)*(UNIT_NUM-1);
			p->c_k_1[i][j] = e_min+(i-1)*(e_max-e_min)*(UNIT_NUM-1);
		}
		p->delta_k[i] = 40;
		p->delta_k_1[i] = 40;
	}

	for(i = 0; i < SAM_NUM; i++){
		p->proba[i] = p->p_init; 
		p->queue_len[i] = 0;
		p->SamIn[i] = 0;
		p->SamOut[i] = 0;
	} 

	for(i=0;i<PARTICLE_NUM;i++)
		p->NetOut[i] = 0;

	p->e_k = 0;
	p->e_k_1 = 0;
	p->e_k_2 = 0;

	//PSO 2013-9-8
	p->iw1			= 0.9;//用于计算iwt
	p->iw2			= 0.2;//用于计算iwt
	p->iwe			= (MAX_EPOCH)*3.0/4;//用于计算iwt
	p->ac1			= 2;
	p->ac2			= 2;
	p->mv			= 0.01;//粒子速度绝对值最大值
	p->mwav			= 1.0; //粒子位置绝对值最大值
	p->ergrd		= 1e-9;
	p->ergrdep		= (MAX_EPOCH)/20.0;
	//init position vector and velcity vector
	for(j=0;j<UNIT_NUM;j++)
	{
		p->pos[0][j] = p->w_k[j];
		p->vel[0][j] = rand_my(-(p->mv),p->mv);
	}
	for(i=1;i<PARTICLE_NUM;i++)
		for(j=0;j<UNIT_NUM;j++){
			p->pos[i][j]=rand_my(-(p->mwav),p->mwav);
			p->vel[i][j]=rand_my(-(p->mv),p->mv);
		}

	p->gbestval = 0;
	p->gbest_index = 0;

	kernel_fpu_end();//为了支持浮点运算

	p->Scell_log	= Scell_log;
	p->Scell_max	= (255 << Scell_log);

	memcpy(p->Stab, stab, sizeof(p->Stab));
}

static inline void rbfgrad_restart(struct rbfgrad_parms *p)
{
	//待定？？？？
}


/*-------------------------------------------------*/

enum {
	RBFGRAD_BELOW_PROB,
	RBFGRAD_ABOVE_PROB,
};

static inline int rbfgrad_cmp_prob(struct rbfgrad_parms *p)
{
	int p_random,current_p;
	p_random = abs(net_random());

	kernel_fpu_begin();//为了支持浮点运算
	//p->p_k will be written by another thread, so when reading it's value in a diffent thread, "current_p_sem" should be set
	current_p = (int)(p->p_k*RAND_MAX);
	kernel_fpu_end();//为了支持浮点运算
		
	if ( p_random < current_p){
		return RBFGRAD_BELOW_PROB;
	}
	else{
		return RBFGRAD_ABOVE_PROB;
	}
}

enum {
	RBFGRAD_DONT_MARK,
	RBFGRAD_PROB_MARK,
};

static inline int rbfgrad_action(struct rbfgrad_parms *p)
{
	switch (rbfgrad_cmp_prob(p)) {
		case RBFGRAD_ABOVE_PROB:
			return RBFGRAD_DONT_MARK;

		case RBFGRAD_BELOW_PROB:
			return RBFGRAD_PROB_MARK;
	}

	BUG();
	return RBFGRAD_DONT_MARK;
}

#endif
