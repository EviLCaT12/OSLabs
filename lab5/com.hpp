#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iomanip>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#endif

#ifdef _WIN32
HANDLE initializeSerialConnection(const char* portIdentifier) {
    HANDLE serialHandle = CreateFileA(portIdentifier, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, 0, 0);
    if (serialHandle == INVALID_HANDLE_VALUE) {
        std::cerr << "Ошибка при открытии порта: " << portIdentifier << std::endl;
        exit(EXIT_FAILURE);
    }
    DCB serialParameters = { 0 };
    serialParameters.DCBlength = sizeof(serialParameters);
    if (!GetCommState(serialHandle, &serialParameters)) {
        std::cerr << "Не удалось получить параметры порта." << std::endl;
        exit(EXIT_FAILURE);
    }
    serialParameters.BaudRate = CBR_9600;
    serialParameters.ByteSize = 8;
    serialParameters.StopBits = ONESTOPBIT;
    serialParameters.Parity = NOPARITY;
    if (!SetCommState(serialHandle, &serialParameters)) {
        std::cerr << "Не удалось установить параметры порта." << std::endl;
        exit(EXIT_FAILURE);
    }
    COMMTIMEOUTS timeSettings = {0};
    timeSettings.ReadIntervalTimeout = 50;
    timeSettings.ReadTotalTimeoutConstant = 50;
    timeSettings.ReadTotalTimeoutMultiplier = 10;
    timeSettings.WriteTotalTimeoutConstant = 50;
    timeSettings.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(serialHandle, &timeSettings)) {
        std::cerr << "Не удалось задать таймауты порта." << std::endl;
        exit(EXIT_FAILURE);
    }
    return serialHandle;
}

double fetchTemperatureData(HANDLE serialHandle) {
    DWORD bytesRead;
    char buffer[256] = { 0 };
    if (ReadFile(serialHandle, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
        return std::strtod(buffer, nullptr);
    } else {
        std::cerr << "Ошибка чтения данных с порта." << std::endl;
        return 0.0;
    }
}
#else
int establishSerialLink(const std::string& portName) {
    int fileDescriptor = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fileDescriptor == -1) {
        std::cerr << "Ошибка при открытии порта: " << portName << std::endl;
        exit(EXIT_FAILURE);
    }
    struct termios terminalOptions;
    tcgetattr(fileDescriptor, &terminalOptions);
    terminalOptions.c_cflag = B9600 | CS8 | CLOCAL | CREAD; 
    terminalOptions.c_iflag = IGNPAR; 
    terminalOptions.c_oflag = 0;
    terminalOptions.c_lflag = ICANON;
    tcsetattr(fileDescriptor, TCSANOW, &terminalOptions);
    return fileDescriptor;
}

double retrieveTemperatureValue(int fd) {
    char buffer[256];
    int readBytes = read(fd, buffer, sizeof(buffer) - 1);
    if (readBytes > 0) {
        buffer[readBytes] = '\0'; 
        return std::strtod(buffer, nullptr); 
    } else {
        std::cerr << "Ошибка чтения данных с порта." << std::endl;
        return 0.0;
    }
}
#endif