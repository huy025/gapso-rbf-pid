#ifndef _TP_RBFGRAD_VARS_TRACE_H
#define _TP_RBFGRAD_VARS_TRACE_H

#include <linux/tracepoint.h>
#include <net/rbfgrad.h>
struct trace_rbfgrad_parms {
	/* Parameters */
	int 	sampl_period;   //采样时间

	/*u32		Scell_max;
	u8		Scell_log;
	u8		Stab[RBFGRAD_STAB_SIZE];*/

	/* Parameters */
	//队列控制
	int q_ref;//参考队列长度
	double p_max;//最大丢弃概率
	//PID
	double eta_p;//比例 学习速率
	double eta_i;//积分 学习速率
	double eta_d;//微分 学习速率
	//RBF
	int n;
	int m;
	double alpha;
	double eta;
		
	/* Variables */
	//队列控制
	double	p_k;	/* Packet current marking probability */
	double p_k_1; /* Packet last marking probability */
	   double e_k;
	double e_k_1;
	double e_k_2;
	//PID
	double kp_k;
	double kp_k_1;
	double ki_k;
	double ki_k_1;
	double kd_k;
	double kd_k_1;
	//RBF
/*	double* w_k;
	double* w_k_1;
	double* w_k_2;
	double* c_k;
	double* c_k_1;
	double* c_k_2;
	double* delta_k;
	double* delta_k_1;
	double* delta_k_2;*/
};
DECLARE_TRACE(rbfgrad_vars_output,
	TP_PROTO(struct trace_rbfgrad_parms *parms),
	TP_ARGS(parms));



#endif
