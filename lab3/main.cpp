#include "shared_memory.hpp"
#include "task_manager.hpp"
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <format>

#define LOG_FILE "task_log.txt"

struct TaskData
{
    int counter = 0;
    int active_copies = 0;
    int total_processes = 0;
    int main_process_id = -1;
};

memlib::SharedMem<TaskData> get_shared_memory()
{
    return memlib::SharedMem<TaskData>("task_shared_memory");
}

template <typename T>
void log_message(memlib::SharedMem<TaskData> &shared_memory, const T &message)
{
    shared_memory.lock();
    std::ofstream log(LOG_FILE, std::ios::app);
    log << message << "\n";
    shared_memory.unlock();
}

bool is_any_process_running(memlib::SharedMem<TaskData> &shared_memory)
{
    shared_memory.lock();
    bool is_running = shared_memory.data()->total_processes > 0;
    shared_memory.unlock();
    return is_running;
}

std::string get_current_time()
{
    return tasklib::get_current_timestamp();
}

std::string get_process_id()
{
    return std::to_string(tasklib::get_current_process_id());
}

void wait_for_main_role(memlib::SharedMem<TaskData> &shared_memory, const std::atomic_bool &is_running)
{
    int current_pid = tasklib::get_current_process_id();
    auto sleep_duration = std::chrono::milliseconds(10);

    do
    {
        shared_memory.lock();
        if (shared_memory.data()->main_process_id == -1)
        {
            shared_memory.data()->main_process_id = current_pid;
        }
        shared_memory.unlock();
        if (shared_memory.data()->main_process_id == current_pid)
        {
            break;
        }
        std::this_thread::sleep_for(sleep_duration);
    } while (is_running);
}

void counter_thread(memlib::SharedMem<TaskData> &shared_memory, const std::atomic_bool &is_running)
{
    auto sleep_duration = std::chrono::milliseconds(300);

    while (shared_memory.is_valid() && is_running)
    {
        shared_memory.lock();
        shared_memory.data()->counter += 1;
        shared_memory.unlock();
        std::this_thread::sleep_for(sleep_duration);
    }
}

void copy_thread(memlib::SharedMem<TaskData> &shared_memory, const std::atomic_bool &is_running, const char *program_name)
{
    wait_for_main_role(shared_memory, is_running);

    auto sleep_duration = std::chrono::seconds(3);

    while (shared_memory.is_valid() && is_running)
    {
        shared_memory.lock();
        if (shared_memory.data()->active_copies > 0)
        {
            std::string message = std::format("[{} | {}] Failed to start copies: {} copies are still running.",
                                              get_current_time(), get_process_id(), shared_memory.data()->active_copies);
            log_message(shared_memory, message);
        }
        else
        {
            int status = 0;
            char *argv[] = {(char *)program_name, (char *)"1", nullptr};
            tasklib::launch_process(2, argv, status);

            argv[1] = (char *)"2";
            tasklib::launch_process(2, argv, status);
        }
        shared_memory.unlock();
        std::this_thread::sleep_for(sleep_duration);
    }
}


void log_thread(memlib::SharedMem<TaskData> &shared_memory, const std::atomic_bool &is_running)
{
    wait_for_main_role(shared_memory, is_running);

    auto sleep_duration = std::chrono::seconds(1);

    while (shared_memory.is_valid() && is_running)
    {
        shared_memory.lock();
        int counter = shared_memory.data()->counter;
        std::string message = std::format("[{} | {}] Counter: {}", get_current_time(), get_process_id(), counter);
        shared_memory.unlock();
        log_message(shared_memory, message);
        std::this_thread::sleep_for(sleep_duration);
    }

    shared_memory.lock();
    if (shared_memory.data()->main_process_id == tasklib::get_current_process_id())
    {
        shared_memory.data()->main_process_id = -1;
    }
    shared_memory.unlock();
}

enum class ProgramBehavior
{
    MAIN,
    COPY_1,
    COPY_2
};

std::string behavior_to_string(ProgramBehavior behavior)
{
    switch (behavior)
    {
    case ProgramBehavior::MAIN:
        return "Main Program";
    case ProgramBehavior::COPY_1:
        return "Copy 1";
    case ProgramBehavior::COPY_2:
        return "Copy 2";
    default:
        return "Unknown";
    }
}

int main(int argc, char **argv)
{
    auto shared_memory = get_shared_memory();
    if (!shared_memory.is_valid())
    {
        std::cerr << "Failed to initialize shared memory!" << std::endl;
        return -1;
    }

    ProgramBehavior behavior = ProgramBehavior::MAIN;
    if (argc > 1)
    {
        behavior = static_cast<ProgramBehavior>(std::atoi(argv[1]));
    }

    if (!is_any_process_running(shared_memory))
    {
        std::ofstream log(LOG_FILE);
        log.clear();
    }

    std::string startup_message = std::format("[{} | {}] Started {}", get_current_time(), get_process_id(), behavior_to_string(behavior));
    log_message(shared_memory, startup_message);

    if (behavior == ProgramBehavior::MAIN)
    {
        std::cout << std::format("Started {} : PID={}. Log file: {}\n", argv[0], get_process_id(), LOG_FILE);

        std::vector<std::thread> threads;
        std::atomic_bool is_running = true;

        shared_memory.lock();
        shared_memory.data()->total_processes++;
        threads.emplace_back(counter_thread, std::ref(shared_memory), std::ref(is_running));
        threads.emplace_back(log_thread, std::ref(shared_memory), std::ref(is_running));
        threads.emplace_back(copy_thread, std::ref(shared_memory), std::ref(is_running), argv[0]);
        shared_memory.unlock();

        std::string command;
        int new_counter_value = 0;
        while (true)
        {
            std::cin >> command;
            if (command == "exit" || command == "e")
            {
                break;
            }
            else if (command == "modify" || command == "m")
            {
                std::cout << "Enter new counter value: ";
                std::cin >> new_counter_value;
                shared_memory.lock();
                shared_memory.data()->counter = new_counter_value;
                shared_memory.unlock();
            }
            else if (command == "show" || command == "s")
            {
                shared_memory.lock();
                int counter = shared_memory.data()->counter;
                shared_memory.unlock();
                std::cout << "Current counter value: " << counter << "\n";
            }
        }

        std::cout << "Exiting...\n";

        shared_memory.lock();
        shared_memory.data()->total_processes--;
        shared_memory.unlock();

        is_running = false;
        for (auto &thread : threads)
        {
            thread.join();
        }

        std::cout << "Goodbye!\n";
    }
    else if (behavior == ProgramBehavior::COPY_1)
    {
        shared_memory.lock();
        shared_memory.data()->counter += 10;
        shared_memory.unlock();
    }
    else if (behavior == ProgramBehavior::COPY_2)
    {
        shared_memory.lock();
        shared_memory.data()->counter *= 2;
        shared_memory.data()->active_copies++;
        shared_memory.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(2));

        shared_memory.lock();
        shared_memory.data()->counter /= 2;
        shared_memory.data()->active_copies--;
        shared_memory.unlock();
    }

    std::string shutdown_message = std::format("[{} | {}] Finished {}", get_current_time(), get_process_id(), behavior_to_string(behavior));
    log_message(shared_memory, shutdown_message);

    return 0;
}