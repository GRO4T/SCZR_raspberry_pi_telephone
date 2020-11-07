#ifndef __IPC_HPP__
#define __IPC_HPP__

#include <limits>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <array>
#include <algorithm>
#include <limits>
#include <atomic>
#include "error.hpp"

template<typename T>
class shared_mem_ptr {

    struct ref {
        std::size_t use_count = 0;
        T obj;
    };

    ref* ptr;
    int memfd;
    const char* path;
public:

    template<typename ... Args>
    shared_mem_ptr(const char* ptr, Args&&... args);
    shared_mem_ptr(const shared_mem_ptr& other);
    shared_mem_ptr(shared_mem_ptr&& other);

    ~shared_mem_ptr();

    T* get() noexcept { return &ptr->obj; }
    const T* get() const noexcept { return &ptr->obj; }
    T& operator*() noexcept { return ptr->obj; }
    const T& operator*() const noexcept { return ptr->obj; }
    T* operator->() noexcept { return &ptr->obj; }
    const T* operator->() const noexcept { return &ptr->obj; }
};

template<typename T, std::size_t N>
class fast_deque {
    static constexpr std::size_t invalid_index = std::numeric_limits<std::size_t>::max();

    std::array<T, N> array;
    std::size_t front_index = invalid_index;
    std::size_t back_index = 0; 

    static std::size_t next(std::size_t n) noexcept;
    static std::size_t prev(std::size_t n) noexcept;
public:
    typename std::array<T, N>::iterator push_front() noexcept;
    typename std::array<T, N>::iterator pop_front() noexcept;

    typename std::array<T, N>::iterator push_back() noexcept;
    typename std::array<T, N>::iterator pop_back() noexcept;

    typename std::array<T, N>::iterator frontit() noexcept { return std::next(array.begin(), front_index); };
    typename std::array<T, N>::iterator backit() noexcept { return std::next(array.begin(), back_index); };

    T& front() noexcept { return array[front_index]; }
    const T& front() const noexcept { return array[front_index]; }

    T& back() noexcept { return array[back_index]; }
    const T& back() const noexcept { return array[back_index]; }

    bool valid(typename std::array<T, N>::iterator it) const noexcept { return it != array.end(); }
    bool empty() const noexcept;
    bool full() const noexcept;
    void reset() noexcept;
};

template<typename T>
class spin_locked_resource {
    std::atomic_bool lock_; 
    T obj;
    
    class locked_resource {
        spin_locked_resource& lock;
        T& obj;
    public:
        locked_resource(spin_locked_resource& lock, T& obj);
        locked_resource(const locked_resource& ) = delete;
        locked_resource(locked_resource&& ) = delete;
        ~locked_resource();

        T* operator->() noexcept { return &obj; }
        T& operator*() noexcept { return obj; }

        auto operator=(const locked_resource& ) = delete;
    };

    void unlock() noexcept;
public:
    template<typename ... Args>
    spin_locked_resource(Args&&... args);

    locked_resource lock() noexcept;
};

template<typename T>
template<typename ... Args>
shared_mem_ptr<T>::shared_mem_ptr(const char* path, Args&&... args)
    : path(path){

    memfd = shm_open(path, O_RDWR, 0666);
    if (memfd < 0) 
        memfd = shm_open(path, O_RDWR | O_CREAT, 0666);

    if (memfd < 0)
        throw BackendException();

    struct stat st;
    if (fstat(memfd, &st) < 0)
        throw BackendException();

    if (st.st_size != sizeof(ref)) {
        if (ftruncate(memfd, sizeof(ref)) < 0)
            throw BackendException();
    }

    ptr = (ref*)mmap(NULL, sizeof(ref), PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0); 

    if (ptr->use_count == 0) {
        ptr->use_count = 1;
        new (&ptr->obj) T(std::forward<Args>(args)... );
    } else {
        ptr->use_count++;
    }
}

template<typename T>
shared_mem_ptr<T>::~shared_mem_ptr() {
    if (!ptr) 
        return;

    ptr->use_count--;
    ::close(memfd);

    if (ptr->use_count == 0) {
        shm_unlink(path);
    }
}

template<typename T>
shared_mem_ptr<T>::shared_mem_ptr(const shared_mem_ptr<T>& other) {
    this->memfd = other.memfd;
    this->ptr = other->ptr;
    this->path = other.path;

    ptr->use_count++;
}

template<typename T>
shared_mem_ptr<T>::shared_mem_ptr(shared_mem_ptr<T>&& other) {
    this->memfd = other.memfd;
    this->ptr = other->ptr;
    this->path = other.path;

    other.ptr = nullptr;
}

template<typename T, std::size_t N>
typename std::array<T, N>::iterator fast_deque<T, N>::push_front() noexcept {
    if (full())
        return array.end();

    if (empty()) {
        front_index = 0;
        back_index = 0;
    } else {
        front_index = prev(front_index);
    }

    return frontit();
}

template<typename T, std::size_t N>
typename std::array<T, N>::iterator fast_deque<T, N>::push_back() noexcept {
    if (full())
        return array.end();

    if (empty()) {
        front_index = 0;
        back_index = 0;
    } else {
        back_index = next(back_index);
    }

    return backit();
}

template<typename T, std::size_t N>
typename std::array<T, N>::iterator fast_deque<T, N>::pop_front() noexcept {
    if (empty())
        return array.end();

    const auto current_index = front_index;
    if (front_index == back_index)
        reset();
    else
        front_index = next(front_index);

    return std::next(array.begin(), current_index);
}

template<typename T, std::size_t N>
typename std::array<T, N>::iterator fast_deque<T, N>::pop_back() noexcept {
    if (empty())
        return array.end();

    const auto current_index = back_index;
    if (front_index == back_index)
        reset();
    else
        back_index = prev(back_index);

    return std::next(array.begin(), current_index);
}

template<typename T, std::size_t N>
bool fast_deque<T, N>::empty() const noexcept {
    return front_index == invalid_index;
}

template<typename T, std::size_t N>
bool fast_deque<T, N>::full() const noexcept {
    return (front_index == 0 && back_index == N - 1) || (front_index == back_index + 1);
}

template<typename T, std::size_t N>
std::size_t fast_deque<T, N>::next(std::size_t n) noexcept {
    return (n + 1) % N;
}

template<typename T, std::size_t N>
std::size_t fast_deque<T, N>::prev(std::size_t n) noexcept {
    return n == 0 ? N - 1 : n - 1;
}

template<typename T, std::size_t N>
void fast_deque<T, N>::reset() noexcept {
    front_index = invalid_index;
    back_index = invalid_index;
}

template<typename T>
template<typename ... Args>
spin_locked_resource<T>::spin_locked_resource(Args&&... args) 
    : obj(std::forward<Args>(args)... ), lock_(false) {}

template<typename T>
typename spin_locked_resource<T>::locked_resource spin_locked_resource<T>::lock() noexcept {
    while (lock_.exchange(true, std::memory_order_acquire))
        ;

    return locked_resource(*this, obj);
}

template<typename T>
void spin_locked_resource<T>::unlock() noexcept {
    lock_.store(false, std::memory_order_release);
}

template<typename T>
spin_locked_resource<T>::locked_resource::locked_resource(spin_locked_resource<T>& lock, T& obj)
    : lock(lock), obj(obj) {}

template<typename T>
spin_locked_resource<T>::locked_resource::~locked_resource() {
    lock.unlock();
}

#endif
