#include "ring_buffer.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

template <typename T>
RingBuffer<T>::RingBuffer()
{
    buffer = std::vector<T>();
    front = 0;
}

template <typename T>
RingBuffer<T>::~RingBuffer()
{
}

template <typename T>
void RingBuffer<T>::init(String k)
{
    buffer = Array();
    front = 0;
    tag = k;
}

template <typename T>
void RingBuffer<T>::push_back(T item)
{
    buffer.push_back(item);
}

template <typename T>
void RingBuffer<T>::emplace(T&& item)
{
    buffer.emplace(item);
}

template <typename T>
T RingBuffer<T>::pop_back()
{
    auto item = buffer.pop_back();
    if (is_empty()) {
        reset();
    }
    return item;
}

template <typename T>
T RingBuffer<T>::pop_front()
{
    if (is_empty()) {
        return nullptr;
    }
    auto item = buffer[front];
    front += 1;
    if (is_empty()) {
        reset();
    }
    return item;
}

template <typename T>
bool RingBuffer<T>::is_empty()
{
    return front == buffer.size();
}

template <typename T>
size_t RingBuffer<T>::size()
{
    return buffer.size() - front;
}

template <typename T>
void RingBuffer<T>::reset()
{
    front = 0;
    buffer.clear();
}