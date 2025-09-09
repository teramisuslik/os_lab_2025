#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <getopt.h>

typedef struct {
    int start;
    int end;
    int mod;
    long long* result;
    pthread_mutex_t* mutex;
} ThreadData;

void* compute_partial_factorial(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    long long partial_result = 1;
    
    for (int i = data->start; i <= data->end; i++) {
        partial_result = (partial_result * i) % data->mod;
    }
    
    pthread_mutex_lock(data->mutex);
    *(data->result) = (*(data->result) * partial_result) % data->mod;
    pthread_mutex_unlock(data->mutex);
    
    return NULL;
}

int main(int argc, char* argv[]) {
    int k = 0;
    int pnum = 1;
    int mod = 0;
    
    static struct option options[] = {
        {"k", required_argument, 0, 'k'},
        {"pnum", required_argument, 0, 'p'},
        {"mod", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "k:p:m:", options, &option_index)) != -1) {
        switch (c) {
            case 'k':
                k = atoi(optarg);
                break;
            case 'p':
                pnum = atoi(optarg);
                break;
            case 'm':
                mod = atoi(optarg);
                break;
            case '?':
                printf("Unknown option\n");
                return 1;
        }
    }
    
    if (k <= 0 || pnum <= 0 || mod <= 0) {
        printf("Usage: %s --k <number> --pnum <threads> --mod <modulus>\n", argv[0]);
        return 1;
    }
    
    pthread_t* threads = malloc(pnum * sizeof(pthread_t));
    ThreadData* thread_data = malloc(pnum * sizeof(ThreadData));
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    long long result = 1;
    
    int chunk_size = k / pnum;
    int remainder = k % pnum;
    int current_start = 1;
    
    for (int i = 0; i < pnum; i++) {
        int chunk = chunk_size;
        if (i < remainder) {
            chunk++;
        }
        
        thread_data[i].start = current_start;
        thread_data[i].end = current_start + chunk - 1;
        thread_data[i].mod = mod;
        thread_data[i].result = &result;
        thread_data[i].mutex = &mutex;
        
        current_start += chunk;
        
        if (pthread_create(&threads[i], NULL, compute_partial_factorial, &thread_data[i]) != 0) {
            perror("pthread_create failed");
            return 1;
        }
    }
    
    for (int i = 0; i < pnum; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("%d! mod %d = %lld\n", k, mod, result);
    
    free(threads);
    free(thread_data);
    pthread_mutex_destroy(&mutex);
    
    return 0;
}