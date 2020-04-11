//
// Created by 84515 on 2020/4/11.
//

#ifndef RTP1078_H264_STREAM_H
#define RTP1078_H264_STREAM_H

#include <stdint.h>

class h264_stream
{
public:
	h264_stream();

	void append(char* data, size_t size);

	bool find_nalu(char** nalu, size_t* size);

	char* data();

	int size();

	int availabe();

	void reset();

	void skip(size_t size);
private:
	enum {
		MAX_SIZE = 16 * 1024 * 1024
	};

	char buf_[MAX_SIZE];
	char* reader_;
	char* writer_;
};


#endif //RTP1078_H264_STREAM_H
