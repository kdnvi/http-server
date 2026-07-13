#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 12
#define BUF_SIZE 4096

int main(void) {
  // create an endpoint for communication, return a file descriptor
  int server_fd = socket(PF_INET, SOCK_STREAM, 0);
  if (server_fd == -1) {
    perror("cannot create endpoint");
    exit(1);
  }

  // reuse address
  const int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_port = htons(PORT),
      .sin_addr.s_addr = INADDR_ANY,
  };
  // attach socket to local address and port
  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("cannot bind");
    exit(1);
  }

  // listen for connections on the socket
  if (listen(server_fd, BACKLOG) < 0) {
    perror("cannot listen");
    exit(1);
  }
  printf("listening on port %d\n", PORT);

  // main loop
  while (1) {
    int client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
      perror("cannot accept connection");
      continue;
    }

    unsigned char buf[BUF_SIZE];
    const ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      printf("recieved:\n%s\n", buf);
    } else if (n < 0) {
      perror("read failed");
    } else {
      printf("client disconnected");
    }

    const char *reply = "hi there :D";
    write(client_fd, reply, strlen(reply));
    close(client_fd);
  }

  // try to close the socket
  if (close(server_fd) < 0) {
    perror("cannot close connection");
    exit(1);
  }
  return 0;
}
