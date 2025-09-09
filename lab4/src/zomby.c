#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("дочерний PID %d завершается сразу\n", getpid());
        exit(0);
    } else {
        // Родительский процесс
        printf("родительский PID %d\n", getpid());
        printf("дочерний PID %d стал зомби\n", pid);
        
        while(1) {
            sleep(10); 
        }
    }
    
    return 0;
}