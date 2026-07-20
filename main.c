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
#define HEADER_CAP 32

struct http_header {
  char *name;
  char *value;
};

struct http_request {
  char *method;
  char *target;
  char *version;
  struct http_header *headers;
  size_t header_count;
};

void print_request(const struct http_request *req) {
  printf("\nRECIEVED:\n");
  printf("method: %s\n", req->method);
  printf("target: %s\n", req->target);
  printf("version: %s\n", req->version);
  printf("headers:\n");
  for (size_t i = 0; i < req->header_count; i++) {
    struct http_header *ref = &req->headers[i];
    printf("%s: %s\n", ref->name, ref->value);
  }
}

void send_response(int client_fd, int status_code, const char *reason,
                   const char *body) {
  char resp[BUF_SIZE];
  int resp_len = snprintf(resp, sizeof(resp),
                          "HTTP/1.1 %d %s\r\n"
                          "Content-Length: %zu\r\n"
                          "Content-Type: text/plain\r\n"
                          "Connection: close\r\n"
                          "\r\n"
                          "%s",
                          status_code, reason, strlen(body), body);
  write(client_fd, resp, resp_len);
}

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

    char buf[BUF_SIZE];
    const ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
      buf[n] = '\0';
      struct http_request req;

      req.method = buf;
      char *ptr = strchr(req.method, ' ');
      if (ptr == NULL) {
        perror("malformed request");
        close(client_fd);
        continue;
      }
      *ptr = '\0';

      req.target = ptr + 1;
      ptr = strchr(req.target, ' ');
      if (ptr == NULL) {
        perror("malformed request");
        close(client_fd);
        continue;
      }
      *ptr = '\0';

      req.version = ptr + 1;
      ptr = strstr(req.version, "\r\n");
      if (ptr == NULL) {
        perror("malformed request");
        close(client_fd);
        continue;
      }
      *ptr = '\0';

      // header parse
      struct http_header *headers =
          malloc(HEADER_CAP * sizeof(struct http_header));
      req.header_count = 0;
      char *line = ptr + 2; // skip \0\n

      for (;;) {
        struct http_header header;
        header.name = line;
        ptr = strstr(header.name, ": ");
        if (ptr == NULL)
          break;

        *ptr = '\0';
        header.value = ptr + 2;
        ptr = strstr(header.value, "\r\n");
        if (ptr == NULL) {
          perror("no header value");
          line = ptr + 2;
          continue;
        }

        *ptr = '\0';
        line = ptr + 2;
        if (req.header_count >= HEADER_CAP) {
          perror("too many headers :D");
          break;
        }
        headers[req.header_count++] = header;
      }
      req.headers = headers;

      print_request(&req);
      free(req.headers);
    } else if (n < 0) {
      perror("read failed");
      send_response(client_fd, 500, "Internal Server Error",
                    "Internal Server Error\n");
    } else {
      printf("client disconnected");
    }

    send_response(client_fd, 200, "OK", "hi there :D\n");
    close(client_fd);
  }

  // try to close the socket
  if (close(server_fd) < 0) {
    perror("cannot close connection");
    exit(1);
  }
  return 0;
}
