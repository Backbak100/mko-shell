#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#define DELETE 11
#define KEEP 10

int exnum = 0;

void big_sig_handler(struct sigaction);
void sig_handler(int);
void charwrite(char *, int);
struct sigaction sa_init(struct sigaction);
void my_sig_handler(int);


/*int main(void) {
    //charwrite("main started\n", KEEP);

    struct sigaction sa;
    //sa = sa_init(sa);
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;

    while (1) {
        big_sig_handler(sa);
        sleep(30);
    }
    
    charwrite("main ended\n", KEEP);
    return 0;
}*/

void big_sig_handler(struct sigaction sa) {
    for (int i = 0; i < NSIG; i++)
        sigaction(i, &sa, NULL);
}

void sig_handler(int sig) {
    char *str;
    sprintf(str, "sig_handler( %d ) called for ", sig);
    charwrite(str, DELETE);
    switch (sig) {
        case SIGINT:
            charwrite("SIGINT\n", KEEP);
            break;
        case SIGTSTP:
            charwrite("SIGTSTP\n", KEEP);
            break;
        case SIGQUIT:
            charwrite("SIGQUIT\n", KEEP);
            break;
        default:
            charwrite("UNKNOWN\n", KEEP);
            break;
    }
}

void charwrite(char *string, int status) {
    if (status == DELETE)
        write(1, "\r\e[2K", 6);

    unsigned long len = strlen(string);
    unsigned long bytes = write(1, string, len);

    if (bytes == -1) 
        write(2, "Error writing to stdout\n", 25);

}

struct sigaction sa_init(struct sigaction sa) {
    sa.sa_handler = sig_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    return sa;
} //does not work
