//// std
//#include <cassert>
//#include <iostream>
//#include <fstream>
//
//// boost
//#include <boost/log/expressions.hpp>
//#include <boost/log/attributes.hpp>
//#include <boost/log/sinks.hpp>
//#include <boost/utility/empty_deleter.hpp>
//#include <boost/log/support/date_time.hpp>
//
//namespace logging = boost::log;
//namespace expr = boost::log::expressions;
//namespace sinks = boost::log::sinks;
//namespace attrs = boost::log::attributes;
//namespace src = boost::log::sources;
//namespace keywords = boost::log::keywords;
//
//BOOST_LOG_GLOBAL_LOGGER_INIT(mon::ups_logger, src::severity_logger_mt<severity_level>) 
//{
//    src::severity_logger_mt<severity_level> lg;
//    lg.add_attribute("RecordID", attrs::counter<unsigned int>(1));
//    lg.add_attribute("TimeStamp", attrs::local_clock());
//    lg.add_attribute("Scope", attrs::named_scope());
//    return lg;
//}
//
//void mon::logger_configurator::configure(mon::severity_level lvl)
//{
//    // level validation
//    if (lvl < info || lvl > critical)
//        lvl = info;
//
//    boost::shared_ptr<logging::core> log_core(logging::core::get());
//
//    // console logging
//    boost::shared_ptr<sinks::text_ostream_backend> console_backend = boost::make_shared<sinks::text_ostream_backend>();
//    console_backend->auto_flush();
//    console_backend->add_stream(boost::shared_ptr<std::ostream>(&std::clog, logging::empty_deleter()));
//
//    typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink_t;
//    boost::shared_ptr<text_sink_t> console_sink = boost::make_shared<text_sink_t>(console_backend);
//    console_sink->set_formatter(expr::stream
//        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
//        << " [" << expr::attr<severity_level>("Severity")
//        << "] [" << expr::format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::forward)
//        << "]: " << expr::smessage);
//    log_core->add_sink(console_sink);
//    
//    // rotation file logging
//    const std::string filename = "my_log_%N.log";
//    boost::shared_ptr<sinks::text_file_backend> file_backend = boost::make_shared<sinks::text_file_backend>
//        (
//            keywords::file_name = filename,
//            keywords::rotation_size = 5 * 1024 * 1024,
//            keywords::time_based_rotation = sinks::file::rotation_at_time_point(0, 0, 0),
//            keywords::auto_flush = true
//        );
//    file_backend->set_file_collector(sinks::file::make_collector
//        (
//            keywords::target = "log_history",
//            keywords::max_size = 100 * 1024 * 1024,
//            keywords::min_free_space = 50 * 1024 * 1024
//        ));
//
//    try
//    {
//        file_backend->scan_for_files(sinks::file::scan_all);
//    }
//    catch (const std::exception& ex)
//    {
//        logerror << "exception during scanning: " << ex.what();
//    }
//    
//    typedef sinks::synchronous_sink<sinks::text_file_backend> file_text_sink_t;
//    boost::shared_ptr<file_text_sink_t> file_sink = boost::make_shared<file_text_sink_t>(file_backend);
//
//    file_sink->set_formatter(expr::stream
//        << expr::attr<unsigned int>("RecordID")
//        << ". [" << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
//        << "] [" << expr::attr<severity_level>("Severity")
//        << "] [" << expr::format_named_scope("Scope", keywords::format = "%n", keywords::iteration = expr::forward)
//        << "]: " << expr::message);
//    log_core->add_sink(file_sink);
//
//    // set logging filter
//    log_core->set_filter(expr::attr<mon::severity_level>("Severity") >= lvl);
//}

#include "logger.h"

#include <boost/utility/empty_deleter.hpp>

#include "configurator.h"

namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace logging = boost::log;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;

extern Configurator config;

BOOST_LOG_GLOBAL_LOGGER_INIT(my_logger, logger_t)
{
	logger_t lg;
	logging::add_common_attributes();

	boost::shared_ptr<logging::core> log_core(logging::core::get());

	boost::shared_ptr<sinks::text_file_backend> file_backend = boost::make_shared<sinks::text_file_backend>
		(
			keywords::file_name = config.LogFile(),
			keywords::open_mode = std::ios::app,
			keywords::rotation_size = config.RotateSizeMB() * 1024 * 1024,
			keywords::min_free_space = config.MinDriveFreeSpace() * 1024 * 1024,
			keywords::auto_flush = true
			);

	file_backend->set_file_collector(sinks::file::make_collector
		(
			keywords::target = config.ArchivePath(),
			keywords::max_size = config.ArchiveSizeMB() * 1024 * 1024,
			keywords::min_free_space = config.MinDriveFreeSpace() * 1024 * 1024
		));

	file_backend->scan_for_files();

	typedef sinks::synchronous_sink<sinks::text_file_backend> file_text_sink_t;
	boost::shared_ptr<file_text_sink_t> file_sink = boost::make_shared<file_text_sink_t>(file_backend);

	file_sink->set_formatter(expr::stream
		<< expr::format_date_time< boost::posix_time::ptime >("TimeStamp", "%d.%m.%Y %H:%M:%S.%f")
		<< " [" << expr::attr< severity_level >("Severity") << "]: "
		<< expr::smessage);
	log_core->add_sink(file_sink);

#if _DEBUG
	logging::core::get()->set_filter(expr::attr<severity_level>("Severity") >= trace);
#else
	logging::core::get()->set_filter(expr::attr<severity_level>("Severity") >= info);
#endif

	return lg;
}