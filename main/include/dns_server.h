#ifndef __DNS_SERVER_H__
#define __DNS_SERVER_H__

#include <sys/param.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#define OPCODE_MASK (0x7800)
#define QR_FLAG (1 << 7)
#define QD_TYPE_A (0x0001)
#define ANS_TTL_SEC (300)
#define DNS_PORT (53)
#define DNS_MAX_LEN (256)

// DNS Header Packet
typedef struct __attribute__((__packed__))
{
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} dns_header_t;

// DNS Question Packet
typedef struct {
    uint16_t type;
    uint16_t class;
} dns_question_t;

// DNS Answer Packet
typedef struct __attribute__((__packed__))
{
    uint16_t ptr_offset;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t addr_len;
    uint32_t ip_addr;
} dns_answer_t;


void dns_server_task(void *pvParameters);

#endif
