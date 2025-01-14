#include <iostream>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif 


void delayExecution(int milliseconds);

void delayExecution(int milliseconds) {
    #ifdef _WIN32
        Sleep(milliseconds);
    #else
        usleep(milliseconds * 1000);
    #endif 
}

int main() {
    std::cout << "Привет! Я завершу работу через 5 секунд." << std::endl;
    delayExecution(5000); 
    std::cout << "Завершение работы..." << std::endl;
    return 0; 
}