#include "ipc.hpp"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int* array;
  struct seminfo *__buf;
};

int Semaphore::next_proj_id() {
    static int proj_id = 1;
    return proj_id++;
}

Semaphore::Semaphore(int value): Semaphore(".", next_proj_id(), value) {
    
}

Semaphore::Semaphore(const char* path, int proj_id,int value) {
    key_t key = ftok(path, proj_id);
    if(key == -1) {
        std::perror("Cannot create key for semaphore");
        throw std::runtime_error("Cannot create key for semaphore");
    }
    int semid = semget(key, 1, IPC_CREAT | 0660);
    if(semid == -1) {
        std::perror("Cannot acquire semaphore.");
        throw std::runtime_error("Cannot acquire semaphore.");
    }
    semun val;
    val.val = value;
    if(semctl(semid, 0, SETVAL, val) == -1) {
        std::perror("Cannot set initial value");
        throw std::runtime_error("Cannot set initial value");
    }
    id = semid;
}
Semaphore::Semaphore(const Semaphore& other) {
    id = other.id;
}

void Semaphore::P() {
   sembuf op;
   op.sem_num = 0;
   op.sem_op = -1;
   op.sem_flg = 0;
   if(semop(id, &op, 1) == -1) {
       std::perror("Cannot lock semaphore.");
       throw std::runtime_error("Cannot lock semaphore.");
   }
}
void Semaphore::V() {
   sembuf op;
   op.sem_num = 0;
   op.sem_op = 1;
   op.sem_flg = 0;
   if(semop(id, &op, 1) == -1) {
       std::perror("Cannot unlock semaphore.");
       throw std::runtime_error("Cannot unlock semaphore.");
   }
}