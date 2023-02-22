/*
 * This is a modified version of the https_client sample in the nRF SDK.
 * There's no HTTP client here, just raw sockets.
 */

#include <modem/lte_lc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/net/socket.h>

#define SPAN_PORT 80
#define SPAN_HOST "172.16.15.14"

#define BUF_SIZE 512

static char data_buf[BUF_SIZE];

#define PAYLOAD_SIZE (512 * 100)
void main(void) {
  int err;
  int fd;
  char *p;
  int bytes;
  size_t off;
  struct addrinfo *res;
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM,
  };

  printk("HTTPS client sample started\n\r");
  err = getaddrinfo(SPAN_HOST, NULL, &hints, &res);
  if (err) {
    printk("getaddrinfo() failed, err %d\n", errno);
    return;
  }

  ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(SPAN_PORT);

  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (fd == -1) {
    printk("Failed to open socket!\n");
    goto clean_up;
  }

  printk("Connecting to %s\n", SPAN_HOST);
  err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
  if (err) {
    printk("connect() failed, err: %d\n", errno);
    goto clean_up;
  }

  sprintf(data_buf, "POST /microblob HTTP/1.1\r\n");
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  sprintf(data_buf, "Content-Type: application/binary\r\n");
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  sprintf(data_buf, "Host: span\r\n");
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  sprintf(data_buf, "Connection: close\r\n");
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  sprintf(data_buf, "Content-Length: %d\r\n", PAYLOAD_SIZE);
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  sprintf(data_buf, "\r\n");
  err = send(fd, data_buf, strlen(data_buf), 0);
  if (err < 0) {
    goto clean_up;
  }
  printk("> %s", data_buf);

  /* We'll generate just some bytes here and send them. If you are slightly */
  for (int i = 0; i < PAYLOAD_SIZE;) {
    for (int j = 0; j < BUF_SIZE; j++) {
      data_buf[j] = (char)(i % 0xff);
    }

    size_t sz = (i - PAYLOAD_SIZE) > BUF_SIZE ? BUF_SIZE : (i - PAYLOAD_SIZE);
    err = send(fd, data_buf, BUF_SIZE, 0);
    if (err < 0) {
      printk("Error sending (%d). Stopping\n", err);
      goto clean_up;
    }
    i += err;
    printk("> (%d bytes, %d total)\n", err, i);
  }

  // Read the response. Won't parse it
  off = 0;
  do {
    bytes = recv(fd, &data_buf[off], BUF_SIZE - off, 0);
    if (bytes < 0) {
      printk("recv() failed, err %d\n", errno);
      goto clean_up;
    }
    off += bytes;
  } while (bytes != 0 /* peer closed connection */);

  printk("Received %d bytes\n", off);

  /* Make sure recv_buf is NULL terminated (for safe use with strstr) */
  if (off < sizeof(data_buf)) {
    data_buf[off] = '\0';
  } else {
    data_buf[sizeof(data_buf) - 1] = '\0';
  }

  /* Print HTTP response */
  p = strstr(data_buf, "\r\n");
  if (p) {
    off = p - data_buf;
    data_buf[off + 1] = '\0';
    printk("\n>\t %s\n\n", data_buf);
  }

  printk("Finished, closing socket.\n");

clean_up:
  printk("Clean up\n");
  freeaddrinfo(res);
  (void)close(fd);

  lte_lc_power_off();
}
