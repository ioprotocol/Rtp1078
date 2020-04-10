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
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <fstream>

#include "system.h"

extern config_t global_config;

void init_system_logger()
{
	boost::shared_ptr<boost::log::sinks::text_file_backend> backend(new boost::log::sinks::text_file_backend());
	backend->set_file_name_pattern("jtt1078_%Y-%m-%d_%H-%M-%S.%N.log");
	backend->set_rotation_size(100 * 1024 * 1024);
	backend->set_time_based_rotation(boost::log::sinks::file::rotation_at_time_point(0, 0, 0));
	backend->set_file_collector(boost::log::sinks::file::make_collector(boost::log::keywords::target = "logs",
		boost::log::keywords::max_size = 8 * 1024 * 1024 * 1024L,
		boost::log::keywords::min_free_space = 1024 * 1024 * 1024L));
	backend->auto_flush(true);
	// Wrap it into the frontend and register in the core.
	// The backend requires synchronization in the frontend.
	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> sink_t;
	boost::shared_ptr<sink_t> sink(new sink_t(backend));

	sink->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
	sink->set_formatter(boost::log::expressions::stream
		<< boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S : <")
		<< boost::log::trivial::severity << "> : "
		<< boost::log::expressions::smessage
	);
	boost::log::add_common_attributes();
	boost::log::core::get()->add_sink(sink);
}

void init_system_config()
{
	global_config.bind_port = 2935;
	global_config.rtmp_svr = "127.0.0.1";
	global_config.rtmp_port = 1935;
}

void init()
{
	init_system_logger();
	init_system_config();
}

#endif //RTP1078_SYSTEM_INIT_H
