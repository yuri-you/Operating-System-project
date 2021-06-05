#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h> 
#include <sched.h> 
int main() {
    int time1;
    int num_pro;
    int k;
    int policy;
    int success;
    int h,i,j;
    int s=1;
    int flag=0;
    int role=0;
    int whole;
    pid_t pid;
    time_t t1,t2;
    struct sched_param param;
    param.sched_priority=0;
    printf("The number of cycles in each process=");
    scanf("%d",&time1);
    printf("The number of the processes=");
    scanf("%d",&num_pro);
    printf("The policy you want to change into(0-NORMAL,1-FIFO,2-RR,6-WRR,7-PRR):");
    scanf("%d",&policy);
    if(policy!=6&&policy!=7){
        printf("The priority=");
        scanf("%d",&param.sched_priority);
    }
    time(&t1);
    for(k=1;k<num_pro;++k){
        pid=fork();
        if(pid!=0){//parent
            if(policy==7){
            if(role%10==9)param.sched_priority=0;
            }
            else param.sched_priority=5;
            success=sched_setscheduler(pid,policy,&param);
            break;
        }
        else{
            flag=1;
            ++role;
        }
    }
    if(role%9==0)time1*=3;
    for(h=0;h<time1;++h){
    for(i=0;i<time1;++i){
        for(j=0;j<time1;++j){
            s*=6;
            s/=6;
        }
    }
    }
    wait(NULL);
    if(flag==0){
        time(&t2);
        printf("%ld\n",t2-t1);
    }
return 0;
}