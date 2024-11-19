#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <godot_cpp/classes/ref_counted.hpp>

namespace godot {

class GDRingBuffer : public RefCounted {
    GDCLASS(GDRingBuffer, RefCounted)

public:
    Array buffer;
    int64_t front;
    String tag;

    GDRingBuffer();
    ~GDRingBuffer();

    void init(String k);
    void append(Variant item);
    Variant pop_back();
    Variant pop_front();
    bool is_empty();
    size_t size();
    void reset();
};

}

#endif