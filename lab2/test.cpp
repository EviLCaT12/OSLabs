#include "background.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
#ifdef _WIN32
    std::string programPath = ".\\long_task.exe";
#else
    std::string programPath = "./long_task.exe";
#endif
    
    bool waitForCompletion = (argc <= 1) || std::stoi(argv[1]);

    if (argc > 2) {
        programPath = argv[2];
    }

    int processId = start_background(programPath.c_str());
    if (processId == -1) {
        std::cerr << "Ошибка: не удалось запустить программу." << std::endl;
        return 1;
    }

    int exitCode = 0, status = 0;
    if (waitForCompletion) {
        status = wait_program(processId, &exitCode);
        if (status != 0) {
            std::cerr << "Ошибка: не удалось дождаться завершения программы." << std::endl;
            return 1;
        }
    }

    std::cout << "Программа завершена. PID: " << processId
              << ", Код завершения: " << exitCode
              << ", Статус: " << status << std::endl;

    return 0;
}