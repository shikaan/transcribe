#pragma once

#ifndef NDEBUG
typedef unsigned long segment_t;
segment_t prof_segstart(const char *label);
void prof_segend(segment_t seg);
void prof_print(void);

#else

#define prof_segstart(...)
#define prof_segend(...)
#define prof_print(...)

#endif
