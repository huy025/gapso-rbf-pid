#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/pkt_sched.h>
#define MAX 10000
int main(void)
{
	int fd;
	FILE* fp_proc;
	FILE* fp_vars;
	size_t nbytes;
	ssize_t bytes_read;
	char *path_proc = "/proc/rbfgrad_vars";
	char *path_vars = "vars";
	char s[20];
	char *ret;
	int count_total;
	int count_line;
	double td;
	long long tll;
	fp_proc = fopen(path_proc, "r");
	fp_vars = fopen(path_vars, "w");
	if(fp_proc == NULL || fp_vars == NULL)
	{
		printf("\n unable to read on %s and %s\n", path_proc, path_vars);
		return 1;
	}
	count_total = 0;
	count_line = 0;
	while(!feof(fp_proc)){
		if((ret=fgets(s, MAX, fp_proc))!=NULL){
			count_line ++;
			count_total ++;
			switch(count_line){
			case 1:
			case 2:
			case 3:
			case 4:	
            case 5:
            case 6:
			case 7:
			case 8:
            case 9:
			case 10:	
				tll = atoll(s);
				td = *((double *)&tll);
				fprintf(fp_vars, "%.16f\t", td);
				break;
            case 11:
				tll = atoll(s);
				td = *((double *)&tll);
				fprintf(fp_vars, "%.16f\n", td);
				count_line = 0;
				break;
			}
		}
	}
	printf("totol:%d", count_total);
	fclose(fp_proc);
	fclose(fp_vars);
}

