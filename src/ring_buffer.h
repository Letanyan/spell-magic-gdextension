#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <godot_cpp/classes/ref_counted.hpp>
#include <vector>

namespace godot {

template <typename T>
class RingBuffer {
public:
    std::vector<T> buffer;
    int64_t front;
    String tag;

    RingBuffer();
    ~RingBuffer();

    void init(String k);
    void push_back(T item);
    void emplace(T&& item);
    T pop_back();
    T pop_front();
    bool is_empty();
    size_t size();
    void reset();
};

}

#endif