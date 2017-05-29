#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <wiringPi.h>
#include <signal.h>
#include <err.h>
#include <string.h>
#include <errno.h>

#define PWM_PIN 18

#define PWM_CLOCK_JJY40 48
#define PWM_CLOCK_JJY60 32

#define RANGE 10

#define JJY_ON  5
#define JJY_OFF 0

#define true  (1)
#define false (0)

#define PIDFILE ("/var/run/jjyRepeater.pid")

volatile sig_atomic_t sig_flag = 0;

int init_pwm()
{
    if (wiringPiSetupGpio() == -1) return false;

    pinMode(PWM_PIN,PWM_OUTPUT);

    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(PWM_CLOCK_JJY40);
    pwmSetRange(RANGE);

    pwmWrite(PWM_PIN,JJY_OFF);


    return true;
}

void finish()
{
    pwmWrite(PWM_PIN,JJY_OFF);
    pinMode(PWM_PIN,INPUT);
}

static inline void send_bit(int bit)
{
    struct timeval now_u;
    gettimeofday(&now_u,NULL);

    if(sig_flag != 0) return;

    delayMicroseconds(1000000 - now_u.tv_usec);

    if(sig_flag != 0) return;

    pwmWrite(PWM_PIN,JJY_ON);
    switch(bit)
    {
        case -1: delayMicroseconds(200000); break;
        case  0: delayMicroseconds(800000); break;
        case  1: delayMicroseconds(500000); break;
    }
    pwmWrite(PWM_PIN,JJY_OFF);
}

void mkjjyCode(int *jjyCode)
{
    time_t     now;
    struct tm *now_st;

    now = time(NULL);
    now = now + 60;
    now_st = localtime(&now);

    jjyCode[ 0] = -1;
    jjyCode[ 1] = (((now_st->tm_min  / 10 ) & 0x4) >> 2);
    jjyCode[ 2] = (((now_st->tm_min  / 10 ) & 0x2) >> 1);
    jjyCode[ 3] =  ((now_st->tm_min  / 10 ) & 0x1)      ;
    jjyCode[ 4] = 0;
    jjyCode[ 5] = (((now_st->tm_min  % 10 ) & 0x8) >> 3);
    jjyCode[ 6] = (((now_st->tm_min  % 10 ) & 0x4) >> 2);
    jjyCode[ 7] = (((now_st->tm_min  % 10 ) & 0x2) >> 1);
    jjyCode[ 8] =  ((now_st->tm_min  % 10 ) & 0x1)      ;
    jjyCode[ 9] = -1;
    jjyCode[10] = 0;
    jjyCode[11] = 0;
    jjyCode[12] = (((now_st->tm_hour / 10 ) & 0x2) >> 1);
    jjyCode[13] =  ((now_st->tm_hour / 10 ) & 0x1)      ;
    jjyCode[14] = 0;
    jjyCode[15] = (((now_st->tm_hour % 10 ) & 0x8) >> 3);
    jjyCode[16] = (((now_st->tm_hour % 10 ) & 0x4) >> 2);
    jjyCode[17] = (((now_st->tm_hour % 10 ) & 0x2) >> 1);
    jjyCode[18] =  ((now_st->tm_hour % 10 ) & 0x1)      ;
    jjyCode[19] = -1;
    jjyCode[20] = 0;
    jjyCode[21] = 0;
    jjyCode[22] = ((((now_st->tm_yday + 1) / 100) & 0x2) >> 1);
    jjyCode[23] =  (((now_st->tm_yday + 1) / 100) & 0x1)      ;
    jjyCode[24] = 0;
    jjyCode[25] = (((((now_st->tm_yday + 1) / 10) % 10) & 0x8) >> 3); 
    jjyCode[26] = (((((now_st->tm_yday + 1) / 10) % 10) & 0x4) >> 2);
    jjyCode[27] = (((((now_st->tm_yday + 1) / 10) % 10) & 0x2) >> 1);
    jjyCode[28] =  ((((now_st->tm_yday + 1) / 10) % 10) & 0x1)      ;
    jjyCode[29] = -1;
    jjyCode[30] = ((((now_st->tm_yday + 1) % 10) & 0x8) >> 3);
    jjyCode[31] = ((((now_st->tm_yday + 1) % 10) & 0x4) >> 2);
    jjyCode[32] = ((((now_st->tm_yday + 1) % 10) & 0x2) >> 1);
    jjyCode[33] =  (((now_st->tm_yday + 1) % 10) & 0x1)      ;
    jjyCode[34] = 0;
    jjyCode[35] = 0;
    jjyCode[36] = jjyCode[12] ^ jjyCode[13] ^ jjyCode[15] ^ jjyCode[16] ^ jjyCode[17] ^ jjyCode[18];
    jjyCode[37] = jjyCode[ 1] ^ jjyCode[ 2] ^ jjyCode[ 3] ^ jjyCode[ 5] ^ jjyCode[ 6] ^ jjyCode[ 7] ^ jjyCode[ 8];
    jjyCode[38] = 0;
    jjyCode[39] = -1;
    jjyCode[40] = 0;
    jjyCode[41] = ((((now_st->tm_year % 100) / 10) & 0x8) >> 3);
    jjyCode[42] = ((((now_st->tm_year % 100) / 10) & 0x4) >> 2);
    jjyCode[43] = ((((now_st->tm_year % 100) / 10) & 0x2) >> 1);
    jjyCode[44] =  (((now_st->tm_year % 100) / 10) & 0x1);
    jjyCode[45] = (((now_st->tm_year  % 10) & 0x8) >> 3);
    jjyCode[46] = (((now_st->tm_year  % 10) & 0x4) >> 2);
    jjyCode[47] = (((now_st->tm_year  % 10) & 0x2) >> 1);
    jjyCode[48] =  ((now_st->tm_year  % 10) & 0x1);
    jjyCode[49] = -1;
    jjyCode[50] = (((now_st->tm_wday      ) & 0x4) >> 2);
    jjyCode[51] = (((now_st->tm_wday      ) & 0x2) >> 1);
    jjyCode[52] =  ((now_st->tm_wday      ) & 0x1)      ;
    jjyCode[53] = 0;
    jjyCode[54] = 0;
    jjyCode[55] = 0;
    jjyCode[56] = 0;
    jjyCode[57] = 0;
    jjyCode[58] = 0;
    jjyCode[59] = -1;
}

void jjyProcCheck()
{
    pid_t pid;
    FILE* fp;
    const char *pid_file = PIDFILE;

    fp = fopen(pid_file,"r");
    if(fp != NULL)
    {
        fscanf( fp , "%ld" , &pid );
        fclose(fp);
        kill(pid,SIGTERM);
        sleep(1);
    }

    fp = fopen(pid_file,"w");
    if(fp == NULL) err(1,"can not create pid file: %s", pid_file);
    fprintf(fp, "%ld\n", (long) getpid() );
    fclose(fp);
}

void getsignal(int sig)
{
    switch(sig)
    {
        case SIGINT:
        case SIGTERM:
        case SIGQUIT:
            sig_flag = 4;
            break;
        case SIGTSTP:
            sig_flag = 3;
            break;
        case SIGCONT:
            sig_flag = 2;
            break;
     }
}

void wait_start()
{
    int wait_time;

    struct timeval now_u;
    struct tm *now_st;

    gettimeofday(&now_u,NULL);
    now_st = localtime(&now_u.tv_sec);
    wait_time = (59.1*1000*1000) - (now_st->tm_sec*1000*1000) - now_u.tv_usec;
    if(wait_time > 0) delayMicroseconds(wait_time);

}


int main(int argc,char *argv[])
{
    int jjyCode[60];
    unsigned int i,wait_time;

    struct sigaction act;

    struct timeval now_u;
    struct tm *now_st;

    const char *pid_file = PIDFILE;

    memset(&act,0,sizeof(act));
    act.sa_handler = getsignal;
    act.sa_flags   = SA_RESETHAND;


    if(getuid() != 0)                     err(1,"Require ROOT!\n");
    jjyProcCheck();
    if(sigaction(SIGINT ,&act,NULL) != 0) err(-1,"Error: sigaction(SIGINT)" );
    if(sigaction(SIGTERM,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTERM)");
    if(sigaction(SIGQUIT,&act,NULL) != 0) err(-1,"Error: sigaction(SIGQUIT)");
    if(sigaction(SIGTSTP,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTSTP)");
    if(sigaction(SIGCONT,&act,NULL) != 0) err(-1,"Error: sigaction(SIGCOUT)");
    if(sigaction(SIGHUP ,&act,NULL) != 0) err(-1,"Error: sigaction(SIGHUP)" );


    if(!init_pwm())        errno = ENOTTY,err(-1,"pwm_init() error");

    while(sig_flag < 4)
    {

        if(sig_flag == 3){
            kill(getpid(),SIGTSTP);
        }

        if(sig_flag == 2)
        {

            kill(getpid(),SIGCONT);

            if(sigaction(SIGTSTP,&act,NULL) != 0)
            {
                finish();
                err(-2,"Error: set sigaction(SIGTSTP)");
            }

            if(sigaction(SIGCONT,&act,NULL) != 0)
            {
                finish();
                err(-2,"Error: set sigaction(SIGCONT)");
            }

            sig_flag = 0;
        }

        wait_start();
        mkjjyCode(jjyCode);
        for(i=0;i<60;i++) send_bit(jjyCode[i]);
    }

    finish();

    if(remove(pid_file) != 0) err(1,"Could not Remove PID file");
    
}
