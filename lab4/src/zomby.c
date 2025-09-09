#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        printf("Дочерний PID: %d - завершается сразу\n", getpid());
        exit(0);
    } else {
        // Родительский процесс
        printf("Родительский PID: %d\n", getpid());
        printf("Дочерний PID: %d стал зомби\n", pid);
        printf("Родитель НЕ вызывает wait() и спит вечно...\n");
        printf("Зомби останется в системе пока родитель не завершится\n");
        
        while(1) {
            sleep(10); 
        }
    }
    
    return 0;
}