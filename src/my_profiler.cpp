#include "my_profiler.h"

#include <godot_cpp\classes\time.hpp>

GDProfiler::GDProfiler()
{
    elapsed = 0.0;
    start_time = godot::Time::get_singleton()->get_unix_time_from_system();
}
GDProfiler::~GDProfiler() { }

void GDProfiler::start()
{
    start_time = godot::Time::get_singleton()->get_unix_time_from_system();
}

void GDProfiler::reset()
{
    start_time = godot::Time::get_singleton()->get_unix_time_from_system();
    elapsed = 0.0;
}

double GDProfiler::lap()
{
    elapsed += godot::Time::get_singleton()->get_unix_time_from_system() - start_time;
    start_time = godot::Time::get_singleton()->get_unix_time_from_system();
    return elapsed;
}

double GDProfiler::stop()
{
    elapsed = godot::Time::get_singleton()->get_unix_time_from_system() - start_time;
    return elapsed;
}