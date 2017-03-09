#pragma once

#include "stdafx.h"

#include "../tinyxml/tinyxml.h"

#pragma comment (lib, "../tinyxml/tinyxml.lib")

struct LoggerParameters {
	std::basic_string<char> LogFileExtention = "_%3N.log";
	std::basic_string<char> LogName;
	std::basic_string<char> LogFilePath;
	std::basic_string<char> ArchivePath;
	std::basic_string<char> LogFileName;
	int LogRotateSizeMB = 5;
	int ArchiveSizeMB = 50;
	int MinDriveFreeSpace = 100;
	bool WriteConfigToLog = false;
};

struct PLCParameters {
	PLC_TYPES PLCType = AUTODETECT;
	int PLCPortNumber;
	std::basic_string<char> PLCIPAddress;
	int PLCPollPeriodMSec;
};

struct LocalDBParameters {
	DB_TYPES LocalDBType;
	std::basic_string<char> LocalDBPath;
	std::basic_string<char> TrendsDBFileName;
	std::basic_string<char> MessagesDBFileName;
	std::basic_string<char> DictionariesDBFileName;
	std::basic_string<char> SecretsDBFileName;
};

struct MainDBParameters {
	std::basic_string<char> MainDBConnectionString;
	int MainDBPollPeriodMSec;
};

struct HardwareControllerParameters {
	std::basic_string<char> FilePath;
	std::basic_string<char> FileName = "sag.arachne.hwcontrol.exe";
};

class Configurator
{
private:

	TCHAR *SERVICE_PATH = nullptr;

	std::basic_string<char> path_;
	SERVICE_TYPES service_type_;
	int service_control_port_;
	std::basic_string<char> service_name_;
	wchar_t* service_name_c_;
	const std::basic_string<char> config_file_name_ = "core.config";

	LoggerParameters logger_;
	PLCParameters plc_;
	LocalDBParameters local_db_;
	MainDBParameters main_db_;
	HardwareControllerParameters hw_controller_;

	bool is_loaded_ = false;

public:

	/* CONFIG */
	bool IsLoaded() const;;

	/* SERVICE */
	std::basic_string<char> ServicePath() const;
	std::basic_string<char> ServiceName() const;
	LPCWSTR ServiceName_C() const;

	/* LOGGER */
	std::basic_string<char> LogName() const;
	std::basic_string<char> LogFile() const;
	std::basic_string<char> ArchivePath() const;

	int RotateSizeMB() const;
	int ArchiveSizeMB() const;
	int MinDriveFreeSpace() const;

	bool WriteConfigToLog() const;

	/* PLC */
	int PLCPollPeriodMSec() const;

	/* MAINDB */
	int MainDBPollPeriodMSec() const;

	/* HARDWARE CONTROLLER */
	std::basic_string<char> FileName() const;
	std::basic_string<char> FilePath() const;


	Configurator();

	bool LoadConfig(int argc, wchar_t* argv[], wchar_t* env[]);

	~Configurator();
};

inline bool Configurator::IsLoaded() const
{
	return is_loaded_;
}

inline std::basic_string<char> Configurator::ServicePath() const
{
	return path_;
}

inline std::basic_string<char> Configurator::ServiceName() const
{
	return service_name_;
}

inline const wchar_t* Configurator::ServiceName_C() const
{
	return service_name_c_;
}

inline std::basic_string<char> Configurator::LogName() const
{
	return logger_.LogName;
}

inline std::basic_string<char> Configurator::LogFile() const
{
	return logger_.LogFilePath + logger_.LogFileName;
}

inline std::basic_string<char> Configurator::ArchivePath() const
{
	return logger_.ArchivePath;
}

inline int Configurator::RotateSizeMB() const
{
	return logger_.LogRotateSizeMB;
}

inline int Configurator::ArchiveSizeMB() const
{
	return logger_.ArchiveSizeMB;
}

inline int Configurator::MinDriveFreeSpace() const
{
	return logger_.MinDriveFreeSpace;
}

inline bool Configurator::WriteConfigToLog() const
{	
	return logger_.WriteConfigToLog;
}

inline int Configurator::PLCPollPeriodMSec() const
{
	return plc_.PLCPollPeriodMSec;
}

inline std::basic_string<char> Configurator::FileName() const
{
	return hw_controller_.FileName;
}

inline std::basic_string<char> Configurator::FilePath() const
{
	return hw_controller_.FilePath;
}

inline int Configurator::MainDBPollPeriodMSec() const
{
	return main_db_.MainDBPollPeriodMSec;
}

inline Configurator::Configurator(): service_type_(SERVICE_TYPES::ARACHNE_NODE), service_control_port_(5555), service_name_(), service_name_c_(), is_loaded_(false)
{
}

inline bool Configurator::LoadConfig(int argc, wchar_t* argv[], wchar_t* env[])
{
	std::wstring full_path = argv[0];
	const auto end_path= full_path.rfind('\\');
	const auto config_path = full_path.substr(0, end_path + 1);

	path_ = std::basic_string<char>(config_path.begin(), config_path.end());

	auto config_file = path_ + config_file_name_;

	TiXmlDocument config(config_file.c_str());

	if (!config.Error() && config.LoadFile(TIXML_ENCODING_UTF8))
	{
		try
		{
			auto service = config.FirstChildElement("service");

			if (service)
				switch (static_cast<SERVICE_TYPES>(atoi(service->Attribute("type"))))
				{
				case SERVICE_TYPES::ARACHNE_NODE:
					goto EXIT;

					break;
				case SERVICE_TYPES::ARACHNE_PLC_CONTROL:

					service_name_ = service->Attribute("name");

					if (!service_name_.empty())
					{

						const auto service_name_length = (service_name_.length() + 1);

						service_name_c_ = new wchar_t[service_name_length];

						size_t converted_chars = 0;

						mbstowcs_s(&converted_chars, service_name_c_, service_name_length, service_name_.c_str(), _TRUNCATE);

						service_type_ = static_cast<SERVICE_TYPES>(atoi(service->Attribute("type")));
						service_control_port_ = atoi(service->Attribute("command_port"));

						auto logger = service->FirstChildElement("log");
						if (logger)
						{
							logger_.LogName = service_name_;

							logger_.WriteConfigToLog = !static_cast<bool>(std::strcmp(logger->Attribute("config_to_log"), "true"));

							logger_.LogFileExtention = logger->Attribute("file_extension_pattern");

							std::basic_string<char> _buff = logger->Attribute("alter_file_name");

							!_buff.empty() ? logger_.LogFileName = _buff : logger_.LogFileName = logger_.LogName + logger_.LogFileExtention;

							_buff = logger->Attribute("alter_path");

							!_buff.empty() ? _buff.substr(0, 2) == ".\\" ? logger_.LogFilePath = path_ + _buff.substr(2, _buff.length()) : logger_.LogFilePath = _buff : logger_.LogFilePath = path_;

							logger_.LogRotateSizeMB = atoi(logger->Attribute("file_size_mb"));
							logger_.ArchiveSizeMB = atoi(logger->Attribute("archive_size_mb"));
							logger_.MinDriveFreeSpace = atoi(logger->Attribute("min_drive_free_space_mb"));

							logger_.ArchivePath = logger->Attribute("archive_path");

							auto plc = service->FirstChildElement("plc");
							if (plc)
							{
								plc_.PLCType = static_cast<PLC_TYPES>(atoi(plc->Attribute("type")));
								plc_.PLCPollPeriodMSec = atoi(plc->Attribute("poll_period_msec"));
								plc_.PLCPortNumber = atoi(plc->Attribute("port"));
								plc_.PLCIPAddress = plc->Attribute("ip_address");

								auto localdb = service->FirstChildElement("localdb");
								if (localdb)
								{
									local_db_.LocalDBType = static_cast<DB_TYPES>(atoi(localdb->Attribute("type")));

									switch (static_cast<DB_TYPES>(atoi(localdb->Attribute("type"))))
									{
									case DB_TYPES::DB_ORACLE:
										goto EXIT;

										break;

									case DB_TYPES::DB_MSSQL:
										goto EXIT;

										break;

									case DB_TYPES::DB_SQLITE:

										_buff = localdb->Attribute("alter_path");

										!_buff.empty() ? _buff.substr(0, 2) == ".\\" ? local_db_.LocalDBPath = path_ + _buff.substr(2, _buff.length()) : local_db_.LocalDBPath = _buff : local_db_.LocalDBPath = path_;

										local_db_.DictionariesDBFileName = local_db_.LocalDBPath + localdb->Attribute("dictionaries");
										local_db_.MessagesDBFileName = local_db_.LocalDBPath + localdb->Attribute("messages");
										local_db_.SecretsDBFileName = local_db_.LocalDBPath + localdb->Attribute("secrets");
										local_db_.TrendsDBFileName = local_db_.LocalDBPath + localdb->Attribute("trends");

										break;
									default:
										goto EXIT;
									}

									auto maindb = service->FirstChildElement("maindb");
									if (maindb)
									{
										switch (static_cast<DB_TYPES>(atoi(maindb->Attribute("type"))))
										{
										case DB_TYPES::DB_ORACLE:

											main_db_.MainDBConnectionString = maindb->Attribute("conn_string");
											main_db_.MainDBPollPeriodMSec = atoi(maindb->Attribute("poll_period_msec"));

											break;

										case DB_TYPES::DB_MSSQL:

											main_db_.MainDBConnectionString = maindb->Attribute("conn_string");
											main_db_.MainDBPollPeriodMSec = atoi(maindb->Attribute("poll_period_msec"));

											break;

										case DB_TYPES::DB_SQLITE:
											goto EXIT;

											break;
										default:
											goto EXIT;
										}

										auto hwcontrol = service->FirstChildElement("hwcontrol");
										if(hwcontrol)
										{
											_buff = hwcontrol->Attribute("alter_path");

											!_buff.empty() ? _buff.substr(0, 2) == ".\\" ? hw_controller_.FilePath = path_ + _buff.substr(2, _buff.length()) : hw_controller_.FilePath = _buff : hw_controller_.FilePath = path_;

											_buff = hwcontrol->Attribute("alter_file_name");

											if (!_buff.empty()) hw_controller_.FileName = _buff;
										}

										is_loaded_ = true;
										return is_loaded_;
									}
									else
									{
										goto EXIT;
									}
								}
								else
								{
									goto EXIT;
								}
							}
							else
							{
								goto EXIT;
							}
						}
						else
						{
							goto EXIT;
						}
					}
					else
					{
						goto EXIT;
					}

					break;
				case SERVICE_TYPES::ARACHNE_WEIGHT_CONTROL:
					goto EXIT;

					break;
				case SERVICE_TYPES::ARACHNE_TERMAL_CONTROL:
					goto EXIT;

					break;
				case SERVICE_TYPES::ARACHNE_BUZZER_CONTROL:
					goto EXIT;

					break;
				case ARACHNE_PLC_EXCHANGER: break;
				default:
					goto EXIT;
				}
			else goto EXIT;
		}
		catch (...)
		{
			goto EXIT;
		}
	}

EXIT:
	is_loaded_ = false;
	return is_loaded_;
}

inline Configurator::~Configurator()
{
}

