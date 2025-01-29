#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <fstream>
#include <ctime>
#include "com.hpp"
#include "sqlite3.h"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

struct TempLogEntry {
    std::chrono::system_clock::time_point logTime;
    double tempValue;
};

void setupNetwork() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "Ошибка инициализации сети: " << result << std::endl;
        exit(EXIT_FAILURE);
    }
#endif
}

void cleanupNetwork() {
#ifdef _WIN32
    WSACleanup();
#endif
}

void closeConnection(int socketId) {
#ifdef _WIN32
    closesocket(socketId);
#else
    close(socketId);
#endif
}

std::string formatTimestamp(const std::chrono::system_clock::time_point &logTime) {
    auto timeT = std::chrono::system_clock::to_time_t(logTime);
    std::tm tmStruct = *std::localtime(&timeT);
    std::ostringstream oss;
    oss << std::put_time(&tmStruct, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void storeTemperature(sqlite3 *db, const TempLogEntry &entry) {
    const char *insertCmd = "INSERT INTO TemperatureLogs (timestamp, temperature) VALUES (?, ?);";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, insertCmd, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Ошибка подготовки команды: " << sqlite3_errmsg(db) << std::endl;
        return;
    }
    std::string timestampStr = formatTimestamp(entry.logTime);
    sqlite3_bind_text(stmt, 1, timestampStr.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_double(stmt, 2, entry.tempValue);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "Ошибка выполнения команды: " << sqlite3_errmsg(db) << std::endl;
    }
    sqlite3_finalize(stmt);
}

std::string fetchHistoryEndpoint(sqlite3 *db, const std::string &startTime, const std::string &endTime) {
    std::string query = "SELECT timestamp, temperature FROM TemperatureLogs WHERE timestamp BETWEEN ? AND ? ORDER BY id DESC;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nDatabase error.";
    }
    sqlite3_bind_text(stmt, 1, startTime.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, endTime.c_str(), -1, SQLITE_STATIC);
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n[";
    bool isFirst = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (!isFirst) {
            response += ",";
        }
        isFirst = false;
        response += "{";
        response += "\"timestamp\": \"" + std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))) + "\",";
        response += "\"temperature\": " + std::to_string(sqlite3_column_double(stmt, 1));
        response += "}";
    }
    response += "]";
    sqlite3_finalize(stmt);
    return response;
}

std::string decodeAndFormatDate(const std::string& input) {
    std::string decoded = input;
    
    size_t pos = decoded.find("%3A");
    while (pos != std::string::npos) {
        decoded.replace(pos, 3, ":");
        pos = decoded.find("%3A", pos + 1);
    }
    pos = decoded.find("T");
    if (pos != std::string::npos) {
        decoded.replace(pos, 1, " ");
    }
    return decoded;
}

std::string getCurrentTempEndpoint(sqlite3 *db) {
    const char *query = "SELECT timestamp, temperature FROM TemperatureLogs ORDER BY id DESC LIMIT 1;";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nDatabase error.";
    }
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        response += "\"timestamp\": \"" + std::string(reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0))) + "\",";
        response += "\"temperature\": " + std::to_string(sqlite3_column_double(stmt, 1));
    } else {
        response += "\"error\": \"No data available\"";
    }
    response += "}";
    sqlite3_finalize(stmt);
    return response;
}

std::string getStatsEndpoint(sqlite3 *db) {
    const char *query = "SELECT AVG(temperature) FROM TemperatureLogs WHERE timestamp >= datetime('now', '-1 day');";
    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
        return "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\nDatabase error.";
    }
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        response += "\"average_temperature\": " + std::to_string(sqlite3_column_double(stmt, 0));
    } else {
        response += "\"error\": \"No data available\"";
    }
    response += "}";
    sqlite3_finalize(stmt);
    return response;
}

std::string processRequest(const std::string &request, sqlite3 *db) {
    if (request.find("GET /temperature") == 0) {
        return getCurrentTempEndpoint(db);
    } else if (request.find("GET /stats") == 0) {
        return getStatsEndpoint(db);
    } else if (request.find("GET /history") == 0) {
        auto start_pos = request.find("start_datetime=");
        auto end_pos = request.find("end_datetime=");
        std::string start_datetime = "1970-01-01T00:00"; 
        std::string end_datetime = "2100-01-01T00:00"; 
        if (start_pos != std::string::npos) {
            size_t start_datetime_end_pos = request.find("&", start_pos);  
            if (start_datetime_end_pos == std::string::npos) {
                start_datetime_end_pos = request.length(); 
            }
            start_datetime = request.substr(start_pos + 15, start_datetime_end_pos - (start_pos + 15)); 
        }
        if (end_pos != std::string::npos) {
            size_t end_datetime_end_pos = request.find("&", end_pos);  
            if (end_datetime_end_pos == std::string::npos) {
                end_datetime_end_pos = request.length(); 
            }
            end_datetime = request.substr(end_pos + 13, end_datetime_end_pos - (end_pos + 14)); 
        }
        return fetchHistoryEndpoint(db, decodeAndFormatDate(start_datetime), decodeAndFormatDate(end_datetime));
    } else {
        return "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nNot Found";
    }
}

int main() {
#ifdef _WIN32
    const char* cmd_name = "cmd /c timeout /t 5 >nul 2>&1";
    SetConsoleOutputCP(CP_UTF8);
#else
    const char* cmd_name = "sleep 5";
#endif
    setupNetwork();
    sqlite3 *db;
    if (sqlite3_open("temperature_logs.db", &db)) {
        std::cerr << "Ошибка открытия базы данных: " << sqlite3_errmsg(db) << std::endl;
        return EXIT_FAILURE;
    }
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    const int PORT = 8080;
#ifdef _WIN32
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета: " << WSAGetLastError() << std::endl;
        cleanupNetwork();
        return -1;
    }
#else
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Ошибка создания сокета");
        return -1;
    }
#endif
#ifdef _WIN32
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        std::cerr << "Ошибка настройки сокета: " << WSAGetLastError() << std::endl;
        closeConnection(server_fd);
        cleanupNetwork();
        return -1;
    }
#else
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Ошибка настройки сокета");
        closeConnection(server_fd);
        return -1;
    }
#endif

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
#ifdef _WIN32
        std::cerr << "Ошибка привязки: " << WSAGetLastError() << std::endl;
#else
        perror("Ошибка привязки");
#endif
        closeConnection(server_fd);
        cleanupNetwork();
        return -1;
    }
    if (listen(server_fd, 3) < 0) {
#ifdef _WIN32
        std::cerr << "Ошибка прослушивания: " << WSAGetLastError() << std::endl;
#else
        perror("Ошибка прослушивания");
#endif
        closeConnection(server_fd);
        cleanupNetwork();
        return -1;
    }
    std::cout << "Сервер запущен на порту " << PORT << std::endl;
    while (true) {
        int addrlen = sizeof(address);
        int clientConn;
#ifdef _WIN32
        clientConn = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (clientConn == INVALID_SOCKET) {
            std::cerr << "Ошибка принятия соединения: " << WSAGetLastError() << std::endl;
#else
        clientConn = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (clientConn < 0) {
            perror("Ошибка принятия соединения");
#endif
            closeConnection(server_fd);
            cleanupNetwork();
            sqlite3_close(db);
            return -1;
        }
        std::cout << "Принято новое соединение" << std::endl;
        char buffer[1024] = {0};
        ssize_t bytesRead;
#ifdef _WIN32
        bytesRead = recv(clientConn, buffer, sizeof(buffer), 0);
#else
        bytesRead = read(clientConn, buffer, sizeof(buffer));
#endif
        if (bytesRead > 0) {
            std::string request(buffer, bytesRead);
            std::cout << "Получен запрос:\n" << request << std::endl;
            std::string response = processRequest(request, db);
#ifdef _WIN32
            send(clientConn, response.c_str(), response.length(), 0);
#else
            send(clientConn, response.c_str(), response.length(), 0);
#endif
        }
        closeConnection(clientConn);
    }
    closeConnection(server_fd);
    cleanupNetwork();
    sqlite3_close(db);
    return EXIT_SUCCESS;
}