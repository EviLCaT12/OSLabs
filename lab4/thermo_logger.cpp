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

void clearOldEntries(const std::string& filename, const boost::chrono::system_clock::time_point& threshold) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Failed to open log file: " << filename << std::endl;
        return;
    }

    std::ofstream outputFile("tempfile.txt");

    if (!outputFile.is_open()) {
        std::cerr << "Failed to create temporary file" << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inputFile, line)) {
        std::string timestampStr = line.substr(1, 19); // YYYY/MM/DD HH:MM:SS
        std::tm timestamp = {};
        std::istringstream(timestampStr) >> std::get_time(&timestamp, "%Y/%m/%d %H:%M:%S");
        auto entryTime = boost::chrono::system_clock::from_time_t(std::mktime(&timestamp));

        if (entryTime >= threshold) {
            outputFile << line << '\n';
        }
    }

    inputFile.close();
    outputFile.close();

    std::remove(filename.c_str());
    std::rename("tempfile.txt", filename.c_str());
}

void logTemperature(double temperature, const string& filename) {
    ofstream logfile(filename, ios::app);
    if (logfile.is_open()) {
        auto now = boost::chrono::system_clock::to_time_t(boost::chrono::system_clock::now());
        std::tm* ptm{std::localtime(&now)};
        logfile << "[" << std::put_time(ptm, "%Y/%m/%d %H:%M:%S") << "] " << temperature << " C\n";
    } else {
        cerr << "Failed to open logfile: " << filename << endl;
    }
}

void updateHourlyLog() {
    if (!hourlyTemperatures.empty()) {
        double sum = 0.0;
        for (double temp : hourlyTemperatures) {
            sum += temp;
        }
        double average = sum / hourlyTemperatures.size();
        logTemperature(average, LOG_FILE_HOUR);
        hourlyTemperatures.clear();
    }
}

void updateDailyLog() {
    if (dailyTemperatureCount > 0) {
        double average = dailyTotalTemperature / dailyTemperatureCount;
        logTemperature(average, LOG_FILE_DAY);
        dailyTotalTemperature = 0.0;
        dailyTemperatureCount = 0;
    }
}

int main(int argc, char** argv) {
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
        // Get serial port handle
        HANDLE serialHandle = port.native_handle();

        // Configure serial port settings (replace with your desired settings)
        DCB dcbSerialParams = { 0 };
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        if (!GetCommState(serialHandle, &dcbSerialParams)) {
            cerr << "Failed to get serial port state" << endl;
            return 1;
        }

        // Set flow control to none
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
            
            logTemperature(temperature, LOG_FILE_ALL);
            if (boost::chrono::system_clock::now() - lastAllLogUpdate > boost::chrono::hours(24)) {
                clearOldEntries(LOG_FILE_ALL, boost::chrono::system_clock::now() - boost::chrono::hours(24));
                lastAllLogUpdate = boost::chrono::system_clock::now();
            }

            hourlyTemperatures.push_back(temperature);
            if (boost::chrono::system_clock::now() - lastHourlyUpdate > LOG_HOUR) {
                updateHourlyLog();
                lastHourlyUpdate = boost::chrono::system_clock::now();
            }
            if (boost::chrono::system_clock::now() - lastMonthlyUpdate > boost::chrono::hours(24 * 30)) {
                clearOldEntries(LOG_FILE_HOUR, boost::chrono::system_clock::now() - boost::chrono::hours(24 * 30));
                lastMonthlyUpdate = boost::chrono::system_clock::now();
            }

            dailyTotalTemperature += temperature;
            dailyTemperatureCount++;
            if (boost::chrono::system_clock::now() - lastDailyUpdate > LOG_DAY) {
                updateDailyLog();
                lastDailyUpdate = boost::chrono::system_clock::now();
            }
            if (boost::chrono::system_clock::now() - lastYearUpdate > boost::chrono::hours(24 * 365)) {
                clearOldEntries(LOG_FILE_HOUR, boost::chrono::system_clock::now() - boost::chrono::hours(24 * 365));
                lastYearUpdate = boost::chrono::system_clock::now();
            }
        }

        boost::this_thread::sleep_for(boost::chrono::seconds(10));
    }

    return 0;
}