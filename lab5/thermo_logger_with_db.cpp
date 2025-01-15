#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>

#include <boost/asio.hpp>
#include <boost/date_time.hpp>

#ifdef _WIN32
#include <WinSock2.h>
#include <boost/thread/thread.hpp>
#else
#include <unistd.h>
#include <boost/thread.hpp>
#endif

#include "thermo_logger.hpp"

using namespace std;
using namespace boost::asio;
using namespace boost::asio::ip;

vector<double> hourlyTemperatures;
double dailyTotalTemperature = 0.0;
int dailyTemperatureCount = 0;

string COMM;
sqlite3* db;

io_service service;
tcp::acceptor acceptor(service, tcp::endpoint(tcp::v4(), 8080));

void clearOldEntries(const std::string& tableName, const boost::chrono::system_clock::time_point& threshold) {
    char timestamp[20];
    auto thresholdTime = boost::chrono::system_clock::to_time_t(threshold);
    std::strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", std::localtime(&thresholdTime));

    std::string deleteQuery = "DELETE FROM " + tableName + " WHERE timestamp < '" + timestamp + "';";

    int rc = sqlite3_exec(db, deleteQuery.c_str(), 0, 0, 0);
    if (rc != SQLITE_OK) {
        cerr << "Failed to clear old entries in the database: " << sqlite3_errmsg(db) << endl;
    }
}

void logTemperature(double temperature, const string& tableName) {
    auto now = boost::chrono::system_clock::to_time_t(boost::chrono::system_clock::now());
    std::tm* ptm = std::localtime(&now);

    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", ptm);

    std::stringstream insertQuery;
    insertQuery << "INSERT INTO " << tableName << " (timestamp, temperature) VALUES ('" << timestamp << "', " << temperature << ");";
    
    int rc = sqlite3_exec(db, insertQuery.str().c_str(), 0, 0, 0);
    if (rc) {
        cerr << "Failed to insert data into the database: " << sqlite3_errmsg(db) << endl;
    }
}

void updateHourlyLog() {
    if (!hourlyTemperatures.empty()) {
        double sum = 0.0;
        for (double temp : hourlyTemperatures) {
            sum += temp;
        }
        double average = sum / hourlyTemperatures.size();
        logTemperature(average, LOG_TABLE_HOUR);
        hourlyTemperatures.clear();
    }
}

void updateDailyLog() {
    if (dailyTemperatureCount > 0) {
        double average = dailyTotalTemperature / dailyTemperatureCount;
        logTemperature(average, LOG_TABLE_DAY);
        dailyTotalTemperature = 0.0;
        dailyTemperatureCount = 0;
    }
}

void db_init() {
    int rc = sqlite3_open("temperature_logs.db", &db);
    if (rc) {
        cerr << "Can't open database: " << sqlite3_errmsg(db) << endl;
        exit(0);
    }

    const char* createTableAllQuery =
        "CREATE TABLE IF NOT EXISTS temperature_all ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME,"
        "temperature REAL);";
    
    const char* createTableHourQuery =
        "CREATE TABLE IF NOT EXISTS temperature_hour ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME,"
        "temperature REAL);";

    const char* createTableDayQuery =
        "CREATE TABLE IF NOT EXISTS temperature_day ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME,"
        "temperature REAL);";

    rc = sqlite3_exec(db, createTableAllQuery, 0, 0, 0);
    if (rc) {
        cerr << "Can't create table 'temperature_all': " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        exit(0);
    }

    rc = sqlite3_exec(db, createTableHourQuery, 0, 0, 0);
    if (rc) {
        cerr << "Can't create table 'temperature_hour': " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        exit(0);
    }

    rc = sqlite3_exec(db, createTableDayQuery, 0, 0, 0);
    if (rc) {
        cerr << "Can't create table 'temperature_day': " << sqlite3_errmsg(db) << endl;
        sqlite3_close(db);
        exit(0);
    }
}

int main(int argc, char** argv) {
    db_init();
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <serial_port>" << std::endl;
        return 1;
    }
    COMM = argv[1];

    io_service io;
    serial_port port(io, COMM);
    port.set_option(serial_port_base::baud_rate(9600));

    if (!port.is_open()) {
        cerr << "Failed to open serial port" << endl;
        return 1;
    }

    #ifdef _WIN32
        HANDLE serialHandle = port.native_handle();
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(serialHandle, &dcbSerialParams)) {
            cerr << "Failed to get serial port state" << endl;
            return 1;
        }

        dcbSerialParams.fOutxCtsFlow = false;
        dcbSerialParams.fOutxDsrFlow = false;
        dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;

        if (!SetCommState(serialHandle, &dcbSerialParams)) {
            cerr << "Failed to set serial port state" << endl;
            return 1;
        }
#endif

    boost::chrono::system_clock::time_point lastHourlyUpdate = boost::chrono::system_clock::now();
    boost::chrono::system_clock::time_point lastDailyUpdate = boost::chrono::system_clock::now();
    boost::chrono::system_clock::time_point lastMonthlyUpdate = boost::chrono::system_clock::now();
    boost::chrono::system_clock::time_point lastYearUpdate = boost::chrono::system_clock::now();
    boost::chrono::system_clock::time_point lastAllLogUpdate = boost::chrono::system_clock::now();

    while (true) {
        char data[256];
        size_t bytesRead = port.read_some(buffer(data, 256));
        cerr << "bytesRead: " << bytesRead << endl << "data: " << data << endl;

        if (bytesRead > 0) {
            double temperature = atof(data);
            
            logTemperature(temperature, LOG_TABLE_ALL);
            if (boost::chrono::system_clock::now() - lastAllLogUpdate > boost::chrono::hours(24)) {
                clearOldEntries(LOG_TABLE_ALL, boost::chrono::system_clock::now() - boost::chrono::hours(24));
                lastAllLogUpdate = boost::chrono::system_clock::now();
            }

            hourlyTemperatures.push_back(temperature);
            if (boost::chrono::system_clock::now() - lastHourlyUpdate > LOG_HOUR) {
                updateHourlyLog();
                lastHourlyUpdate = boost::chrono::system_clock::now();
            }
            if (boost::chrono::system_clock::now() - lastMonthlyUpdate > boost::chrono::hours(24 * 30)) {
                clearOldEntries(LOG_TABLE_HOUR, boost::chrono::system_clock::now() - boost::chrono::hours(24 * 30));
                lastMonthlyUpdate = boost::chrono::system_clock::now();
            }

            dailyTotalTemperature += temperature;
            dailyTemperatureCount++;
            if (boost::chrono::system_clock::now() - lastDailyUpdate > LOG_DAY) {
                updateDailyLog();
                lastDailyUpdate = boost::chrono::system_clock::now();
            }
            if (boost::chrono::system_clock::now() - lastYearUpdate > boost::chrono::hours(24 * 365)) {
                clearOldEntries(LOG_TABLE_DAY, boost::chrono::system_clock::now() - boost::chrono::hours(24 * 365));
                lastYearUpdate = boost::chrono::system_clock::now();
            }
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(10));
    }

    sqlite3_close(db);
    return 0;
}