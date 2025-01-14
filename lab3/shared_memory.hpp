#ifndef SHARED_MEMORY_HPP
#define SHARED_MEMORY_HPP

#include <cstring>
#include <cstdlib>
#ifdef _WIN32
#include <windows.h>
#define SHM_HANDLE HANDLE
#define INVALID_SHM_HANDLE (NULL)
#define SHM_SEMAPHORE HANDLE
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>
#define SHM_HANDLE int
#define INVALID_SHM_HANDLE (-1)
#define SHM_SEMAPHORE sem_t *
#endif

#define SEMAPHORE_SUFFIX "_sem"

namespace memlib
{
    template <typename T>
    class SharedMemory
    {
    public:
        SharedMemory(const char *name, bool create_if_not_exists = true)
            : _handle(INVALID_SHM_HANDLE), _memory(nullptr), _semaphore(nullptr)
        {
            _name = (char *)malloc(strlen(name) + strlen(SHM_PREFIX) + 1);
            strcpy(_name, SHM_PREFIX);
            strcat(_name, name);

            _sem_name = (char *)malloc(strlen(_name) + strlen(SEMAPHORE_SUFFIX) + 1);
            strcpy(_sem_name, _name);
            strcat(_sem_name, SEMAPHORE_SUFFIX);

            bool is_new = false;
            if (!open_memory() && create_if_not_exists)
            {
                is_new = create_memory();
            }

            if (map_memory() && is_new)
            {
                _memory->reference_count = 0;
                _memory->data = T();
            }

            if (is_valid())
            {
                lock();
                _memory->reference_count++;
                unlock();
            }
            else
            {
                if (is_new)
                {
                    destroy_memory();
                }
                else
                {
                    close_memory();
                }
            }
        }

        ~SharedMemory()
        {
            if (is_valid())
            {
                lock();
                _memory->reference_count--;
                if (_memory->reference_count <= 0)
                {
                    destroy_memory();
                }
                else
                {
                    close_memory();
                }
                unlock();
            }
            free(_name);
            free(_sem_name);
        }

        bool is_valid() const { return _handle != INVALID_SHM_HANDLE && _semaphore != nullptr && _memory != nullptr; }
        void lock() { lock_semaphore(); }
        void unlock() { unlock_semaphore(); }
        T *data() { return is_valid() ? &_memory->data : nullptr; }

    private:
        bool open_memory()
        {
#ifdef _WIN32
            _handle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, _name);
            if (_handle != INVALID_SHM_HANDLE)
            {
                _semaphore = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, _sem_name);
            }
#else
            _handle = shm_open(_name, O_RDWR, 0644);
            if (_handle != INVALID_SHM_HANDLE)
            {
                _semaphore = sem_open(_sem_name, 0);
                if (_semaphore == SEM_FAILED)
                {
                    _semaphore = nullptr;
                }
            }
#endif
            return _handle != INVALID_SHM_HANDLE && _semaphore != nullptr;
        }

        bool create_memory()
        {
#ifdef _WIN32
            _handle = CreateFileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(SharedMemoryData), _name);
            if (_handle != INVALID_SHM_HANDLE)
            {
                _semaphore = CreateSemaphore(nullptr, 1, 1, _sem_name);
            }
#else
            _handle = shm_open(_name, O_CREAT | O_EXCL | O_RDWR, 0644);
            if (_handle != INVALID_SHM_HANDLE)
            {
                ftruncate(_handle, sizeof(SharedMemoryData));
                _semaphore = sem_open(_sem_name, O_CREAT | O_EXCL, 0644, 1);
                if (_semaphore == SEM_FAILED)
                {
                    _semaphore = nullptr;
                }
            }
#endif
            return _handle != INVALID_SHM_HANDLE && _semaphore != nullptr;
        }

        bool map_memory()
        {
#ifdef _WIN32
            _memory = (SharedMemoryData *)MapViewOfFile(_handle, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(SharedMemoryData));
#else
            void *result = mmap(nullptr, sizeof(SharedMemoryData), PROT_READ | PROT_WRITE, MAP_SHARED, _handle, 0);
            if (result == MAP_FAILED)
            {
                _memory = nullptr;
            }
            else
            {
                _memory = (SharedMemoryData *)result;
            }
#endif
            return _memory != nullptr;
        }

        void close_memory()
        {
#ifdef _WIN32
            if (_memory != nullptr)
            {
                UnmapViewOfFile(_memory);
            }
            if (_handle != INVALID_SHM_HANDLE)
            {
                CloseHandle(_handle);
            }
            if (_semaphore != nullptr)
            {
                CloseHandle(_semaphore);
            }
#else
            if (_memory != nullptr)
            {
                munmap(_memory, sizeof(SharedMemoryData));
            }
            if (_handle != INVALID_SHM_HANDLE)
            {
                close(_handle);
            }
            if (_semaphore != nullptr)
            {
                sem_close(_semaphore);
            }
#endif
            _memory = nullptr;
            _handle = INVALID_SHM_HANDLE;
            _semaphore = nullptr;
        }

        void destroy_memory()
        {
            close_memory();
#ifndef _WIN32
            shm_unlink(_name);
            sem_unlink(_sem_name);
#endif
        }

        void lock_semaphore()
        {
#ifdef _WIN32
            WaitForSingleObject(_semaphore, INFINITE);
#else
            sem_wait(_semaphore);
#endif
        }

        void unlock_semaphore()
        {
#ifdef _WIN32
            ReleaseSemaphore(_semaphore, 1, nullptr);
#else
            sem_post(_semaphore);
#endif
        }

        struct SharedMemoryData
        {
            T data;
            int reference_count;
        } *_memory;

        SHM_SEMAPHORE _semaphore;
        SHM_HANDLE _handle;
        char *_name;
        char *_sem_name;
    };
}

#endif 