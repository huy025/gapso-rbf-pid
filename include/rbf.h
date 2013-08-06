/*************************************************************************
	> File Name: rbf.c
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: 2013年05月19日 星期日 20时17分58秒
 ************************************************************************/

#include"exp.h"
#include"fabs.h"
#include<linux/random.h>

/* The largest number rand will return (same as INT_MAX).  */
#define	RAND_MAX	2147483647

/*
struct rbf{
	int SamNum;
	int UnitNum;
	double MaxEpoch;
	double E0;
	double* SamIn;
	double* SamOut;
	double** c_k;
	double** c_k_1;
	double** c_k_2;
	double* delta_k;
	double* delta_k_1;
	double* delta_k_2;
	double* w_k;
	double* w_k_1;
	double* w_k_2;
	double alpha;
	double eta;
};

int rbf_learn(struct rbf *p)
{
	int SamNum=p->SamNum;   
	int UnitNum=p->UnitNum;   
	int MaxEpoch=p->MaxEpoch;   
	double E0=p->E0;   
	double *SamIn=p->SamIn;
	double *SamOut=p->SamOut;
	double** c_k=p->c_k;
	double** c_k_1=p->c_k_1;
	double** c_k_2=p->c_k_2;
	double *delta_k=p->delta_k;
	double *delta_k_1=p->delta_k_1;
	double *delta_k_2=p->delta_k_2;
	double *w_k=p->w_k;
	double *w_k_1=p->w_k_1;
	double *w_k_2=p->w_k_2;
	double alpha=p->alpha;
	double eta=p->eta;
	
	//for will use
	int epoch;
	int i,j,k;
	double *r;
	double *sum;
	double NetOut;
	double SSE;
	
	r=(double *)kmalloc(sizeof(double)*UnitNum, 0);
	sum = (double *)kmalloc(sizeof(double)*UnitNum, 0);
	if(IS_ERR_OR_NULL(r))
		printk("<1>r is null or error");
	if(IS_ERR_OR_NULL(sum))
		printk("<1>sum is null or error");
	
	for(epoch=0; epoch<MaxEpoch; epoch++)
	{
		NetOut = 0;
	    for(i=0;i<UnitNum;i++){
			sum[i] = 0;
			for(j = 0; j < SamNum; j++)
				sum[i] = sum[i] + (SamIn[j]-c_k[i][j])*(SamIn[j]-c_k[i][j]);//求平方和
				
			r[i] = exp(- sum[i] / (2*delta_k[i]*delta_k[i]));//求隐含层神经元值
	    
			NetOut = NetOut + w_k[i] * r[i];
		}
	    
		SSE = (SamOut[0]-NetOut)*(SamOut[0]-NetOut)/2;

	    if(SSE<E0)
	        break;

		
	    for(i=0;i<UnitNum;i++)
		{
			w_k_2[i]=w_k_1[i];
			w_k_1[i]=w_k[i];
			delta_k_2[i]=delta_k_1[i];
			delta_k_1[i]=delta_k[i];
			for(j=0;j<SamNum;j++)
			{
				c_k_2[i][j]=c_k_1[i][j];
				c_k_1[i][j]=c_k[i][j];
			}
		}

	    for(i=0;i<UnitNum;i++)
		{
			w_k[i]=w_k_1[i]+ \
				   alpha*(w_k_1[i]-w_k_2[i])+ \
				   eta*(SamOut[0]-NetOut);
			delta_k[i]=delta_k_1[i]+ \
					   alpha*(delta_k_1[i]-delta_k_2[i])+ \ 
					   eta*(SamOut[0]-NetOut)*w_k_1[i]*r[i]*sum[i]/(delta_k_1[i]*delta_k_1[i]*delta_k_1[i]);
	        for(j=0;j<SamNum;j++)
			{
				c_k[i][j]=c_k_1[i][j]+ \
						  alpha*(c_k_1[i][j]-c_k_2[i][j])+ \
						  eta*(SamOut[0]-NetOut)*w_k_1[i]*(SamIn[j]-c_k_1[i][j])/(delta_k_1[i]*delta_k_1[i]);
			}
		}
	}

	//printk("<1>learn done");
	
	kfree(r);
	kfree(sum);

	return 1;
}
*/
