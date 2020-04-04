//
// Created by xushuyang on 2020-4-3.
//

#ifndef RTP1078_RTMP_H
#define RTP1078_RTMP_H

#define BYTE_ORDER_SWAP16(A)        ((uint16_t)(((A & 0xff00) >> 8) | ((A & 0x00ff) << 8)))


#define BYTE_ORDER_SWAP32(A)        ((uint32_t)(((A & 0xff000000) >> 24) | \
                                                       ((A & 0x00ff0000) >> 8) | \
                                                       ((A & 0x0000ff00) << 8) | \
                                                       ((A & 0x000000ff) << 24)))

#define C0                  3
#define C1_LENGTH           1536
#define S1_LENGTH           1536

#pragma pack(1)
typedef struct {
    uint32_t time;
    uint32_t zero;
    char     payload[C1_LENGTH - 8];
} packet_c1_t;
#pragma pack()

#endif //RTP1078_RTMP_H
