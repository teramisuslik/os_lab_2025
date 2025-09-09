#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include <getopt.h>

#include "find_min_max.h"
#include "utils.h"

pid_t *child_pids = NULL;
int child_count = 0;

void timeout_handler(int sig) {
    printf("Timeout reached! Sending SIGKILL to all child processes\n");
    
    if (child_pids != NULL) {
        for (int i = 0; i < child_count; i++) {
            if (child_pids[i] > 0) {
                kill(child_pids[i], SIGKILL);
            }
        }
    }
}

int main(int argc, char **argv) {
  int seed = -1;
  int array_size = -1;
  int pnum = -1;
  int timeout = 0;
  bool with_files = false;

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"seed", required_argument, 0, 0},
                                      {"array_size", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"timeout", required_argument, 0, 't'},
                                      {"by_files", no_argument, 0, 'f'},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "ft:", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
            seed = atoi(optarg);
            if (seed <= 0) {
                printf("seed must be a positive number\n");
                return 1;
            }
            break;
          case 1:
            array_size = atoi(optarg);
            if (array_size <= 0) {
                printf("array_size must be a positive number\n");
                return 1;
            }
            break;
          case 2:
            pnum = atoi(optarg);
            if (pnum <= 0) {
                printf("pnum must be a positive number\n");
                return 1;
            }
            break;
          case 3:
            with_files = true;
            break;

          default:
            printf("Index %d is out of options\n", option_index);
        }
        break;
      case 't':
        timeout = atoi(optarg);
        if (timeout <= 0){
          printf("timeout must be a positive number\n");
          return 1;
        }
        break;
      case 'f':
        with_files = true;
        break;

      case '?':
        break;

      default:
        printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (seed == -1 || array_size == -1 || pnum == -1) {
    printf("Usage: %s --seed \"num\" --array_size \"num\" --pnum \"num\" [--timeout \"num\"] [--by_files]\n",
           argv[0]);
    return 1;
  }

  if (timeout > 0) {
      child_pids = malloc(pnum * sizeof(pid_t));
      if (child_pids == NULL) {
          perror("malloc failed");
          return 1;
      }
      child_count = pnum;
      for (int i = 0; i < pnum; i++) {
          child_pids[i] = -1;
      }
      signal(SIGALRM, timeout_handler);
  }


  int *array = malloc(sizeof(int) * array_size);
  GenerateArray(array, array_size, seed);
  int active_child_processes = 0;

  int **pipes = NULL;
  if (!with_files) {
    pipes = malloc(pnum * sizeof(int *));
    for (int i = 0; i < pnum; i++) {
      pipes[i] = malloc(2 * sizeof(int));
      if (pipe(pipes[i]) == -1) {
        perror("pipe failed");
        return 1;
      }
    }
  }

  struct timeval start_time;
  gettimeofday(&start_time, NULL);

  for (int i = 0; i < pnum; i++) {
    pid_t child_pid = fork();
    if (child_pid >= 0) {
      // successful fork
      active_child_processes += 1;

      if (timeout > 0){
        child_pids[i] = child_pid;
      }
      if (child_pid == 0) {
        // child process
        
        int chunk_size = array_size / pnum;
        int start = i * chunk_size;
        int end = (i == pnum - 1) ? array_size : (i + 1) * chunk_size;
        
        struct MinMax local_min_max = GetMinMax(array, start, end);
        
        if (with_files) {
          char filename[20];
          sprintf(filename, "minmax_%d.txt", i);
          
          FILE *file = fopen(filename, "w");
          if (file == NULL) {
            perror("fopen failed");
            exit(1);
          }
          fprintf(file, "%d %d", local_min_max.min, local_min_max.max);
          fclose(file);
        } else {
          close(pipes[i][0]); 
          
          write(pipes[i][1], &local_min_max.min, sizeof(int));
          write(pipes[i][1], &local_min_max.max, sizeof(int));
          
          close(pipes[i][1]);
        }
        
        free(array);
        if (!with_files) {
          for (int j = 0; j < pnum; j++) {
            if (pipes[j]) free(pipes[j]);
          }
          free(pipes);
        }
        exit(0);
      }

    } else {
      printf("Fork failed!\n");
      return 1;
    }
  }

  if (timeout > 0){
    alarm(timeout);
  }


 while (active_child_processes > 0) {
    int status;
    pid_t finished_pid = waitpid(-1, &status, 0);
    
    if (finished_pid > 0) {
      active_child_processes -= 1;
      
      if (timeout > 0) {
        // Обновляем массив child_pids
        for (int i = 0; i < pnum; i++) {
          if (child_pids[i] == finished_pid) {
            child_pids[i] = -1;
            break;
          }
        }
        
        if (WIFSIGNALED(status)) {
          printf("Child process %d killed by signal: %d\n", 
                 finished_pid, WTERMSIG(status));
        } else if (WIFEXITED(status)) {
          printf("Child process %d exited normally with status: %d\n",
                 finished_pid, WEXITSTATUS(status));
        }
      }
    }
  }

  if (timeout > 0) {
    alarm(0);
  }

  struct MinMax min_max;
  min_max.min = INT_MAX;
  min_max.max = INT_MIN;

  for (int i = 0; i < pnum; i++) {
    int min = INT_MAX;
    int max = INT_MIN;

    if (with_files) {
      char filename[20];
      sprintf(filename, "minmax_%d.txt", i);
      
      FILE *file = fopen(filename, "r");
      if (file == NULL) {
        perror("fopen failed");
        return 1;
      }
      fscanf(file, "%d %d", &min, &max);
      fclose(file);
      remove(filename);
    } else {
      close(pipes[i][1]); 
      
      read(pipes[i][0], &min, sizeof(int));
      read(pipes[i][0], &max, sizeof(int));
      
      close(pipes[i][0]);
    }

    if (min < min_max.min) min_max.min = min;
    if (max > min_max.max) min_max.max = max;
  }

  struct timeval finish_time;
  gettimeofday(&finish_time, NULL);

  double elapsed_time = (finish_time.tv_sec - start_time.tv_sec) * 1000.0;
  elapsed_time += (finish_time.tv_usec - start_time.tv_usec) / 1000.0;

  free(array);
  if (!with_files) {
    for (int i = 0; i < pnum; i++) {
      if (pipes[i]) free(pipes[i]);
    }
    free(pipes);
  }

  if (child_pids != NULL) {
    free(child_pids);
    child_pids = NULL;
    child_count = 0;
  }

  printf("Min: %d\n", min_max.min);
  printf("Max: %d\n", min_max.max);
  printf("Elapsed time: %fms\n", elapsed_time);
  fflush(NULL);
  return 0;
}
