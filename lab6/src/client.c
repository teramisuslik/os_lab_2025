#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h> 
#include "common.h"

struct Server {
  char ip[255];
  int port;
};

struct ClientThreadArgs {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  int success;
};


void* ProcessServer(void* args) {
  struct ClientThreadArgs* thread_args = (struct ClientThreadArgs*)args;
  
  struct hostent *hostname = gethostbyname(thread_args->server.ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", thread_args->server.ip);
    thread_args->success = 0;
    thread_args->result = 1; 
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(thread_args->server.port);
  
  if (hostname->h_addr_list[0] != NULL) {
    memcpy(&server_addr.sin_addr.s_addr, hostname->h_addr_list[0], hostname->h_length);
  } else {
    fprintf(stderr, "No address found for %s\n", thread_args->server.ip);
    thread_args->success = 0;
    thread_args->result = 1;
    return NULL;
  }

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed for server %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    thread_args->success = 0;
    thread_args->result = 1;
    return NULL;
  }

  struct timeval timeout;
  timeout.tv_sec = 5;
  timeout.tv_usec = 0;
  setsockopt(sck, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
  setsockopt(sck, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection failed to %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    thread_args->success = 0;
    thread_args->result = 1;
    return NULL;
  }

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &thread_args->begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &thread_args->end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &thread_args->mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed to server %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    close(sck);
    thread_args->success = 0;
    thread_args->result = 1;
    return NULL;
  }

  printf("Sent to server %s:%d: %lu-%lu mod %lu\n", 
         thread_args->server.ip, thread_args->server.port, 
         thread_args->begin, thread_args->end, thread_args->mod);

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed from server %s:%d\n", 
            thread_args->server.ip, thread_args->server.port);
    thread_args->success = 0;
    thread_args->result = 1;
  } else {
    memcpy(&thread_args->result, response, sizeof(uint64_t));
    thread_args->success = 1;
    printf("Received from server %s:%d: %lu\n", 
           thread_args->server.ip, thread_args->server.port, thread_args->result);
  }

  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers_file[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        if (!ConvertStringToUI64(optarg, &k)) {
          fprintf(stderr, "Invalid k value\n");
          return 1;
        }
        break;
      case 1:
        if (!ConvertStringToUI64(optarg, &mod)) {
          fprintf(stderr, "Invalid mod value\n");
          return 1;
        }
        break;
      case 2:
        strncpy(servers_file, optarg, sizeof(servers_file) - 1);
        servers_file[sizeof(servers_file) - 1] = '\0';
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers_file)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }

  FILE *file = fopen(servers_file, "r");
  if (file == NULL) {
    fprintf(stderr, "Cannot open servers file: %s\n", servers_file);
    return 1;
  }

  struct Server servers[100];
  int servers_num = 0;
  char line[255];

  while (fgets(line, sizeof(line), file) != NULL && servers_num < 100) {
    char *colon = strchr(line, ':');
    if (colon == NULL) {
      fprintf(stderr, "Invalid server format in file: %s\n", line);
      continue;
    }
    
    *colon = '\0';
    strncpy(servers[servers_num].ip, line, sizeof(servers[servers_num].ip) - 1);
    servers[servers_num].port = atoi(colon + 1);
    
    if (servers[servers_num].port <= 0) {
      fprintf(stderr, "Invalid port number: %s\n", colon + 1);
      continue;
    }
    
    servers_num++;
  }
  fclose(file);

  if (servers_num == 0) {
    fprintf(stderr, "No valid servers found in file\n");
    return 1;
  }

  printf("Found %d servers\n", servers_num);

  uint64_t chunk_size = k / servers_num;
  uint64_t remainder = k % servers_num;
  uint64_t current_begin = 1;

  struct ClientThreadArgs *thread_args = malloc(servers_num * sizeof(struct ClientThreadArgs));
  pthread_t *threads = malloc(servers_num * sizeof(pthread_t));

  for (int i = 0; i < servers_num; i++) {
    uint64_t chunk = chunk_size;
    if (i < remainder) {
      chunk++;
    }

    uint64_t end = current_begin + chunk - 1;
    if (end > k) end = k;

    thread_args[i].server = servers[i];
    thread_args[i].begin = current_begin;
    thread_args[i].end = end;
    thread_args[i].mod = mod;
    thread_args[i].result = 1;
    thread_args[i].success = 0;

    current_begin = end + 1;

    if (pthread_create(&threads[i], NULL, ProcessServer, &thread_args[i]) != 0) {
      fprintf(stderr, "Failed to create thread for server %s:%d\n", 
              servers[i].ip, servers[i].port);
      thread_args[i].success = 0;
      thread_args[i].result = 1;
    }
  }

  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
  }

  uint64_t final_result = 1;
  int successful_servers = 0;
  
  for (int i = 0; i < servers_num; i++) {
    if (thread_args[i].success) {
      final_result = MultModulo(final_result, thread_args[i].result, mod);
      successful_servers++;
    } else {
      printf("Warning: Server %s:%d failed\n", 
             thread_args[i].server.ip, thread_args[i].server.port);
    }
  }

  if (successful_servers == 0) {
    fprintf(stderr, "All servers failed!\n");
    final_result = 0;
  }

  printf("\nFinal result: %lu! mod %lu = %lu\n", k, mod, final_result);
  printf("Successful servers: %d/%d\n", successful_servers, servers_num);

  free(thread_args);
  free(threads);
  return 0;
}