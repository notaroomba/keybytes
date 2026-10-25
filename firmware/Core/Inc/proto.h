/*
 * Line based config protocol (protocol 1) shared by every transport. See
 * app/PROTOCOL.md for the commands. Lines end in \n; \r\n and a bare \r
 * (a terminal's Enter) are accepted too.
 */
#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#define KEYWORD_FW_VERSION "0.1.0"

#define PROTO_LINE_MAX 128
#define PROTO_MAX_PORTS 2

/* One per transport (NUS, USB CDC): its own line buffer and output. The
 * transport sets connected when someone is listening (NUS: TX notifications
 * on, CDC: DTR set); a received line also counts. */
typedef struct
{
  void (*write)(const char *data, uint16_t len);
  volatile uint8_t connected;
  uint8_t overflow;
  uint16_t len;
  char line[PROTO_LINE_MAX];
} proto_port_t;

void PROTO_AddPort(proto_port_t *port);

/* Feed received bytes in any chunk size. Replies go back to this port. */
void PROTO_Input(proto_port_t *port, const uint8_t *data, uint16_t len);

/* Unsolicited line (printf style, \n added) to every connected port. */
void PROTO_Send(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* 96-bit little endian UID as 24 uppercase hex digits, most significant
 * byte first. out must hold 25 chars. */
void PROTO_FormatUid(char *out, const uint8_t uid[12]);

#endif /* PROTO_H */
