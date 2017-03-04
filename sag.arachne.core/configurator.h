#pragma once

#include <atlbase.h>
#include <atlconv.h>
#include "stdafx.h"

#include "tinyxml/tinyxml.h"
#pragma comment (lib, "tinyxml/tinyxml.lib")

struct LoggerParameters {
	std::string LogFileExtention = "_%3N.log";
	std::string LogName;
	std::string LogFilePath;
	std::string ArchivePath;
	std::string LogFileName;
	int LogRotateSizeMB = 5;
	int ArchiveSizeMB = 50;
	int MinDriveFreeSpace = 100;
};

struct PLCParameters {
	PLC_TYPES PLCType = PLC_TYPES::AUTODETECT;
	int PLCPortNumber;
	std::string PLCIPAddress;
	int PLCPollPeriodMSec;
};

struct LocalDBParameters {
	DB_TYPES LocalDBType;
	std::string LocalDBPath;
	std::string TrendsDBFileName;
	std::string MessagesDBFileName;
	std::string DictionariesDBFileName;
	std::string SecretsDBFileName;
};

struct MainDBParameters {
	std::string MainDBConnectionString;
	int MainDBPollPeriodMSec;
};

class Configurator
{
private:

	std::string _path;
	SERVICE_TYPES _service_type;
	int _service_control_port;
	std::string _service_name;
	const std::string _config_file_name = "service.config";

	LoggerParameters _logger;
	PLCParameters _plc;
	LocalDBParameters _localdb;
	MainDBParameters _maindb;

	bool _is_loaded = false;

public:

	/* CONFIG */
	bool IsLoaded() { return _is_loaded; };

	/* SERVICE */
	std::string ServicePath() { return _path; };
	std::string ServiceName() { return _service_name; }

	/* LOGGER */
	std::string LogName() { return _logger.LogName; };
	std::string LogFile() { return _logger.LogFilePath + _logger.LogFileName; };
	std::string ArchivePath() { return _logger.ArchivePath; };

	int RotateSizeMB() { return _logger.LogRotateSizeMB; };
	int ArchiveSizeMB() { return _logger.ArchiveSizeMB; };
	int MinDriveFreeSpace() { return _logger.MinDriveFreeSpace; };

	/* PLC */
	int PLCPollPeriodMSec() { return _plc.PLCPollPeriodMSec; };

	/* MAINDB */
	int MainDBPollPeriodMSec() { return _maindb.MainDBPollPeriodMSec; };

	Configurator()
	{
		_is_loaded = false;
	}

	bool LoadConfig(int argc, TCHAR *argv[], TCHAR *env[])
	{

		TCHAR szFileName[MAX_PATH];
		TCHAR szPath[MAX_PATH];

		GetModuleFileName(0, szFileName, MAX_PATH);
		ExtractFilePath(szFileName, szPath);

		std::wstring buff = szPath;

		_path = std::string(buff.begin(), buff.end());

		std::string configFile = _path + _config_file_name;

		TiXmlDocument config(configFile.c_str());

		if (!config.Error())
		{
			if (config.LoadFile(TIXML_ENCODING_UTF8))
			{
				try
				{
					TiXmlElement *service = config.FirstChildElement("service");

					if (service)
					{
						switch (static_cast<SERVICE_TYPES>(atoi(service->Attribute("type")))) {

						case SERVICE_TYPES::ARACHNE_NODE:
							goto EXIT;

							break;
						case SERVICE_TYPES::ARACHNE_PLC_CONTROL:

							_service_name = service->Attribute("name");

							if (!_service_name.empty())
							{
								_service_type = SERVICE_TYPES::ARACHNE_PLC_CONTROL;
								_service_control_port = atoi(service->Attribute("command_port"));

								TiXmlElement *logger = service->FirstChildElement("log");
								if (logger)
								{
									_logger.LogName = _service_name;

									_logger.LogFileExtention = logger->Attribute("file_extension_pattern");

									std::string buff = logger->Attribute("alter_file_name");
									if (!buff.empty()) {
										_logger.LogFileName = buff;
									}
									else _logger.LogFileName = _logger.LogName + _logger.LogFileExtention;

									buff.clear();
									buff = logger->Attribute("alter_path");
									if (!buff.empty()) {
										if (buff.substr(0, 2) == ".\\") {
											_logger.LogFilePath = _path + buff.substr(2, buff.length());
										}
										else _logger.LogFilePath = buff;
									}
									else _logger.LogFilePath = _path;

									_logger.LogRotateSizeMB = atoi(logger->Attribute("file_size_mb"));
									_logger.ArchiveSizeMB = atoi(logger->Attribute("archive_size_mb")); 
									_logger.MinDriveFreeSpace = atoi(logger->Attribute("min_drive_free_space_mb"));

									_logger.ArchivePath = logger->Attribute("archive_path");

									TiXmlElement *plc = service->FirstChildElement("plc");
									if (plc)
									{
										_plc.PLCType = static_cast<PLC_TYPES>(atoi(plc->Attribute("type")));
										_plc.PLCPollPeriodMSec = atoi(plc->Attribute("poll_period_msec"));
										_plc.PLCPortNumber = atoi(plc->Attribute("port"));
										_plc.PLCIPAddress = plc->Attribute("ip_address");

										TiXmlElement *localdb = service->FirstChildElement("localdb");
										if (localdb)
										{

											_localdb.LocalDBType = static_cast<DB_TYPES>(atoi(localdb->Attribute("type")));

											switch (static_cast<DB_TYPES>(atoi(localdb->Attribute("type")))) {

											case DB_TYPES::DB_ORACLE:
												goto EXIT;

												break;

											case DB_TYPES::DB_MSSQL:
												goto EXIT;

												break;

											case DB_TYPES::DB_SQLITE:

												buff.clear();
												buff = localdb->Attribute("alter_path");
												if (!buff.empty()) {
													if (buff.substr(0, 2) == ".\\") {
														_localdb.LocalDBPath = _path + buff.substr(2, buff.length());
													}
													else _localdb.LocalDBPath = buff;
												}
												else _localdb.LocalDBPath = _path;

												_localdb.DictionariesDBFileName = _localdb.LocalDBPath + localdb->Attribute("dictionaries");
												_localdb.MessagesDBFileName = _localdb.LocalDBPath + localdb->Attribute("messages");
												_localdb.SecretsDBFileName = _localdb.LocalDBPath + localdb->Attribute("secrets");
												_localdb.TrendsDBFileName = _localdb.LocalDBPath + localdb->Attribute("trends");

												break;
											default:
												goto EXIT;
											}

											TiXmlElement *maindb = service->FirstChildElement("maindb");
											if (maindb)
											{
												switch (static_cast<DB_TYPES>(atoi(maindb->Attribute("type")))) {

												case DB_TYPES::DB_ORACLE:

													_maindb.MainDBConnectionString = maindb->Attribute("conn_string");
													_maindb.MainDBPollPeriodMSec = atoi(maindb->Attribute("poll_period_msec"));

													break;

												case DB_TYPES::DB_MSSQL:
													goto EXIT;

													break;

												case DB_TYPES::DB_SQLITE:
													goto EXIT;

													break;
												default:
													goto EXIT;
												}

												_is_loaded = true;
												return _is_loaded;
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
						default:
							goto EXIT;
						}

					}
					else goto EXIT;
				}
				catch (...)
				{
					goto EXIT;
				}
			}
		}

	EXIT:
		_is_loaded = false;
		return _is_loaded;

		//WRITELOG(logger, framework::Diagnostics::LogLevel::Info, _T("Config is cuccessfully loaded... try to parse..."));
		//WRITELOG(logger, framework::Diagnostics::LogLevel::Info, LPWSTR(config.Value()));
	}

	~Configurator()
	{
	}
};

