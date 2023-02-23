
#include "http.h"
#include <zephyr/kernel.h>
#define BUF_SZ 256
static char tmpbuf[BUF_SZ];
static char headerbuf[BUF_SZ];

static int send_header(int fd, const char *header, const char *value) {
  sprintf(headerbuf, "%s: %s\r\n", header, value);
  int ret = send(fd, headerbuf, strlen(headerbuf), 0);
  if (ret != strlen(headerbuf)) {
    return -1;
  }
  return 0;
}

bool http_post(const char *path, http_request_t *req) {
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
  };
  int err = getaddrinfo(SPAN_HOST, NULL, &hints, &req->addr);
  if (err) {
    return false;
  }

  ((struct sockaddr_in *)req->addr->ai_addr)->sin_port = htons(SPAN_PORT);

  int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    return false;
  }
  req->fd = fd;
  req->has_content = false;
  req->has_length = false;

  err = connect(fd, req->addr->ai_addr, sizeof(struct sockaddr_in));
  if (err) {
    close(fd);
    return false;
  }

  const char *post_path = path;
  if (strcmp(path, "") == 0) {
    post_path = "/";
  }
  // Send POST header
  sprintf(tmpbuf, "POST %s HTTP/1.1\r\n", post_path);
  err = send(fd, tmpbuf, strlen(tmpbuf), 0);
  if (err < 0) {
    close(fd);
    return false;
  }
  // Send Host and Connection header
  if (send_header(fd, "Connection", "close")) {
    close(fd);
    return false;
  }
  if (send_header(fd, "Host", "span")) {
    close(fd);
    return false;
  }
  return true;
}

int http_header_content_type(http_request_t *req, const char *mime_type) {
  return send_header(req->fd, "Content-Type", mime_type);
}

int http_header_content_size(http_request_t *req, const size_t sz) {
  req->has_length = true;
  sprintf(tmpbuf, "%d", sz);

  return send_header(req->fd, "Content-Length", tmpbuf);
}

int http_payload(http_request_t *req, const uint8_t *buf, const size_t sz) {
  if (!req->has_length) {
    return -1;
  }
  if (!req->has_content) {
    // Send newline before content
    int err = send(req->fd, "\r\n", 2, 0);
    if (err != 2) {
      return err;
    }
  }
  req->has_content = true;

  size_t remaining_size = sz;
  do {
    void *send_buf = buf;
    int sent = send(req->fd, send_buf, remaining_size, 0);
    if (sent < 0) {
      return sent;
    }
    remaining_size -= sent;
    send_buf += sent;
  } while (remaining_size > 0);
  return 0;
}

int http_result(http_request_t *req) {
  int bytes = 0;
  int status = 0;
  do {
    bytes = recv(req->fd, &tmpbuf, BUF_SZ, 0);
    if (bytes < 0) {
      return status;
    }
    // The response is HTTP/1.1 [status code] [message]
    // This is quite ugly but it works.
    if (bytes > 12 && status == 0) {
      status += 100 * (tmpbuf[9] - '0');
      status += 10 * (tmpbuf[10] - '0');
      status += (tmpbuf[11] - '0');
    }
  } while (bytes != 0 /* peer closed connection */);
  return status;
}

void http_close(http_request_t *req) {
  close(req->fd);
  freeaddrinfo(req->addr);
}
