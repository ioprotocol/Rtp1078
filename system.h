//
// Created by xushuyang on 2020-4-3.
//

#ifndef RTP1078_SYSTEM_H
#define RTP1078_SYSTEM_H

#include <iostream>

typedef struct
{
	uint32_t bind_port;

	std::string rtmp_svr;
	uint32_t rtmp_port;
} config_t;

#endif //RTP1078_SYSTEM_H
