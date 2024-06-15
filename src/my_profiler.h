#ifndef GDPROFILER_H
#define GDPROFILER_H

#include <stdint.h>

class GDProfiler {
public:
    uint64_t start_time;
    double elapsed;
    GDProfiler();
    ~GDProfiler();

    void start();
    double reset();
    double lap();
    double stop();
};

#endif