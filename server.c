#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>

#include "helpers.h"

#define MAX_CONNECTIONS 100

int search_star(char client_topic[50], char topic[50]) {
  if (client_topic[0] == '*' && strlen(client_topic) == 1) {
    return 1;
  }

  char *client_words[10];
  char *topic_words[10];
  int client_num_words = 0;
  int topic_num_words = 0;

  char *token = strtok(client_topic, "/");
  while (token != NULL && client_num_words < 10) {
      client_words[client_num_words++] = token;
      token = strtok(NULL, "/");
  }

  token = strtok(topic, "/");
  while (token != NULL && topic_num_words < 10) {
      topic_words[topic_num_words++] = token;
      token = strtok(NULL, "/");
  }

  int i = 0, j = 0;

  while(i < client_num_words &&  client_words[i][0] !=  '*') {
    i++;
  }

  if (i == client_num_words - 1) {
    i = 0;
    while (j < client_num_words - 1) {
      if (strcmp(client_words[i], topic_words[j]) && client_words[i][0] != '*') {
        return 0;
      }
      j++;
      i++;
    }
    return 1;
  }

  i++;

  while(j < topic_num_words) {
    if (strcmp(client_words[i], topic_words[j])) {
      j++;
      continue;
    } else break;
  }

  if (j == topic_num_words) {
    return 0;
  } else return 1;

  return 0;
}

int search_plus(char client_topic[50], char topic[50]) {
  char *client_words[10];
  char *topic_words[10];
  int client_num_words = 0;
  int topic_num_words = 0;

  char *token = strtok(client_topic, "/");
  while (token != NULL && client_num_words < 10) {
      client_words[client_num_words++] = token;
      token = strtok(NULL, "/");
  }

  token = strtok(topic, "/");
  while (token != NULL && topic_num_words < 10) {
      topic_words[topic_num_words++] = token;
      token = strtok(NULL, "/");
  }

  if (client_num_words != topic_num_words) {
      return 0;
  }

  for (int i = 0; i < client_num_words; i++) {
      if (client_words[i][0] !=  '+' && strcmp(client_words[i], topic_words[i])) {
          return 0;
      }
  }

  return 1;
}

int search_both(char client_topic[50], char topic[50]) {
  char *client_words[10];
  char *topic_words[10];
  int client_num_words = 0;
  int topic_num_words = 0;

  char *token = strtok(client_topic, "/");
  while (token != NULL && client_num_words < 10) {
      client_words[client_num_words++] = token;
      token = strtok(NULL, "/");
  }

  token = strtok(topic, "/");
  while (token != NULL && topic_num_words < 10) {
      topic_words[topic_num_words++] = token;
      token = strtok(NULL, "/");
  }

  int i = 0, j = 0;

  while(i < client_num_words &&  client_words[i][0] !=  '*') {
    i++;
  }

  if (i == client_num_words - 1) {
    for (int k = 0; k < client_num_words; k++) {
      if (client_words[k][0] !=  '+' && client_words[k][0] !=  '*' && strcmp(client_words[k], topic_words[k])) {
          return 0;
      }
    }
    return 1;
  }

  i++;

   while(j < topic_num_words && i < client_num_words) {
    if (strcmp(client_words[i], topic_words[j])) {
      j++;
      continue;
    } else break;
  }

  if (j == topic_num_words) {
    return 0;
  }

  if (client_num_words - i != topic_num_words - j) {
    return 0;
  }

  while(i < client_num_words && j < topic_num_words) {
    if (client_words[i][0] !=  '+' && strcmp(client_words[i], topic_words[j])) {
          return 0;
      }
    i++;
    j++;
  }

  return 1;
}



int main(int argc, char *argv[]) {

  char buf[2000];
  int rc = 0;
  tcp_client clients[MAX_CONNECTIONS];
  int clients_len = 0;
  int curr_sock = 0;

  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  uint16_t port = atoi(argv[1]);

  int tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
  DIE(tcp_socket < 0, "socket tcp");

  int udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
  DIE(udp_socket < 0, "socket udp");

  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;

  int enable = 1;
  if (setsockopt(tcp_socket, SOL_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

  if (setsockopt(tcp_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");  

  rc = bind(tcp_socket, (struct sockaddr *)&serv_addr, socket_len);
  DIE(rc < 0, "bind tcp");

  rc = bind(udp_socket, (struct sockaddr *)&serv_addr, socket_len);
  DIE(rc < 0, "bind udp");

  struct pollfd poll_fds[MAX_CONNECTIONS];

  rc = listen(tcp_socket, MAX_CONNECTIONS);
  DIE(rc < 0, "listen tcp");

  int num_clients = 0;
  poll_fds[0].fd = STDIN_FILENO;
  poll_fds[0].events = POLLIN;
  poll_fds[1].fd = udp_socket;
  poll_fds[1].events = POLLIN;
  poll_fds[2].fd = tcp_socket;
  poll_fds[2].events = POLLIN;

  num_clients = 3;
  protocol_message message_to_send;

  while (1) {
    rc = poll(poll_fds, num_clients, -1);
    DIE(rc < 0, "poll");
    
    for (int i = 0; i < num_clients; i++) {
      if (poll_fds[i].revents & POLLIN) {

        if (poll_fds[i].fd == tcp_socket) {
          struct sockaddr_in cli_addr;
          socklen_t cli_len = sizeof(cli_addr);
          int newsockfd = accept(tcp_socket, (struct sockaddr *)&cli_addr, &cli_len);
          DIE(newsockfd < 0, "accept");

          memset(buf, 0, sizeof(buf));
          rc = recv(newsockfd, buf, sizeof(buf), 0);
          DIE(rc < 0, "recv");

          int ok = 0;
          for (int j = 0; j < clients_len; j++) {
            if (!strncmp(clients[j].id_client, buf, strlen(buf)) && clients[j].is_on == 1) {
              printf("Client %s already connected.\n", buf);
              message_to_send.type = 0;
              rc = send(newsockfd, &message_to_send, sizeof(protocol_message), 0);
              close(newsockfd);
              ok++;
              break;
            } else if (!strncmp(clients[j].id_client, buf, strlen(buf)) && clients[j].is_on == 0) {
              printf("New client %s connected from %s:%d.\n", buf,
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
              clients[j].is_on = 1;
              ok++;  
            }
          }

          if (!ok) {
            strcpy(clients[clients_len].id_client, buf);
            clients[clients_len].port = ntohs(cli_addr.sin_port);
            clients[clients_len].fd = newsockfd;
            clients[clients_len].is_on = 1;
            clients[clients_len].nr_topics = 0;
            printf("New client %s connected from %s:%d.\n", buf,
                inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
            clients_len++;
          }

          poll_fds[num_clients].fd = newsockfd;
          poll_fds[num_clients].events = POLLIN;
          num_clients++;
          break;
        } else if (poll_fds[i].fd == STDIN_FILENO) {
          fgets(buf, sizeof(buf), stdin);
          message_to_send.type = 0;

          if (!strncmp(buf, "exit", 4)) {
            for (int j = 3; j < num_clients; j++) {
              rc = send(poll_fds[j].fd, &message_to_send, sizeof(protocol_message), 0);
              close(poll_fds[j].fd);
            }
            return 0;
          }

        } else if (poll_fds[i].fd == udp_socket) {

          rc = recvfrom(poll_fds[i].fd, buf, sizeof(buf), 0, (struct sockaddr *)&serv_addr, &socket_len);
          DIE(rc < 0, "recvfrom");
          udp_message message;
          message_to_send.type = 1;
          char copy_c[50], copy_t[50];

          strncpy(message.topic, buf, 50);
          
          if (strlen(message.topic) == 50) {
            message.topic[50] = '\0';
          }

          message.data_type = buf[50];
          strncpy(message.content, buf + 51, 1500);
          message.content[strlen(message.content)] = '\0';
          memcpy(message_to_send.buf, buf, sizeof(buf));
          
          for (int j = 0; j < clients_len; j++) {
            for (int k = 0; k < clients[j].nr_topics; k++) {
              if (clients[j].is_on == 0) {
                continue;
              }

              strcpy(copy_c, clients[j].topics[k]);
              copy_c[strlen(copy_c)] = '\0';

              if (strchr(copy_c, '*') && strchr(copy_c, '+') == 0) {
                strcpy(copy_c, clients[j].topics[k]);
                strcpy(copy_t, message.topic);

                if (search_star(copy_c, copy_t)) {
                  rc = send(clients[j].fd, &message_to_send, sizeof(protocol_message), 0);
                  DIE(rc < 0, "send");
                  break;
                }
              }

              if (strchr(copy_c, '+') && strchr(copy_c, '*') == 0) {
                strcpy(copy_c, clients[j].topics[k]);
                copy_c[strlen(copy_c)] = '\0';
                strcpy(copy_t, message.topic);
                copy_t[strlen(copy_t)] = '\0';

                if (search_plus(copy_c, copy_t)) {
                  rc = send(clients[j].fd, &message_to_send, sizeof(protocol_message), 0);
                  DIE(rc < 0, "send");
                  break;
                }
              }

              if (strchr(copy_c, '+') && strchr(copy_c, '*')) {
                strcpy(copy_c, clients[j].topics[k]);
                copy_c[strlen(copy_c)] = '\0';
                strcpy(copy_t, message.topic);
                copy_t[strlen(copy_t)] = '\0';

                if (search_both(copy_c, copy_t)) {
                  rc = send(clients[j].fd, &message_to_send, sizeof(protocol_message), 0);
                  DIE(rc < 0, "send");
                  break;
                }
              }

              if (!strncmp(clients[j].topics[k], message.topic, strlen(message.topic))) {
                rc = send(clients[j].fd, &message_to_send, sizeof(protocol_message), 0);
                DIE(rc < 0, "send");
                break;
              }
            }
          }
          break;
        } else {
          rc = recv(poll_fds[i].fd, &message_to_send, sizeof(protocol_message), 0);
          DIE(rc < 0, "recv");

          if (message_to_send.type == 0) {
            for (int j = 0; j < clients_len; j++) {
              if (poll_fds[i].fd == clients[j].fd) {
                curr_sock = j;
                clients[j].is_on = 0;
                break;
              }
            }

            printf("Client %s disconnected.\n", clients[curr_sock].id_client);
            close(poll_fds[i].fd);
            break;
          }

          if (message_to_send.type == 2) {
            for (int j = 0; j < clients_len; j++) {
              if (poll_fds[i].fd == clients[j].fd) {
                curr_sock = j;
                break;
              }
            }
            char *topic = strtok(message_to_send.buf, " ");
            topic = strtok(NULL, " ");
            topic[strlen(topic) - 1] = '\0';
            strcpy(clients[curr_sock].topics[clients[curr_sock].nr_topics], topic);
            clients[curr_sock].nr_topics++;
            break;
          }

          if (message_to_send.type == 3) {
            for (int j = 0; j < clients_len; j++) {
              if (poll_fds[i].fd == clients[j].fd) {
                curr_sock = j;
                break;
              }
            }
            char *topic = strtok(message_to_send.buf, " ");
            topic = strtok(NULL, " ");
            topic[strlen(topic) - 1] = '\0';
            for (int j = 0; j < clients[curr_sock].nr_topics; j++) {
              if (!strcmp(clients[curr_sock].topics[j], topic)) {
                for (int k = j; k < clients[curr_sock].nr_topics - 1; k++) {
                  strcpy(clients[curr_sock].topics[k], clients[curr_sock].topics[k + 1]);
                }
                clients[curr_sock].nr_topics--;
                break;
              }
            }
            break;
          }
        }
      }
    }
  }

  close(tcp_socket);
  close(udp_socket);

  return 0;
}
