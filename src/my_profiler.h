#ifndef GDPROFILER_H
#define GDPROFILER_H

class GDProfiler {
public:
    double start_time;
    double elapsed;
    GDProfiler();
    ~GDProfiler();

    void start();
    void reset();
    double lap();
    double stop();
};

#endif