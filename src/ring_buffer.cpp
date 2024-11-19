#include "ring_buffer.h"
#include <godot_cpp/variant/utility_functions.hpp>

using namespace godot;

GDRingBuffer::GDRingBuffer()
{
    buffer = Array();
    front = 0;
}

GDRingBuffer::~GDRingBuffer()
{
}

void GDRingBuffer::init(String k)
{
    buffer = Array();
    front = 0;
    tag = k;
}

void GDRingBuffer::append(Variant item)
{
    buffer.append(item);
}

Variant GDRingBuffer::pop_back()
{
    auto item = (Variant)buffer.pop_back();
    if (is_empty()) {
        reset();
    }
    return item;
}

Variant GDRingBuffer::pop_front()
{
    if (is_empty()) {
        return (Variant) nullptr;
    }
    auto item = buffer[front];
    front += 1;
    if (is_empty()) {
        reset();
    }
    return item;
}

bool GDRingBuffer::is_empty()
{
    return front == buffer.size();
}

size_t GDRingBuffer::size()
{
    return buffer.size() - front;
}

void GDRingBuffer::reset()
{
    front = 0;
    buffer.clear();
}