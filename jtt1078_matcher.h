//
// Created by xushuyang on 2020-4-2.
//

#ifndef RTP1078_JTT1078_MATCHER_H
#define RTP1078_JTT1078_MATCHER_H

#include <utility>

class jtt1078_matcher {
public:
    jtt1078_matcher() {}

    template<typename Iterator>
    std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const {
        // find fix header for packet 0x30 0x31 0x63 0x64
        if (end - begin < HEADER_SIZE) {
            return std::make_pair(begin, false);
        }
        Iterator header_start = begin;
        Iterator i = begin;
        while (i != end) {
            if (end - i < HEADER_SIZE) {
                return std::make_pair(begin, false);
            }
            if (*i == 0x30 && *(i + 1) == 0x31 && *(i + 2) == 0x63 && *(i + 3) == 0x64) {
                header_start = i;
                break;
            }
            i++;
        }
        // match the fix header failed, skip the bytes that has read.
        if (i == end) {
            return std::make_pair(end, false);
        }
        // min length is 30
        if (end - header_start < MIN_PACKET_LENGTH) {
            return std::make_pair(header_start, false);
        }
        // length field check
        uint16_t body_length = *(header_start + 28) << 8 | *(header_start + 29);
        if (body_length > MAX_PACKET_SIZE) {
            // length is too long, skip all data.
            return std::make_pair(end, false);
        }

        // body data has not compeletly transported
        if (end - header_start - 30 < body_length) {
            return std::make_pair(header_start, false);
        }
        return std::make_pair(header_start, true);
    }

private:
    enum {
        MAX_PACKET_SIZE = 100 * 1024,
        MIN_PACKET_LENGTH = 30,
        HEADER_SIZE = 4,
    };
};

namespace boost {
    namespace asio {
        template<>
        struct is_match_condition<jtt1078_matcher> : public boost::true_type {
        };
    } // namespace asio
}

#endif //RTP1078_JTT1078_MATCHER_H
