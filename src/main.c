/*
 * This is a modified version of the https_client sample in the nRF SDK. There's no HTTP client here, just
 * raw sockets.
 */

#include <string.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <stdlib.h>
#include <zephyr/net/socket.h>
#include <modem/lte_lc.h>

#define SPAN_PORT 80
#define SPAN_HOST "172.16.15.14"

#define BUF_SIZE 1024

static char data_buf[BUF_SIZE];
#define PAYLOAD_LENGTH (256 * BUF_SIZE)

static int send_buf(int fd, const char *buf, size_t bufsz)
{
    int off = 0;
    int bytes = 0;
    do
    {
        bytes = send(fd, &buf[off], bufsz - off, 0);
        if (bytes < 0)
        {
            printk("send() failed, err %d\n", errno);
            return errno;
        }
        off += bytes;
    } while (off < bufsz);

    printk("Sent %d bytes\n", off);
    return 0;
}

void main(void)
{
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

    printk("Waiting for network.. ");
    err = lte_lc_init_and_connect();
    if (err)
    {
        printk("Failed to connect to the LTE network, err %d\n", err);
        return;
    }
    printk("OK\n");

    err = getaddrinfo(SPAN_HOST, NULL, &hints, &res);
    if (err)
    {
        printk("getaddrinfo() failed, err %d\n", errno);
        return;
    }

    ((struct sockaddr_in *)res->ai_addr)->sin_port = htons(SPAN_PORT);

    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd == -1)
    {
        printk("Failed to open socket!\n");
        goto clean_up;
    }

    printk("Connecting to %s\n", SPAN_HOST);
    err = connect(fd, res->ai_addr, sizeof(struct sockaddr_in));
    if (err)
    {
        printk("connect() failed, err: %d\n", errno);
        goto clean_up;
    }

    sprintf(data_buf, "POST /my/thingy/post/path HTTP/1.1\r\n");
    err = send_buf(fd, data_buf, strlen(data_buf));
    if (err)
    {
        goto clean_up;
    }
    sprintf(data_buf, "Content-Type: application/binary\r\n");
    err = send_buf(fd, data_buf, strlen(data_buf));
    if (err)
    {
        goto clean_up;
    }
    sprintf(data_buf, "Content-Length: %d\r\n", PAYLOAD_LENGTH);
    err = send_buf(fd, data_buf, strlen(data_buf));
    if (err)
    {
        goto clean_up;
    }
    sprintf(data_buf, "\r\n");
    err = send_buf(fd, data_buf, strlen(data_buf));
    if (err)
    {
        goto clean_up;
    }

    // Send the payload. Send BUF_SIZE bytes until we've reached
    for (int i = 0; i < PAYLOAD_LENGTH; i += BUF_SIZE)
    {
        for (int j = 0; j < BUF_SIZE; j++)
        {
            data_buf[j] = (char)(j % 0xFF);
        }
        err = send_buf(fd, data_buf, BUF_SIZE);
        if (err)
        {
            goto clean_up;
        }
    }

    // Read the response. Won't parse it
    off = 0;
    do
    {
        bytes = recv(fd, &data_buf[off], BUF_SIZE - off, 0);
        if (bytes < 0)
        {
            printk("recv() failed, err %d\n", errno);
            goto clean_up;
        }
        off += bytes;
    } while (bytes != 0 /* peer closed connection */);

    printk("Received %d bytes\n", off);

    /* Make sure recv_buf is NULL terminated (for safe use with strstr) */
    if (off < sizeof(data_buf))
    {
        data_buf[off] = '\0';
    }
    else
    {
        data_buf[sizeof(data_buf) - 1] = '\0';
    }

    /* Print HTTP response */
    p = strstr(data_buf, "\r\n");
    if (p)
    {
        off = p - data_buf;
        data_buf[off + 1] = '\0';
        printk("\n>\t %s\n\n", data_buf);
    }

    printk("Finished, closing socket.\n");

clean_up:
    freeaddrinfo(res);
    (void)close(fd);

    lte_lc_power_off();
}
