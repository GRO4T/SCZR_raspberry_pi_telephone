#include "ipc.hpp"

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>

union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};

Semaphore::Semaphore(const char *unique_path, int unique_id) {
  key_t key = ftok(unique_path, unique_id);
  if (key == -1)
    throw std::runtime_error("Cannot create key for semaphore");

  id = semget(key, 1, IPC_CREAT | 0660);
  if (id == -1)
    throw std::runtime_error("Cannot acquire semaphore.");
}

Semaphore::Semaphore(const char *unique_path, int unique_id, int value)
    : Semaphore(unique_path, unique_id) {
  semun val;
  val.val = value;
  if (semctl(id, 0, SETVAL, val) == -1)
    throw std::runtime_error("Cannot set initial value");
}

Semaphore::Semaphore(const Semaphore &other) {
  id = other.id;
}

void Semaphore::P() {
  sembuf op;
  op.sem_num = 0;
  op.sem_op = -1;
  op.sem_flg = 0;

  if (semop(id, &op, 1) == -1)
    throw std::runtime_error("Cannot lock semaphore.");
}

void Semaphore::V() {
  sembuf op;
  op.sem_num = 0;
  op.sem_op = 1;
  op.sem_flg = 0;

  if (semop(id, &op, 1) == -1)
    throw std::runtime_error("Cannot unlock semaphore.");
}
