#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>

typedef struct tcp_client {
  int is_on;
  int port;
  char id_client[11];
  int fd;
  char topics[100][51];
  int nr_topics;
} tcp_client;

typedef struct udp_message {
  char topic[50];
  char data_type;
  char content[1500];
  struct sockaddr_in serv;
} udp_message;

typedef struct protocol_message {
  int type; // type of the message exit = 0, udp message = 1, subscribe = 2, unsubscribe = 3
  char buf[2000]; // actual message
} protocol_message;

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#endif
