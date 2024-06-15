#include "my_profiler.h"

#include <godot_cpp\classes\time.hpp>

GDProfiler::GDProfiler()
{
    elapsed = 0.0;
    start_time = godot::Time::get_singleton()->get_ticks_usec();
}
GDProfiler::~GDProfiler() { }

void GDProfiler::start()
{
    start_time = godot::Time::get_singleton()->get_ticks_usec();
}

double GDProfiler::reset()
{
    auto result = (godot::Time::get_singleton()->get_ticks_usec() - start_time) * 1e-6;
    start_time = godot::Time::get_singleton()->get_ticks_usec();
    elapsed = 0.0;
    return result;
}

double GDProfiler::lap()
{
    elapsed += (godot::Time::get_singleton()->get_ticks_usec() - start_time) * 1e-6;
    start_time = godot::Time::get_singleton()->get_ticks_usec();
    return elapsed;
}

double GDProfiler::stop()
{
    elapsed = (godot::Time::get_singleton()->get_ticks_usec() - start_time) * 1e-6;
    return elapsed;
}