#ifndef THERMO_LOGGER_HPP
#define THERMO_LOGGER_HPP

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>
#include <sqlite3.h>

const std::string LOG_TABLE_ALL = "temperature_all";
const std::string LOG_TABLE_HOUR = "temperature_hour";
const std::string LOG_TABLE_DAY = "temperature_day";

const boost::chrono::hours LOG_HOUR(1);
const boost::chrono::hours LOG_DAY(24);

extern std::vector<double> hourlyTemperatures;
extern double dailyTotalTemperature;
extern int dailyTemperatureCount;

extern std::string COMM;
extern sqlite3* db;

void db_init();
void clearOldEntries(const std::string& tableName, const boost::chrono::system_clock::time_point& threshold);
void logTemperature(double temperature, const std::string& tableName);

void updateHourlyLog();
void updateDailyLog();

#endif