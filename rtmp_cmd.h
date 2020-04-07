//
// Created by 84515 on 2020/4/5.
//

#ifndef RTP1078_RTMP_CMD_H
#define RTP1078_RTMP_CMD_H

#include <stdint.h>
#include <string>

typedef struct {
    uint64_t transaction_id;
    std::string app;
    std::string flashver;
    std::string swf_url;
    std::string tc_url;
    uint8_t fpad;
    uint64_t audio_codecs;
    uint64_t vidio_codecs;
    uint64_t vidio_function;
    std::string page_url;
    uint64_t capabilities;

    std::string name;
} rtmp_cmd_connect_t;

#endif //RTP1078_RTMP_CMD_H
