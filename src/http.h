#pragma once
#include <stdio.h>
#include <zephyr/net/socket.h>

/**
 * This is a *very* simple tool class to write a HTTP request. The protocol
 * itself is as simple as they get. The client connects and sends
 *
 * POST [path] HTTP/1.1
 * [Header]: [Value]
 * [Header]: [Value]
 * [ ... repeated headers ... ]
 *
 * [binary payload]
 *
 * The server will then respond with
 * [three-digit status code] [status message]
 * [Response headers]
 *
 * If the Connection: close header is sent by the client the server will then
 * close the connection. The Host: [host name] header is required in HTTP/1.1.
 * The length of the payload is set by the client via the Content-Length header.
 *
 * These functions doesn't do a lot of error checking, it's just a wrapper on
 * top of the socket to write the headers and is a very naive implementation.
 *
 * For production code I'd recommend looking into adapting the source at
 * https://github.com/zephyrproject-rtos/zephyr/blob/main/subsys/net/lib/http/http_client.c
 * (it's not supported in nRF Connect SDK.. .yet)
 */

#define SPAN_PORT 80
#define SPAN_HOST "172.16.15.14"

typedef struct {
  int fd;
  struct addrinfo *addr;
  bool has_content;
  bool has_length;
} http_request_t;

/**
 * Start a HTTP POST request. This creates a new socket, connects to the server
 * and writes a few standard headers.
 *
 * @return false on error
 */
bool http_post(const char *path, http_request_t *req);

/**
 * Set content type for payload
 *
 * @return 0 on sucess, errno on error
 */
int http_header_content_type(http_request_t *req, const char *mime_type);

/**
 * Set size of payload
 *
 * @return 0 on sucess, errno on error
 */
int http_header_content_size(http_request_t *req, const size_t sz);

/**
 * Send payload
 *
 * @return 0 on sucess, errno on error
 */
int http_payload(http_request_t *req, const uint8_t *buf, const size_t sz);

/**
 * Read the result of the request
 *
 * @return HTTP status code 2xx, 3xx, 4xxx...
 */
int http_result(http_request_t *req);

/**
 * Close the request
 */
void http_close(http_request_t *req);
