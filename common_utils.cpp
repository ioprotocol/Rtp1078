//
// Created by 84515 on 2020/4/4.
//
#include <boost/log/trivial.hpp>

#include "rtmp.h"

char hex_table[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8',
        '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

char integer_to_hex(unsigned char v)
{
    return hex_table[v];
}

void print_packet(const char* p, uint32_t s)
{
    uint32_t i = 0, j = 0;
    char log[s * 2 + 1];
    while (i < s)
    {
        unsigned char c = *(p + i);
        log[j++] = integer_to_hex(c >> 4);
        log[j++] = integer_to_hex(c & 0xF);
        i++;
    }
    log[j] = '\0';
    BOOST_LOG_TRIVIAL(info) << "size:" << s << ":" << log;
}

