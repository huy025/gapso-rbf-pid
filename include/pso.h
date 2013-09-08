/*************************************************************************
	> File Name: pso.h
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: 2013年05月19日 星期日 20时17分58秒
 ************************************************************************/

#include"exp.h"
#include"fabs.h"
//#include"rand_my1.h"
#include<linux/random.h>
#include<linux/kthread.h>
#include<linux/spinlock.h>
#include<linux/interrupt.h>
#include<linux/workqueue.h>


/* The largest number rand will return (same as INT_MAX).  */
#define	RAND_MAX	2147483647

/* multithread  */
#define THREAD_NUM 10 
struct task_struct *tsk[THREAD_NUM];
atomic_t task_run_count;  
wait_queue_head_t task_all_done_wq_h;
/*workqueue*/
struct workqueue_struct *my_workqueue; 
struct my_struct_t 
{
	struct pso *p;
	double * SSE;
	int which;
	struct work_struct my_work;
};
struct my_struct_t rbf_work[THREAD_NUM];

//spinlock_t spinlock_tsk;
//rand_my1  
//get rand matrix
inline void rand_my1(double *matrix,int col,double min,double max)
{
	int j;
		for(j=0; j<col; j++)
			matrix[j]=(double)((max-min)*random32())/(RAND_MAX+1.0)+min;
}

struct pso{
	int SamNum;
	int UnitNum;
	int InDim;
	double MaxEpoch;
	double E0;
	double* SamIn;
	double* SamOut;
	double* Center;
	double* SP;
	double* W;
	double lrCent;
	double lrSP;
	double lrW;
	
	int ParticleNum;
	double **W_PSO;//to seperate with W
	double **pos;
	double **vel;
	int iw1;
	int iw2;
	int iwe;
	int ac1;
	int ac2;
	int mv;
	int mwav;
	double ergrd;
	double ergrdep;
};

double get_rbf_SSE(struct pso *p,int which)
{

	int SamNum=p->SamNum;   
	int UnitNum=p->UnitNum;   

	double *SamIn=p->SamIn;
	double *SamOut=p->SamOut;
	double *Center=p->Center;
	double *SP=p->SP;
	double *W=(p->W_PSO)[which];

	//for will use
	int epoch;
	int MaxEpoch=p->MaxEpoch;   
	double E0=p->E0;   
	int i,j,k;
	double **AllDist;
	double **SPMat;
	double **UnitOut;
	double *NetOut;
	double *Error;
	double SSE;
	double CentGrad = 0;
	double SPGrad = 0;
	double WGrad = 0;

	double lrCent=p->lrCent;
	double lrSP=p->lrSP;
	double lrW=p->lrW;
	double temp ,temp1;
	AllDist=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	SPMat=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	UnitOut=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	for(i=0;i<UnitNum;i++){
		AllDist[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
		SPMat[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
		UnitOut[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
	}
	NetOut=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
	Error=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);

	//get the output value of NN
	for(epoch=0; epoch<MaxEpoch; epoch++)
	{
		for(i=0;i<UnitNum;i++)
			for(j=0;j<SamNum;j++)
			{
				AllDist[i][j]=fabs(Center[i]-SamIn[j]);
				SPMat[i][j]=SP[i];
				temp = -AllDist[i][j]*AllDist[i][j];
				temp1 = SPMat[i][j]*SPMat[i][j];
				UnitOut[i][j]= exp(temp/temp1);
			}
		
		//get error
		for(j=0; j<SamNum; j++)
		{
			NetOut[j]=0;
			for(k=0; k<UnitNum; k++)
				NetOut[j]=NetOut[j]+W[k]*UnitOut[k][j];
			Error[j]=SamOut[j]-NetOut[j];
		}	
		
		//get SSE
		SSE = 0;
		for(j=0; j<SamNum; j++)
		{
			SSE = SSE + Error[j]*Error[j]; 
		} 

	    if(SSE<E0)
	        break;

	    for(i=0;i<UnitNum;i++)
		{
	        CentGrad = 0;
	        SPGrad = 0;
	        WGrad = 0;
	        for(j=0;j<SamNum;j++)
			{
	            CentGrad=CentGrad+(SamIn[j]-Center[i])*(Error[j]*UnitOut[i][j]*W[i]/(SP[i]*SP[i]));
	            SPGrad=SPGrad+AllDist[i][j]*AllDist[i][j]*Error[j]*UnitOut[i][j]*W[i]/(SP[i]*SP[i]*SP[i]);
	            WGrad=WGrad+Error[j]*UnitOut[i][j];
			}
	        
	        Center[i]=Center[i]+lrCent*CentGrad;
	        SP[i]=SP[i]+lrSP*SPGrad;
	        W[i]=W[i]+lrW*WGrad;
		}
	}

	//free space
	for(i=0;i<UnitNum;i++){
		kfree(AllDist[i]);
		kfree(SPMat[i]);
		kfree(UnitOut[i]);
	}
	kfree(AllDist);
	kfree(SPMat);
	kfree(UnitOut);

	kfree(NetOut);
	kfree(Error);

	
	return SSE;
}



int get_rbf_SSE_work_callback_func(struct work_struct *work)
{
	struct my_struct_t* data = container_of(work,struct my_struct_t,my_work);
	struct pso *p =data->p;
	int which = data->which; 
	double SSE;

	int SamNum=p->SamNum;   
	int UnitNum=p->UnitNum;   

	double *SamIn=p->SamIn;
	double *SamOut=p->SamOut;
	double *Center=p->Center;
	double *SP=p->SP;
	double *W=(p->W_PSO)[which];

	//for will use
	int epoch;
	int MaxEpoch=p->MaxEpoch;   
	double E0=p->E0;   
	int i,j,k;
	double **AllDist;
	double **SPMat;
	double **UnitOut;
	double *NetOut;
	double *Error;
	double CentGrad = 0;
	double SPGrad = 0;
	double WGrad = 0;

	double lrCent=p->lrCent;
	double lrSP=p->lrSP;
	double lrW=p->lrW;

	double temp ,temp1;
	AllDist=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	//check space allocate
	if(IS_ERR_OR_NULL(AllDist))
		printk("<1>AllDist is null or error");
	SPMat=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	//check space allocate
	if(IS_ERR_OR_NULL(SPMat))
		printk("<1>SPMat is null or error");
	UnitOut=(double **)kmalloc(sizeof(double *)*UnitNum, GFP_ATOMIC);
	//check space allocate
	if(IS_ERR_OR_NULL(UnitOut))
		printk("<1>UnitOut is null or error");
	for(i=0;i<UnitNum;i++){
		AllDist[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
		if(IS_ERR_OR_NULL(AllDist[i]))
			printk("<1>AllDist[%d] is null or error",i);
		SPMat[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
		if(IS_ERR_OR_NULL(SPMat[i]))
			printk("<1>SPMat[%d] is null or error",i);
		UnitOut[i]=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
		if(IS_ERR_OR_NULL(UnitOut[i]))
			printk("<1>UnitOut[%d] is null or error",i);
	}
	NetOut=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);
	Error=(double *)kmalloc(sizeof(double)*SamNum, GFP_ATOMIC);

	//get the output value of NN
	for(epoch=0; epoch<1; epoch++)
	{
		for(i=0;i<UnitNum;i++)
			for(j=0;j<SamNum;j++)
			{
				AllDist[i][j]=fabs(Center[i]-SamIn[j]);
				SPMat[i][j]=SP[i];
				temp = -AllDist[i][j]*AllDist[i][j];
				temp1 = SPMat[i][j]*SPMat[i][j];
				UnitOut[i][j]= exp(temp/temp1);
			}
		
		//get error
		for(j=0; j<SamNum; j++)
		{
			NetOut[j]=0;
			for(k=0; k<UnitNum; k++)
				NetOut[j]=NetOut[j]+W[k]*UnitOut[k][j];
			Error[j]=SamOut[j]-NetOut[j];
		}	
		
		//get SSE
		SSE = 0;
		for(j=0; j<SamNum; j++)
		{
			SSE = SSE + Error[j]*Error[j]; 
		} 

	    if(SSE<E0)
	        break;

	    for(i=0;i<UnitNum;i++)
		{
	        CentGrad = 0;
	        SPGrad = 0;
	        WGrad = 0;
	        for(j=0;j<SamNum;j++)
			{
	            CentGrad=CentGrad+(SamIn[j]-Center[i])*(Error[j]*UnitOut[i][j]*W[i]/(SP[i]*SP[i]));
	            SPGrad=SPGrad+AllDist[i][j]*AllDist[i][j]*Error[j]*UnitOut[i][j]*W[i]/(SP[i]*SP[i]*SP[i]);
	            WGrad=WGrad+Error[j]*UnitOut[i][j];
			}
	        
	        Center[i]=Center[i]+lrCent*CentGrad;
	        SP[i]=SP[i]+lrSP*SPGrad;
	        W[i]=W[i]+lrW*WGrad;
		}
	}
	//free space
	for(i=0;i<UnitNum;i++){
		kfree(AllDist[i]);
		kfree(SPMat[i]);
		kfree(UnitOut[i]);
	}
	kfree(AllDist);
	kfree(SPMat);
	kfree(UnitOut);

	kfree(NetOut);
	kfree(Error);

	((struct my_struct_t*)data)->SSE[which]=SSE;

	//printk("<1>%ld",SSE*10000);

	//atomic var task_run_count-1 and test if task_run_count=0
	//spin_lock(&spinlock_tsk);
	if(atomic_dec_and_test(&task_run_count))
	{	//wake up task_all_done_wq_h	
		wake_up_interruptible(&task_all_done_wq_h);
	//	wake_up(&task_all_done_wq_h);
//		printk("<1>wakeup");
	}
	//spin_unlock(&spinlock_tsk);
	return 0;
}

int pso_learn(struct pso *p)
{

	int i,j,k,index,err;
	int MaxEpoch = p->MaxEpoch;
	int ParticleNum = p->ParticleNum;
	int UnitNum = p->UnitNum;
	double *SSE;
	double *pbestval;
	double **pbest;
	double gbestval;
	double *gbest;
	double *tr;
	double *iwt;
	int te;
	double rannum1,rannum2,tmp1;
	int cnt2;

	//the below is from struct pso
	double **pos=p->pos;
	double **vel=p->vel;
	int iw1 =p->iw1;
	int iw2 =p->iw2;
	int iwe =p->iwe;
	int ac1 =p->ac1;
	int ac2 =p->ac2;
	int mv =p->mv;
	double ergrd = p->ergrd;
	double ergrdep = p->ergrdep;

	SSE=(double *)kmalloc(sizeof(double)*ParticleNum, GFP_ATOMIC);
	pbestval=(double *)kmalloc(sizeof(double)*ParticleNum, GFP_ATOMIC);
	pbest=(double **)kmalloc(sizeof(double *)*ParticleNum, GFP_ATOMIC);
	for(i=0;i<ParticleNum;i++)
		pbest[i]=(double *)kmalloc(sizeof(double)*UnitNum, GFP_ATOMIC);
	gbest=(double *)kmalloc(sizeof(double)*UnitNum, GFP_ATOMIC);
	//warning! If the MaxEpoch is too large, the next two array will blow
	tr=(double *)kmalloc(sizeof(double)*MaxEpoch, GFP_ATOMIC);
	iwt=(double *)kmalloc(sizeof(double)*MaxEpoch, GFP_ATOMIC);

	//rand_my1(p->Center,UnitNum,-4,4); 
	//rand_my1(p->SP,UnitNum,0.1,0.3); 
	//rand_my1(p->W,UnitNum,-0.1,0.1); 

	//check space allocate
	if(IS_ERR_OR_NULL(pbest))
		printk("<1>pbest is null or error");
	for(i=0;i<ParticleNum;i++)
	{
		if(IS_ERR_OR_NULL(pbest[i]))
			printk("<1>pbest[%d] is null or error",i);
	}
	
	//printk("<1>pso_learn");
	
	//initial pbest positions vals
	for(i=0;i<ParticleNum;i++)
		for(j=0;j<UnitNum;j++)
			pbest[i][j]=pos[i][j];

	//create my workqueue
	my_workqueue = create_workqueue("rbf_workqueue");

	for(i=0;i<ParticleNum;i++)
	{
		rbf_work[i].p = p;
		rbf_work[i].SSE = SSE;
		rbf_work[i].which = i;
		INIT_WORK(&(rbf_work[i].my_work),get_rbf_SSE_work_callback_func);
		/*
		if(IS_ERR_OR_NULL([i])){
      			printk("<1>Unableto init work.\n");
      			err= PTR_ERR(my_work[i]);
      			my_work[i] = NULL;
      	}
		*/
	}

	//put way to get pbestvals here
	//use multi thread
	atomic_set(&task_run_count,THREAD_NUM);
	init_waitqueue_head(&task_all_done_wq_h);	

	//spin_lock(&spinlock_tsk);
	for(i=0; i<ParticleNum; i++){
 		queue_work(my_workqueue,&(rbf_work[i].my_work));//use W[i],SSE[i]
		//printk("<1>tsk[%d]",i);
	}

	wait_event_interruptible(task_all_done_wq_h,atomic_sub_and_test(0,&task_run_count));
	/*if(!atomic_sub_and_test(0,&task_run_count)){
		printk("<1>task_run_count:%d",task_run_count);
		sleep_on(&task_all_done_wq_h);
	}*/ 
	//spin_unlock(&spinlock_tsk);
	//spin_lock_init(&spinlock_tsk);
	
	//printk("<1>wake");

	for(i=0; i<ParticleNum; i++){
	//	get_rbf_SSE(p,i);
		pbestval[i] = SSE[i];
	}

	//assign initial gbest here also 
	gbestval = pbestval[0];
	index = 0;
	for(j=0;j<ParticleNum;j++)
	{
		if(gbestval>pbestval[j])
		{
			gbestval = pbestval[j];
			index = j;
		}
	}
	for(k=0;k<UnitNum;k++)	
		gbest[k] = pbest[index][k];
	tr[0] = gbestval; //keep track of global best SSE
	
	
	//----------------main loop begin---------------- 
	for(i=0; i<MaxEpoch; i++)	
	{
		//use multi thread
		atomic_set(&task_run_count,THREAD_NUM);
		init_waitqueue_head(&task_all_done_wq_h);	
	
		//spin_lock(&spinlock_tsk);
		for(i=0; i<ParticleNum; i++){
	 		queue_work(my_workqueue,&(rbf_work[i].my_work));//use W[i],SSE[i]
			//printk("<1>tsk[%d]",i);
		}
		wait_event_interruptible(task_all_done_wq_h,atomic_sub_and_test(0,&task_run_count));
		//printk("<1>wake");

		for(j=0; j<ParticleNum; j++)
		{
			//SSE[j] = get_rbf_SSE(p,j);//use Wj
			//update pbest
			if(pbestval[j]>SSE[j])
			{
				pbestval[j]=SSE[j];
				for(k=0;k<UnitNum;k++)	
					pbest[j][k] = pos[j][k];
			}

		}
		//update gbest
		gbestval = pbestval[0];
		index = 0;
		for(j=0;j<ParticleNum;j++)
		{
			if(gbestval>pbestval[j])
			{
				gbestval = pbestval[j];
				index = j;
			}
		}
		for(k=0;k<UnitNum;k++)	
			gbest[k] = pbest[index][k];
		
		tr[i+1] = gbestval; //keep track of global best SSE
		te = i;//this will return the epoch number to calling program when done
		
		//get inertia weight ,just a linear funct w.r.t epoch parameter iwe
		if(i<iwe)
			iwt[i]=((iw2-iw1)/(iwe-1))*(i)+iw1;
		else
			iwt[i]=iw2;

		//check space allocate
		if(IS_ERR_OR_NULL(pbest))
		{
			printk("<1>pbest is null or error");
			return 0;
		}
		for(i=0;i<ParticleNum;i++)
		{
			if(IS_ERR_OR_NULL(pbest[i]))
			{
				printk("<1>pbest[%d] is null or error",i);
				return 0;
			}
		}
		//this for loop is the heart of the PSO algorithm,updates position an velocity across dimension D
		for(j=0; j<ParticleNum; j++)
		{
			for(k=0; k<UnitNum; k++)
			{
				rannum1 = random32()/(RAND_MAX+1.0);
				rannum2 = random32()/(RAND_MAX+1.0);

				//update velocity for each dimension of each particle
				vel[j][k] = iwt[i]*vel[j][k] + ac1 *rannum1 * (pbest[j][k] - pos[j][k]) + ac2 *rannum2 * (gbest[k] - pos[j][k]);

				if((vel[j][k])>mv)
					vel[j][k]=mv;
				if((vel[j][k])<-mv)
					vel[j][k]=-mv;
				
			}
			//update position for each particle
			for(k=0; k<UnitNum; k++)
				pos[j][k] = pos[j][k] + vel[j][k]; 
		}

		//check for stopping criterion based on speed of convergence to desired error
		tmp1= abs(tr[i]-gbestval);
		if(tmp1>ergrd)
		{
			cnt2=0;
		}
		else
		{
			cnt2++;
			if(cnt2>=ergrdep)
				break;
		}

	}
	//----------------main loop end---------------- 
	//remove_wait_queue(&task_all_done_wq_h,&pso_wq);

	//update W (ponter in struct pso , it's value is the same as "w_k" in rbfgrad_parms)
	for(j=0;j<UnitNum;j++)
		p->W[j] = gbest[j];	

	//free space
	for(i=0;i<ParticleNum;i++){
		if(!IS_ERR_OR_NULL(pbest[i]))
			kfree(pbest[i]);
	}
	if(!IS_ERR_OR_NULL(pbest))
		kfree(pbest);

	//printk("<1>-------------free done-------------");

	kfree(SSE);
	kfree(pbestval);
	kfree(gbest);
	kfree(tr);
	kfree(iwt);
	return 1;
}

