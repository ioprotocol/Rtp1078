#ifndef RTMP_S1_MATCHER_H
#define RTMP_S1_MATCHER_H

#include <utility>
#include "rtmp.h"

class rtmp_s1_matcher {
public:
    rtmp_s1_matcher() {}

    template<typename Iterator>
    std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const {
        // find fix header for packet 0x30 0x31 0x63 0x64
        if (end - begin < S1_LENGTH + 1) {
            return std::make_pair(begin, false);
        }
        return std::make_pair((begin + S1_LENGTH + 1), true);
    }
};

namespace boost {
    namespace asio {
        template<>
        struct is_match_condition<rtmp_s1_matcher> : public boost::true_type {
        };
    } // namespace asio
}

#endif