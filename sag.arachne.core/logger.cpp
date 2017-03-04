#include "stdafx.h"
#include "logger.h"
#include "configurator.h"
#include <boost/utility/empty_deleter.hpp>

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