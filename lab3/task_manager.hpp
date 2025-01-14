#ifndef TASK_MANAGER_HPP
#define TASK_MANAGER_HPP

#ifdef _WIN32
#include <windows.h>
#else
#include <spawn.h>
#include <sys/wait.h>
#include <ctime>
#include <chrono>
#endif

#include <string>
#include <format>

namespace tasklib
{
    int launch_process(int argc, char **argv, int &status)
    {
#ifdef _WIN32
        std::string command = "";
        for (int i = 0; i < argc; ++i)
        {
            command += std::format("{} ", argv[i]);
        }

        STARTUPINFOA si{};
        PROCESS_INFORMATION pi;
        int success = CreateProcessA(
            nullptr,
            (char *)command.c_str(),
            nullptr,
            nullptr,
            FALSE,
            0,
            nullptr,
            nullptr,
            &si,
            &pi
        );

        if (success == 0)
        {
            status = GetLastError();
        }
        else
        {
            status = 0;
        }

        return pi.dwProcessId;
#else
        pid_t pid;
        status = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv, nullptr);
        return pid;
#endif
    }

    int get_current_process_id()
    {
#ifdef _WIN32
        return GetCurrentProcessId();
#else
        return getpid();
#endif
    }

    std::string get_current_timestamp()
    {
#ifdef _WIN32
        SYSTEMTIME st;
        GetLocalTime(&st);

        return std::format(
            "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds
        );
#else
        auto now = std::chrono::system_clock::now();
        time_t t = std::chrono::system_clock::to_time_t(now);
        tm *st = localtime(&t);

        return std::format(
            "{:04}-{:02}-{:02} {:02}:{:02}:{:02}.{:03}",
            st->tm_year + 1900, st->tm_mon + 1, st->tm_mday,
            st->tm_hour, st->tm_min, st->tm_sec,
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() % 1000
        );
#endif
    }

    int wait_for_process(int pid)
    {
#ifdef _WIN32
        HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        int status = WaitForSingleObject(handle, INFINITE);
        CloseHandle(handle);
        return status;
#else
        int status;
        waitpid(pid, &status, 0);
        return status;
#endif
    }
}

#endif