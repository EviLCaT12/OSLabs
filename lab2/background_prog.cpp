#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <wait.h>
#include <string.h>
#endif

#include "background.hpp"


int launch_on_windows(const char *program_path, int &status) {
    STARTUPINFO si{};
    PROCESS_INFORMATION pi;
    int success = CreateProcess(program_path, 
                               NULL,         
                               NULL,         
                               NULL,         
                               FALSE,        
                               0,           
                               NULL,         
                               NULL,         
                               &si,          
                               &pi           
    );

    if (success == 0) {
        status = GetLastError();
    }

    return pi.dwProcessId;
}


int launch_on_unix(const char *program_path, int &status) {
    pid_t pid;
    char *const argv[] = {NULL};
    status = posix_spawnp(
        &pid,
        program_path,
        NULL,
        NULL,
        argv,
        NULL);

    return pid;
}

int launch_program_with_status(const char *program_path, int &status) {
#ifdef _WIN32
    return launch_on_windows(program_path, status);
#else
    return launch_on_unix(program_path, status);
#endif
}

int launch_program(const char *program_path) {
    int status = 0;
    return launch_program_with_status(program_path, status);
}


int await_on_windows(const int pid, int* exit_code) {
    HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    int status = WaitForSingleObject(handle, INFINITE);
    if (exit_code != nullptr) {
        GetExitCodeProcess(handle, (unsigned long*)exit_code);
    }
    CloseHandle(handle);

    return status;
}


int await_on_unix(const int pid, int* exit_code) {
    int status;
    waitpid(pid, &status, 0);
    if (exit_code != nullptr) {
        *exit_code = WEXITSTATUS(status);
    }

    return WTERMSIG(status);
}

int await_program_completion(const int pid, int* exit_code) {
#ifdef _WIN32
    return await_on_windows(pid, exit_code);
#else
    return await_on_unix(pid, exit_code);
#endif
}