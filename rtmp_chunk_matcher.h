#ifndef RTMP_CHUNK_MATCHER_H
#define RTMP_CHUNK_MATCHER_H

#include <cstdint>
#include <utility>

class rtmp_chunk_matcher
{
 public:
	rtmp_chunk_matcher(uint32_t max_chunk_size) : max_chunk_size_(max_chunk_size)
	{
	}

	template<typename Iterator>
	std::pair<Iterator, bool> operator()(Iterator begin, Iterator end) const
	{
		uint8_t fm = ((uint8_t)(*begin) >> 6) & 0x3;
		uint8_t stream_id = (uint8_t)(*begin) & 0x3F;
		uint32_t payload_size = 0;

		uint32_t chunk_packet_size = 1;
		// Chunk header length
		if (stream_id == 0)
		{
			chunk_packet_size = 2;
		}
		if (stream_id == 1)
		{
			chunk_packet_size = 3;
		}
		if (end - begin < chunk_packet_size)
		{
			return std::make_pair(begin, false);
		}
		// Chunk message header length
		switch (fm)
		{
		case 0:
			if (end - begin < chunk_packet_size + 3)
			{
				return std::make_pair(begin, false);
			}
			payload_size = (*(begin + chunk_packet_size + 3) << 16) & 0xFF;
			payload_size = payload_size << 8;
			payload_size |= (*(begin + chunk_packet_size + 4) << 8) & 0xFF;
			payload_size = payload_size << 8;
			payload_size |= *(begin + chunk_packet_size + 5) & 0xFF;

			chunk_packet_size += 11;
			break;
		case 1:
			if (end - begin < chunk_packet_size + 3)
			{
				return std::make_pair(begin, false);
			}
			payload_size = (*(begin + chunk_packet_size + 3) << 16) & 0xFF;
			payload_size = payload_size << 8;
			payload_size |= (*(begin + chunk_packet_size + 4) << 8) & 0xFF;
			payload_size = payload_size << 8;
			payload_size |= *(begin + chunk_packet_size + 5) & 0xFF;

			chunk_packet_size += 7;
			break;
		case 2:
			payload_size = max_chunk_size_;
			break;
		case 3:
			payload_size = max_chunk_size_;
			break;
		}
		if (payload_size > NGX_RTMP_MAX_CHUNK_SIZE)
		{
			return std::make_pair(end, true);
		}
		// 合并chunk，读取成一包
		if (payload_size > max_chunk_size_)
		{
			//
			chunk_packet_size += payload_size / max_chunk_size_;
		}
		chunk_packet_size += payload_size;
		if (end - begin < chunk_packet_size)
		{
			return std::make_pair(begin, false);
		}
		return std::make_pair((begin + chunk_packet_size), true);
	}

 private:
	uint32_t max_chunk_size_;
};

namespace boost
{
	namespace asio
	{
		template<>
		struct is_match_condition<rtmp_chunk_matcher> : public boost::true_type
		{
		};
	} // namespace asio
}

#endif