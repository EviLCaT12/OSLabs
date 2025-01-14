#include <iostream>
#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif // _WIN32

void waitFor(int seconds);

void waitFor(int seconds) {
    #ifdef _WIN32
        Sleep(seconds * 5);
    #else
        sleep(seconds);
    #endif
}

int main() {
    std::cout << "Начало выполнения длительной задачи..." << std::endl;
    for (int i = 1; i <= 10; ++i) {
        std::cout << "Шаг " << i << " из 10..." << std::endl;
        waitFor(1); 
    }
    std::cout << "Длительная задача завершена!" << std::endl;
    return 0; 
}