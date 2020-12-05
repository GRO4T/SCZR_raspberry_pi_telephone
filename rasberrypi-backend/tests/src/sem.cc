#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>
#include "ipc.hpp"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << argv[0] << " <sem_path>\n";
        return -1;
    }

    int pid = fork();
    if (!pid) {
        Semaphore sem(argv[1], 1, 0);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sem.P();
        std::cout << "Child exited\n";
    } else {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        Semaphore sem(argv[1], 1);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        sem.V(); 
        std::cout << "Parent exited\n";
    }


}
