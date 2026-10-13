#ifndef PERFORMANCEQUERY_H
#define PERFORMANCEQUERY_H

#include "SDL3/SDL.h"

typedef struct PerformanceQuery {
    Uint64 perfCounter;
    Uint64 perfFrequency;
} PerformanceQuery;

static inline void PerformanceQueryBegin(PerformanceQuery* query) {
    query->perfCounter = SDL_GetPerformanceCounter();
    query->perfFrequency = SDL_GetPerformanceFrequency();
}

static inline double PerformanceQueryEnd(PerformanceQuery* query) {
    Uint64 perfCounterNow = SDL_GetPerformanceCounter();
    return (double)(perfCounterNow - query->perfCounter) / (double)query->perfFrequency;
}

#endif