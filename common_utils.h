//
// Created by 84515 on 2020/4/4.
//

#ifndef RTP1078_COMMON_UTILS_H
#define RTP1078_COMMON_UTILS_H

#include <stdint.h>

#define BYTE_ORDER_SWAP16(A)        ((uint16_t)(((A & 0xff00) >> 8) | ((A & 0x00ff) << 8)))


#define BYTE_ORDER_SWAP32(A)        ((uint32_t)(((A & 0xff000000) >> 24) | \
                                                       ((A & 0x00ff0000) >> 8) | \
                                                       ((A & 0x0000ff00) << 8) | \
                                                       ((A & 0x000000ff) << 24)))

#define BYTE_ORDER_SWAP64(A)        ((uint64_t)((      (A & 0xff00000000000000) >> 56)   | \
                                                       ((A & 0x00ff000000000000) >> 40)  | \
                                                       ((A & 0x0000ff0000000000) >> 24)  | \
                                                       ((A & 0x000000ff00000000) >> 8)   | \
                                                       ((A & 0x00000000ff000000) << 8)   | \
                                                       ((A & 0x0000000000ff0000) << 24)  | \
                                                       ((A & 0x000000000000ff00) << 40)  | \
                                                       ((A & 0x00000000000000ff) << 56)))

char integer_to_hex(unsigned char v);

void print_packet(const char *p, uint32_t s);

#endif //RTP1078_COMMON_UTILS_H
