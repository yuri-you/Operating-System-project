#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h> /* high-res timers */
#include <sched.h> /* for std. scheduling system calls */
int sched_slice(){
    int number;
    int policy;
    struct timespec t;
    pid_t pid;
    printf("Input the process id(PID):");
    scanf("%d",&number);
    pid=number;
    printf("The scheduler method of this process is ");
    policy=sched_getscheduler(pid);
    switch(policy){
        case 0:printf("NORMAL\n");return 0;
        case 1:printf("FIFO\n");return 0;
        case 2:{
            printf("RR\n");
            printf("The slice of this process is ");
            if(sched_rr_get_interval(pid,&t)!=0)return -1;
            printf("%ld milisecond\n",t.tv_nsec/1000000);
            return 0;
        }
        case 6:{
            printf("WRR\n");
            printf("The slice of this process is ");
            if(sched_rr_get_interval(pid,&t)!=0)return -1;
            printf("%ld milisecond\n",t.tv_nsec/1000000);
        }
        default:return -2;
    }

}
int main(){
    char x[100];
    int flag;
    do{
        switch(sched_slice()){
            case 0:break;
            case -1:printf("Interval Read Error!\n");return -1;
            return -2;printf("Policy Error!\n");return -1;
        }
        printf("Do you want to continue?(y/n):");
        do{
        scanf("%s",&x);
        if(strcmp(x,"y")==0 ||strcmp(x,"Y")==0||strcmp(x,"yes")==0 ||strcmp(x,"Yes")==0)flag=0;
        else{
            if(strcmp(x,"n")==0 ||strcmp(x,"N")==0||strcmp(x,"no")==0 ||strcmp(x,"No")==0)return 0;
            else printf("Input Error!Please input again:");flag=1;
        }
        }while(flag);
    }while(1);
    return 0;
}