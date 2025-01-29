#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iomanip>
#include "com.hpp"
#include "sqlite3.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#endif

struct TempRecord {
    std::chrono::system_clock::time_point logTime;
    double tempValue;
};

std::string formatTimestamp(const std::chrono::system_clock::time_point &logTime) {
    auto timeT = std::chrono::system_clock::to_time_t(logTime);
    std::tm tmStruct = *std::localtime(&timeT); 
    std::ostringstream oss;
    oss << std::put_time(&tmStruct, "%Y-%m-%d %H:%M:%S"); 
    return oss.str();
}

void setupDB(sqlite3 *&db) {
    if (sqlite3_open("temperature_logs.db", &db)) {
        std::cerr << "Ошибка при открытии базы данных: " << sqlite3_errmsg(db) << std::endl;
        exit(EXIT_FAILURE);
    }
    const char *createTableCmd = "CREATE TABLE IF NOT EXISTS TemperatureLogs ("
                                 "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                 "timestamp TEXT NOT NULL,"
                                 "temperature REAL NOT NULL);";
    const char *createAvgHourTableCmd = "CREATE TABLE IF NOT EXISTS AvgHourTemp ("
                                        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                        "timestamp TEXT NOT NULL,"
                                        "avg_temp REAL NOT NULL);";
    const char *createAvgDayTableCmd = "CREATE TABLE IF NOT EXISTS AvgDayTemp ("
                                       "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                                       "timestamp TEXT NOT NULL,"
                                       "avg_temp REAL NOT NULL);";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, createTableCmd, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы TemperatureLogs: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(EXIT_FAILURE);
    }
    if (sqlite3_exec(db, createAvgHourTableCmd, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы AvgHourTemp: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(EXIT_FAILURE);
    }
    if (sqlite3_exec(db, createAvgDayTableCmd, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка создания таблицы AvgDayTemp: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        exit(EXIT_FAILURE);
    }
}

void cleanupOldRecords(sqlite3* db, const std::string& tableName, std::chrono::hours maxAge) {
    auto now = std::chrono::system_clock::now();
    auto cutoffTime = now - maxAge;
    std::string timestampStr = formatTimestamp(cutoffTime);
    std::string deleteCmd = "DELETE FROM " + tableName + " WHERE timestamp < '" + timestampStr + "';";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, deleteCmd.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка удаления старых записей из таблицы " << tableName << ": " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void storeTemperatureInDB(sqlite3* db, const TempRecord &entry) {
    cleanupOldRecords(db, "TemperatureLogs", std::chrono::hours(24));
    std::string timestampStr = formatTimestamp(entry.logTime);
    std::string insertCmd = "INSERT INTO TemperatureLogs (timestamp, temperature) VALUES ('" + timestampStr + "', " + std::to_string(entry.tempValue) + ");";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, insertCmd.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка записи в базу данных: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

void storeHourlyAverage(sqlite3* db, double avgTemp, const std::chrono::system_clock::time_point &logTime) {
    cleanupOldRecords(db, "AvgHourTemp", std::chrono::hours(24 * 30));
    std::string timestampStr = formatTimestamp(logTime);
    std::string insertCmd = "INSERT INTO AvgHourTemp (timestamp, avg_temp) VALUES ('" + timestampStr + "', " + std::to_string(avgTemp) + ");";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, insertCmd.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка записи средней температуры за час в базу данных: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}


void storeDailyAverage(sqlite3* db, double avgTemp, const std::chrono::system_clock::time_point &logTime) {
    cleanupOldRecords(db, "AvgDayTemp", std::chrono::hours(24 * 365));
    std::string timestampStr = formatTimestamp(logTime);
    std::string insertCmd = "INSERT INTO AvgDayTemp (timestamp, avg_temp) VALUES ('" + timestampStr + "', " + std::to_string(avgTemp) + ");";
    char *errMsg = nullptr;
    if (sqlite3_exec(db, insertCmd.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Ошибка записи средней температуры за день в базу данных: " << errMsg << std::endl;
        sqlite3_free(errMsg);
    }
}

double computeHourlyAverage(const std::vector<TempRecord> &entries) {
    double total = 0;
    int count = 0;
    auto now = std::chrono::system_clock::now();
    for (const auto &entry : entries) {
        auto elapsedHours = std::chrono::duration_cast<std::chrono::hours>(now - entry.logTime).count();
        if (elapsedHours <= 1) {
            total += entry.tempValue;
            count++;
        }
    }
    return count > 0 ? total / count : 0;
}

//средняя температура за последний день
double computeDailyAverage(const std::vector<TempRecord> &entries) {
    double total = 0;
    int count = 0;
    auto now = std::chrono::system_clock::now();
    for (const auto &entry : entries) {
        auto elapsedHours = std::chrono::duration_cast<std::chrono::hours>(now - entry.logTime).count();
        if (elapsedHours <= 24) {
            total += entry.tempValue;
            count++;
        }
    }
    return count > 0 ? total / count : 0;
}

int main() {
    sqlite3* db;
    setupDB(db);
    if (sqlite3_open("temperature_logs.db", &db)) {
        std::cerr << "Ошибка при открытии базы данных: " << sqlite3_errmsg(db) << std::endl;
        return EXIT_FAILURE;
    }
    std::vector<TempRecord> tempEntries;
#ifdef _WIN32
    const char* cmd_name = "cmd /c timeout /t 5 >nul 2>&1";
    SetConsoleOutputCP(CP_UTF8);
#else
    const char* cmd_name = "sleep 5";
#endif
#ifdef _WIN32
    HANDLE serialPortFd = initializeSerialConnection("\\\\.\\COM6");
#else
    int serialPortFd = establishSerialLink("/dev/ttyUSB0");
#endif
    auto lastReadTime = std::chrono::steady_clock::now();
    long currentDay = 1;
    while (true) {
        double temperature;
        temperature = fetchTemperatureData(serialPortFd);
        TempRecord entry = { std::chrono::system_clock::now(), temperature };
        tempEntries.push_back(entry);
        storeTemperatureInDB(db, entry);
        // Каждые 60 минут записывать среднюю температуру за час
        auto now = std::chrono::system_clock::now();
        auto elapsedHours = std::chrono::duration_cast<std::chrono::hours>(now - entry.logTime).count();
        if (elapsedHours != 0 && elapsedHours % 1 == 0) {  // Каждые 1 час
            double hourlyAvg = computeHourlyAverage(tempEntries);
            storeHourlyAverage(db, hourlyAvg, entry.logTime);
        }
        // Каждые 24 часа записывать среднюю температуру за день
        auto elapsedDays = std::chrono::duration_cast<std::chrono::hours>(now - entry.logTime).count();
        if (elapsedHours != 0 && elapsedHours % 24 == 0 && elapsedDays == currentDay) {  // Каждые 24 часа
            currentDay++;
            double dailyAvg = computeDailyAverage(tempEntries);
            storeDailyAverage(db, dailyAvg, entry.logTime);
        }
#ifdef _WIN32
        Sleep(300);
#else
        sleep(1);
#endif
    }
#ifdef __linux__
    close(serialPortFd);
#elif _WIN32
    CloseHandle(serialPortFd);
#endif
    sqlite3_close(db);
    return EXIT_SUCCESS;
}