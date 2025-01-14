#pragma once

int launch_program(const char *program_path);

int launch_program_with_status(const char *program_path, int &status);

int await_program_completion(const int pid, int* exit_code = nullptr);