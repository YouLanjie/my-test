#include <stdio.h>
#include <time.h>
#include <sys/time.h>

int main() {
    time_t timep;
    struct tm *p;
    time (&timep);
    p=gmtime(&timep);
    printf("%d/",1900+p->tm_year);/*获取当前年份,从1900开始，所以要加1900*/
    printf("%d/",1+p->tm_mon);/*获取当前月份,范围是0-11,所以要加1*/
    printf("%d ",p->tm_mday);/*获取当前月份日数,范围是1-31*/
    printf("%d:",8+p->tm_hour);/*获取当前时,这里获取西方的时间,刚好相差八个小时*/
    printf("%d:",p->tm_min); /*获取当前分*/
    printf("%d\n",p->tm_sec); /*获取当前秒*/

    struct timeval t;
    gettimeofday(&t, NULL);
    printf("%ld.%06ld\n", time(NULL), t.tv_usec);
}
