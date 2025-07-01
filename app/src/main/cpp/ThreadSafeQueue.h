//
// Created by MOSI000211 on 2025/6/27.
//

#ifndef CAMERAPREVIEW_THREADSAFEQUEUE_H
#define CAMERAPREVIEW_THREADSAFEQUEUE_H

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <stdexcept>

template<typename T>
class ThreadSafeQueue {
private:
    std::deque<T> queue;
    mutable std::mutex mtx;
    std::condition_variable not_empty;
    std::condition_variable not_full;
    size_t max_size;

public:
    // 构造函数，初始化最大容量
    explicit ThreadSafeQueue(size_t max_size = 10) : max_size(max_size) {}

    // 入队
    void push(const T& value) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this]{ return queue.size() < max_size; });
        queue.push_back(value);
        lock.unlock();
        not_empty.notify_one();
    }

    // 出队
    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this]{ return !queue.empty(); });
        T value = queue.front();
        queue.pop_front();
        lock.unlock();
        not_full.notify_one();
        return value;
    }

    // 尝试出队，不阻塞
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mtx);
        if (queue.empty()) return false;
        value = queue.front();
        queue.pop_front();
        not_full.notify_one();
        return true;
    }

    // 队列是否为空
    bool empty() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty();
    }

    // 队列是否已满
    bool full() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size() == max_size;
    }

    // 获取队列大小
    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.size();
    }
};

#endif //CAMERAPREVIEW_THREADSAFEQUEUE_H
