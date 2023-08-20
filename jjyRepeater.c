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
#include <pthread.h>

#define PRG_ON_PIN  6
#define PWM_PIN    12
#define PWM_PIN2   13

#define PWM_CLOCK_JJY40 48
#define PWM_CLOCK_JJY60 32

#define RANGE 10

#define JJY_ON  5
#define JJY_OFF 0

#define true  (1)
#define false (0)

#define PIDFILE ("/run/jjyRepeater.pid")
#define NTPFILE ("/tmp/ntp_status")

volatile sig_atomic_t sig_flag   = 0;

unsigned int getNtpStatusFromFile();

int init_pwm()
{
    if (wiringPiSetupGpio() == -1) return false;

    pinMode(PRG_ON_PIN,OUTPUT    );
    pinMode(PWM_PIN   ,PWM_OUTPUT);
    pinMode(PWM_PIN2  ,PWM_OUTPUT);

    pwmSetMode(PWM_MODE_MS);
    pwmSetClock(PWM_CLOCK_JJY40);
    pwmSetRange(RANGE);

    pwmWrite(PWM_PIN ,JJY_OFF);
    pwmWrite(PWM_PIN2,      0);
    digitalWrite(PRG_ON_PIN,HIGH);

    return true;
}

void finish()
{
    pwmWrite(PWM_PIN ,JJY_OFF);
    pwmWrite(PWM_PIN2,      0);
    digitalWrite(PRG_ON_PIN,LOW);

    pinMode(PRG_ON_PIN,INPUT);
    pinMode(PWM_PIN   ,INPUT);
    pinMode(PWM_PIN2  ,INPUT);
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
        case -1: delayMicroseconds(200 * 1000);break;
        case  0: delayMicroseconds(800 * 1000);break;
        case  1: delayMicroseconds(500 * 1000);break;
    }
    pwmWrite(PWM_PIN,JJY_OFF);
}

unsigned int getNtpStatusFromFile()
{
    FILE *fp;
    char *cmdline = "timedatectl status | grep 'System clock synchronized:' | grep 'yes' | wc -l";
    if ((fp=popen(cmdline, "r")) == NULL) {
        return false;
    }

    unsigned int ret;
    while(fscanf(fp, "%d", &ret) != EOF) {}

    pclose(fp);

    return ret;
}

unsigned int wait_ntp_sync(unsigned int interval_sec,unsigned int limit_sec)
{
    unsigned int sec    = 0;
    unsigned int on_off = 0;
    while(!getNtpStatusFromFile())
    {
        if((limit_sec != 0) & (sec > limit_sec))
	{
            pwmWrite(PWM_PIN2,0);
            return false;
        }
        for(int i=0;i<interval_sec;i++)
        {
            if(on_off == 0) on_off = RANGE;
	    else            on_off = 0;
            pwmWrite(PWM_PIN2,on_off);
            sleep(1);
            sec++;
        }
    }
    pwmWrite(PWM_PIN2,0);
    return true;
}

void *ntp_monitor(void *arg)
{
    unsigned int times    = 0;
    unsigned int p2_width = 0;
    unsigned int status;
    int up = true;

    status = getNtpStatusFromFile();
    while(true)
    {
        if (status)
        {
            pwmWrite(PWM_PIN2,p2_width);
            if(up) p2_width++;
            else   p2_width--;
            if(p2_width == 0     ) up = true;
            if(p2_width == RANGE ) up = false;
            usleep(100 * 1000);
            times++;
            if(times == 600)
            {
                status = getNtpStatusFromFile();
	        times = 0;
            }
        } else {
	    status = wait_ntp_sync(30,0);
            times = 0;
        }
    }
    pwmWrite(PWM_PIN2,      0);
    pthread_exit(NULL);
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
    jjyCode[41] = ((((now_st->tm_year / 10) % 10) & 0x8) >> 3);
    jjyCode[42] = ((((now_st->tm_year / 10) % 10) & 0x4) >> 2);
    jjyCode[43] = ((((now_st->tm_year / 10) % 10) & 0x2) >> 1);
    jjyCode[44] =  (((now_st->tm_year / 10) % 10) & 0x1);
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
        case SIGQUIT:
        //case SIGKILL:
        case SIGTERM:
            sig_flag = 6;
            break;
        case SIGHUP:
            sig_flag = 5;
            break;
        //case SIGSTOP:
        case SIGTSTP:
            sig_flag = 4;
            break;
        case SIGCONT:
            sig_flag = 3;
            break;
        case SIGUSR1:
            sig_flag = 2;
            break;
        case SIGUSR2:
            sig_flag = 1;
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

    unsigned int is_running = true;

    struct sigaction act;

    struct timeval now_u;
    struct tm *now_st;

    const char *pid_file = PIDFILE;

    pthread_t ntp_mon;

    memset(&act,0,sizeof(act));
    act.sa_handler = getsignal;
    act.sa_flags   = SA_RESETHAND;

    if(getuid() != 0)                     err( 1,"Require ROOT!\n");
    jjyProcCheck();
    if(sigaction(SIGHUP ,&act,NULL) != 0) err(-1,"Error: sigaction(SIGINT)" );
    if(sigaction(SIGINT ,&act,NULL) != 0) err(-1,"Error: sigaction(SIGINT)" );
    if(sigaction(SIGQUIT,&act,NULL) != 0) err(-1,"Error: sigaction(SIGQUIT)");
    //if(sigaction(SIGKILL,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTERM)");
    if(sigaction(SIGTERM,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTERM)");
    //if(sigaction(SIGSTOP,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTSTP)");
    if(sigaction(SIGTSTP,&act,NULL) != 0) err(-1,"Error: sigaction(SIGTSTP)");
    if(sigaction(SIGCONT,&act,NULL) != 0) err(-1,"Error: sigaction(SIGCOUT)");
    if(sigaction(SIGUSR1,&act,NULL) != 0) err(-1,"Error: sigaction(SIGHUP)" );
    if(sigaction(SIGUSR2,&act,NULL) != 0) err(-1,"Error: sigaction(SIGHUP)" );

    if(!init_pwm())        errno = ENOTTY,err(-1,"pwm_init() error");

    wait_ntp_sync(10,180);

    if(pthread_create(&ntp_mon,NULL,ntp_monitor,NULL) != 0)
                           errno = ENOTTY,err(-1,"ntp_monitor() error");

    while(sig_flag < 6)
    {
        switch(sig_flag)
        {
            case 5:
                pthread_cancel(ntp_mon);
                pthread_join(ntp_mon,NULL);
                finish();
                if(!init_pwm()) errno = ENOTTY,err(-1,"pwm_init() error");
                if(pthread_create(&ntp_mon,NULL,ntp_monitor,NULL) != 0)
                                errno = ENOTTY,err(-1,"ntp_monitor() error");
                sig_flag = 0;
                break;
            case 4:
                if(is_running)
                {
                    pthread_cancel(ntp_mon);
                    pthread_join(ntp_mon,NULL);
                    is_running = false;
                }
                usleep(100 * 1000);
                break;
            case 3:
                if(!is_running)
                {
                    if(pthread_create(&ntp_mon,NULL,ntp_monitor,NULL) != 0)
                        errno = ENOTTY,err(-1,"ntp_monitor() error");
                    is_running = true;
                }
                sig_flag = 0;
                break;
            case 2:
                sig_flag = 0;
                break;
            case 1:
                sig_flag = 0;
                break;
            default:
               wait_start();
               mkjjyCode(jjyCode);
               for(i=0;i<60;i++) send_bit(jjyCode[i]);
               break;
        }
    }

    pthread_cancel(ntp_mon);
    pthread_join(ntp_mon,NULL);
    finish();

    if(remove(pid_file) != 0) err(1,"Could not Remove PID file");
    
}
