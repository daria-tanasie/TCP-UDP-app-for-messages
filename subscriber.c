#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <math.h>

#include "helpers.h"

void print_message(udp_message message, char *buf) {
  char type[15];
  memset(type, 0, sizeof(type));

  if (message.data_type == 0) {
    strcpy(type, "INT");
    type[3] = '\0';
    int nr, signed_nr;

    memcpy(&nr, buf + 52, sizeof(uint32_t));
    if ((int)buf[51] == 0) {
      signed_nr = ntohl(nr);
    } else {
      signed_nr = -ntohl(nr);
    }

    printf("%s:%d - %.50s - %s - %d\n", inet_ntoa(message.serv.sin_addr), ntohs(message.serv.sin_port),
      message.topic, type, signed_nr);
  } else if (message.data_type == 1) {
    strcpy(type, "SHORT_REAL");
    type[10] = '\0';
    uint16_t nr;
    memcpy(&nr, buf + 51, sizeof(uint16_t));
    double short_nr = ntohs(nr) / 100.00;

    printf("%s:%d - %.50s - %s - %.2f\n", inet_ntoa(message.serv.sin_addr), ntohs(message.serv.sin_port),
      message.topic, type, short_nr);
  } else if (message.data_type == 2) {
    strcpy(type, "FLOAT");
    type[5] = '\0';
    uint32_t nr;
    uint8_t pw;
    double signed_nr, p = 1;
    memcpy(&nr, buf + 52, sizeof(uint32_t));
    memcpy(&pw, buf + 52 + sizeof(uint32_t), sizeof(uint8_t));

    for (int k = 0; k < pw; k++) {
      p = p / 10;
    }

    if ((int)buf[51] == 0) {
      signed_nr = ntohl(nr) * 1.00 * p;
    } else {
      signed_nr = -(ntohl(nr) * 1.00 * p);
    }

    message.topic[strlen(message.topic)] = '\0';

    printf("%s:%d - %.50s - %s - %.4f\n", inet_ntoa(message.serv.sin_addr), ntohs(message.serv.sin_port),
      message.topic, type, signed_nr);
  } else if (message.data_type == 3) {
    strcpy(type, "STRING");
    type[6] = '\0';

    message.topic[strlen(message.topic)] = '\0';
    message.content[strlen(message.content)] ='\0';

    printf("%s:%d - %.50s - %s - %.1500s\n", inet_ntoa(message.serv.sin_addr), ntohs(message.serv.sin_port),
      message.topic, type, message.content);
  }
}

int main(int argc, char *argv[]) {

  int sockfd = -1;
  char buf[2000] = {0};
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  port = atoi(argv[3]);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");

  rc = send(sockfd, argv[1], strlen(argv[1]), 0);

  struct pollfd poll_fds[3];

  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;
  poll_fds[1].fd = sockfd;
  poll_fds[1].events = POLLIN;

  int enable = 1;
  if (setsockopt(sockfd, SOL_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  protocol_message message_to_send;  

  while (1)
  {
    rc = poll(poll_fds, 2, -1);
    DIE(rc < 0, "poll");
    
    for (int i = 0; i < 2; i++) {
      if (poll_fds[i].revents & POLLIN) {
        if (poll_fds[i].fd == sockfd) {

          memset(buf, 0, sizeof(buf));
          memset(&message_to_send, 0, sizeof(protocol_message));
          rc = recv(sockfd, &message_to_send, sizeof(protocol_message), 0);
          DIE(rc < 0, "recv");

          if (message_to_send.type == 0) {
            close(sockfd);
            return 0;
          } else if (message_to_send.type == 1) {
            udp_message message = {0};

            strncpy(message.topic, message_to_send.buf, 50);
            message.data_type = message_to_send.buf[50];
            strncpy(message.content, message_to_send.buf + 51, 1500);
            message.serv.sin_addr = serv_addr.sin_addr;
            message.serv.sin_port = serv_addr.sin_port;

            print_message(message, message_to_send.buf);
            break;
          }
        } else if (poll_fds[i].fd == STDIN_FILENO) {
          fgets(buf, sizeof(buf), stdin);
          memcpy(message_to_send.buf, buf, sizeof(buf));

          if (!strncmp(buf, "exit", 4)) {
            message_to_send.type = 0;
            rc = send(sockfd, &message_to_send, sizeof(protocol_message), 0);
            DIE(rc < 0, "recv");
            close(sockfd);
            return 0;
          } else if (!strncmp(buf, "subscribe", 9)) {
            message_to_send.type = 2;
            rc = send(sockfd, &message_to_send, sizeof(protocol_message), 0);
            DIE(rc < 0, "recv");
            char *topic = strtok(buf, " ");
            topic = strtok(NULL, " ");
            topic[strlen(topic) - 1] = '\0';
            printf("Subscribed to topic %s\n", topic);
            break;

          } else if (!strncmp(buf, "unsubscribe", 11)) {
            message_to_send.type = 3;
            rc = send(sockfd, &message_to_send, sizeof(protocol_message), 0);
            DIE(rc < 0, "recv");
            char *topic = strtok(buf, " ");
            topic = strtok(NULL, " ");
            topic[strlen(topic) - 1] = '\0';
            printf("Unsubscribed from topic %s\n", topic);
            break;
          }
        }
      }
    }
  }
  

  close(sockfd);

  return 0;
}
