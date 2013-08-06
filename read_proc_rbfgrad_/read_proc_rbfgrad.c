#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <linux/pkt_sched.h>
#define MAX 10000
#define ENISL 10 //ELEMENTS_IN_SIGLE_LINE
int main(void)
{
	int fd;
	FILE* fp_proc;
	FILE* fp_params;
	size_t nbytes;
	ssize_t bytes_read;
	char *path_proc = "/proc/rbfgrad";
	char *path_params = "params";
	char s[20];
	char *ret;
	int count_total;
	int count_line;
	double td;
	long long tll;
	fp_proc = fopen(path_proc, "r");
	fp_params = fopen(path_params, "w");
	if(fp_proc == NULL || fp_params == NULL)
	{
		printf("\n unable to read on %s and %s\n", path_proc, path_params);
		return 1;
	}
	//fd = fileno(fp_proc);
	//bytes_read = read(fd, &buf, nbytes);
	count_total = 0;
	count_line = 0;
	while(!feof(fp_proc)){
		if((ret=fgets(s, MAX, fp_proc))!=NULL){
			count_line ++;
			count_total ++;
			switch(count_line){
			case 1:
			case 2:
			case 7:
			case 8:
				fprintf(fp_params, "%d\t", atoi(s));
				break;
			case 3:
			case 4:	
            case 5:
            case 6:
            case 9:
				tll = atoll(s);
				td = *((double *)&tll);
				fprintf(fp_params, "%.16f\t", td);
				break;
            case 10:
				tll = atoll(s);
				td = *((double *)&tll);
				fprintf(fp_params, "%.16f\n", td);
				count_line = 0;
				break;
			}
		}
	}
	printf("totol:%d", count_total);
	fclose(fp_proc);
	fclose(fp_params);
}

