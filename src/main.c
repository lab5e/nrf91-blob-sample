/*
 * This is a modified version of the https_client sample in the nRF SDK.
 * There's no HTTP client here, just raw sockets.
 */

#include "http.h"
#include <modem/lte_lc.h>
#include <zephyr/kernel.h>

#define SPAN_PORT 80
#define SPAN_HOST "172.16.15.14"

#define BUF_SIZE 512

static char data_buf[BUF_SIZE];

#define PAYLOAD_SIZE (512 * 100)
void main(void) {
  lte_lc_connect();
  printk("HTTPS client sample started\n\r");
  http_request_t req;
  if (!http_post("/microblobber/service", &req)) {
    printk("Error creating request\n");
    goto clean_up;
  }

  http_header_content_type(&req, "application/binary");
  http_header_content_size(&req, PAYLOAD_SIZE);

  printf("Sending payload...\n");
  /* We'll generate just some bytes here and send them. If you are slightly */
  for (int i = 0; i < PAYLOAD_SIZE;) {
    for (int j = 0; j < BUF_SIZE; j++) {
      data_buf[j] = (char)(i % 0xff);
    }
    if (http_payload(&req, data_buf, BUF_SIZE)) {
      goto clean_up;
    }
    i += BUF_SIZE;
    printk("Sent %d of %d bytes\n", i, PAYLOAD_SIZE);
  }

  printk("Status: %d\n", http_result(&req));

clean_up:
  printk("Clean up\n");
  http_close(&req);

  printk("Power off LTE modem\n");
  lte_lc_power_off();
}
