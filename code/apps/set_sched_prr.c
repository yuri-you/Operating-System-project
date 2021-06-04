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

#define SCHED_NORMAL        0
#define SCHED_FIFO      1
#define SCHED_RR        2
#define SCHED_WRR       6
int change_mode(){
    int number;
    pid_t pid;
    int origin,new;//policy
    int prior;
    int success;
    printf("The id of the process (PID) that you want to modify:");
    scanf("%d",&number);
    pid=number;
    if(!pid){
       printf("Input Error!");
       return -1;
    }
    struct sched_param param;
    param.sched_priority=0;//initial

    printf("The schedule policy of pid=%d processor is ",pid);
    origin=sched_getscheduler(pid);
    switch(origin){
        case 0:printf("NORMAL\n");break;
        case 1:printf("FIFO\n");break;
        case 2:printf("RR\n");break;
        case 6:printf("WRR\n");break;
        case 7:printf("PRR\n");break;
    }
    printf("The policy you want to change into(0-NORMAL,1-FIFO,2-RR,6-WRR,7-PRR):");
    scanf("%d",&new);
    if(new!=6){
        if(new==7)printf("The priority you want to set(0-5):");
        else printf("The priority you want to set:");
        scanf("%d",&prior);
        param.sched_priority=prior;
    }
    if(sched_setscheduler(pid,new,&param)!=0){
        printf("Change fails!\n");
        return -1;
    }
    printf("Successfuly change pid %d to policy ",pid);
    switch(new){
        case 0:printf("NORMAL\n");break;
        case 1:printf("FIFO\n");break;
        case 2:printf("RR\n");break;
        case 6:printf("WRR\n");break;
        case 7:printf("PRR\n");break;
    }
    return 0;
}
int main(){
    char x[100];
    int flag;
    do{
        if(change_mode()<0)break;
        printf("Do you want to continue?(y/n):");
        do{
        scanf("%s",&x);
        if(strcmp(x,"y")==0 ||strcmp(x,"Y")==0||strcmp(x,"yes")==0 ||strcmp(x,"Yes")==0)flag=0;
        else{
            if(strcmp(x,"n")==0 ||strcmp(x,"N")==0||strcmp(x,"no")==0 ||strcmp(x,"No")==0)return 0;
            else printf("Input Error!Please input again:");flag=1;
        }
        }while(flag);
        if(x==0)break;
    }while(1);
    return 0;
}
