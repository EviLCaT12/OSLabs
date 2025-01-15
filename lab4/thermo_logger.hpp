#ifndef THERMO_LOGGER_HPP
#define THERMO_LOGGER_HPP

#include <string>
#include <vector>
#include <boost/asio.hpp>
#include <boost/chrono.hpp>

const std::string LOG_FILE_ALL = "temperature_all.log";
const std::string LOG_FILE_HOUR = "temperature_hour.log";
const std::string LOG_FILE_DAY = "temperature_day.log";

const boost::chrono::hours LOG_HOUR(1);
const boost::chrono::hours LOG_DAY(24);

extern std::vector<double> hourlyTemperatures;
extern double dailyTotalTemperature;
extern int dailyTemperatureCount;

extern std::string COMM;

void clearOldEntries(const std::string& filename, const boost::chrono::system_clock::time_point& threshold);
void logTemperature(double temperature, const std::string& filename);
void updateHourlyLog();
void updateDailyLog();

#endif