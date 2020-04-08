//
// Created by xushuyang on 2020-4-2.
//

#ifndef RTP1078_JTT1078_MATCHER_H
#define RTP1078_JTT1078_MATCHER_H

#include <utility>

class jtt1078_matcher
{
public:
	template<typename Iterator>
	std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const
	{
		// find fix header for packet 0x30 0x31 0x63 0x64
		if (end - begin < 4)
		{
			return std::make_pair(begin, false);
		}
		if (!((*(begin) & 0xFF) == 0x30 && (*(begin + 1) & 0xFF) == 0x31 && (*(begin + 2) & 0xFF) == 0x63 && (*(begin + 3) & 0xFF)) == 0x64)
		{
			return std::make_pair(begin + 1, false);
		}
		// min length is 30
		if (end - begin < 30)
		{
			return std::make_pair(begin, false);
		}
		// length field check
		uint32_t body_length;
		body_length = (*(begin + 28)) & 0xFF;
		body_length = body_length << 8;
		body_length |= *(begin + 29) & 0xFF;
		if (body_length > 8 * 1024)
		{
			// length is too long, skip all data.
			return std::make_pair(end, false);
		}

		// body data has not compeletly transported
		if (end - begin < body_length + 30)
		{
			return std::make_pair(begin, false);
		}
		return std::make_pair(begin + 30 + body_length, true);
	}
};

namespace boost
{
	namespace asio
	{
		template<>
		struct is_match_condition<jtt1078_matcher> : public boost::true_type
		{
		};
	} // namespace asio
}
#endif //RTP1078_JTT1078_MATCHER_H
