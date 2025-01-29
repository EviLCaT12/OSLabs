#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include "com.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#endif

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
#ifdef _WIN32
    HANDLE serialPortHandle = initializeSerialConnection("\\\\.\\COM5");
    while (true) {
        float tempValue = -10.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 50.0f));
        std::string tempStr = std::to_string(tempValue) + "\n";
        DWORD bytesSent;
        if (!WriteFile(serialPortHandle, tempStr.c_str(), tempStr.size(), &bytesSent, nullptr)) {
            std::cerr << "Ошибка записи в COM-порт." << std::endl;
            break;
        }
        Sleep(1000);
    }
    // Закрытие COM-порта
    CloseHandle(serialPortHandle);
#else
    int portFd = establishSerialLink("/dev/ttyUSB0");
    while (true) {
        float tempValue = -10.0f + static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX / 50.0f));
        std::string tempStr = std::to_string(tempValue) + "\n";
        if (write(portFd, tempStr.c_str(), tempStr.size()) == -1) {
            perror("Ошибка записи в эмулированный последовательный порт");
            break;
        }
        sleep(1);
    }
    close(portFd);
#endif
    return EXIT_SUCCESS;
}