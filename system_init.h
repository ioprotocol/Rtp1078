//
// Created by 84515 on 2020/4/2.
//

#ifndef RTP1078_SYSTEM_INIT_H
#define RTP1078_SYSTEM_INIT_H

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

void init() {
    boost::log::add_file_log(boost::log::keywords::file_name = "sample_%N.log",
                             boost::log::keywords::rotation_size = 10 * 1024 * 1024,
                             boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
                             boost::log::keywords::format = "[%TimeStamp%]: %Message%");

    boost::log::core::get()->set_filter(
            boost::log::trivial::severity >= boost::log::trivial::info
    );
}

#endif //RTP1078_SYSTEM_INIT_H
