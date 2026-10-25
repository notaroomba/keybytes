/*
 * Line based config protocol (protocol 1) shared by every transport. See
 * app/PROTOCOL.md for the commands. Lines end in \n, \r\n is accepted.
 */
#ifndef PROTO_H
#define PROTO_H

#include <stdint.h>

#define KEYWORD_FW_VERSION "0.1.0"

/* Feed received bytes in any chunk size. */
void PROTO_Input(const uint8_t *data, uint16_t len);

/* Send one line (printf style, \n added). */
void PROTO_Send(const char *fmt, ...) __attribute__((format(printf, 1, 2)));

/* 96-bit little endian UID as 24 uppercase hex digits, most significant
 * byte first. out must hold 25 chars. */
void PROTO_FormatUid(char *out, const uint8_t uid[12]);

#endif /* PROTO_H */
