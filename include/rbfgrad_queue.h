/*qjl*/

#define QUEUE_SHOW_MAX 30000
#define SAM_NUM  10
#define UNIT_NUM  6

struct queue_show{
           int numbers;
           __u32 length;
		   int mark_type; //RBFGRAD_DONT_MARK or RBFGRAD_PROB_MARK
		   long long jacobian;
		   long long NetOut;
		   long long p;
		   long long kp;
		   long long ki;
		   long long kd;
		   long long SamIn[SAM_NUM];
		   long long SamOut[SAM_NUM];
		   long long proba[SAM_NUM];
		   int queue_len[SAM_NUM];
			int  e_k;
			int  e_k_1;
			int  e_k_2;
		   //long long w[UNIT_NUM];
		   //long long delta[UNIT_NUM];
		   //long long c[UNIT_NUM][SAM_NUM];
};
extern struct queue_show queue_show_base_rbfgrad[QUEUE_SHOW_MAX];
extern int array_element_rbfgrad;
extern struct queue_show queue_show_base1[30000];
extern int array_element1;
