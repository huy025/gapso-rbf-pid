/*************************************************************************
	> File Name: transfer.c
	> Author: ma6174
	> Mail: ma6174@163.com 
	> Created Time: 2013年07月19日 星期五 23时06分23秒
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<malloc.h>
#define ROW 20000
#define TEXT_ROW 2
#define COL 12 
#define CUR_MAX 4095

void main(void)
{
int i,j;
	FILE *fp;
	FILE *fpd;
	long long a; 
	double d[ROW][COL];
	char *buffer=(char *)malloc(sizeof(char)*CUR_MAX);
	if(!(fp = fopen("queuedata_rbfgrad.txt","r")))
	{

		printf("haha");
	}
	if(!(fpd = fopen("transfered_data.txt","w")))
	{
		printf("haha");
	}

	//输出文件的形式是以gnuplot为准的，所以第一行为注释，会被gnuplot忽略
	//给第一行加注释，
	fputc('#',fpd);

	for(i=0;i<TEXT_ROW;i++){
		fgets(buffer,sizeof(char)*CUR_MAX,fp);
		if(i==1)
			fputs(buffer,fpd);
	}

	free(buffer);

	for(i=0;i<ROW;i++)
	{
			for(j=0;j<COL;j++)
			{
				fscanf(fp,"%lld",&a);
				switch(j){
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
						d[i][j] = *((double *)(&a));	
						fprintf(fpd,"%.15lf\t",d[i][j]);
						break;
					case 8:
						d[i][j] = *((double *)(&a));	
						fprintf(fpd,"%.15lf\t",d[i][j]*6500);
						//fprintf(fpd,"%.15lf\t",d[i][j]);
						break;
					default:
						fprintf(fpd,"%lld\t",a);
				}
			}

			fprintf(fpd,"\n");
	}
	fclose(fp);
	fclose(fpd);
}
