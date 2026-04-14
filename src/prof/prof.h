#pragma once

#ifndef NDEBUG
typedef unsigned long segment_t;
segment_t prof_segstart(const char *label);
void prof_segend(segment_t id);
void prof_print(void);

#define PROF_CAT(a, b) a##b
#define PROF_SEGNAME(line) _PROF_CAT(segment, line)

#define prof_segment(Name, Block)                                              \
  do {                                                                         \
    segment_t _PROF_SEGNAME(__LINE__) = prof_segstart(Name);                   \
    Block;                                                                     \
    prof_segend(_PROF_SEGNAME(__LINE__));                                      \
  } while (0)
#define xprof_segment(Name, Block)                                             \
  do {                                                                         \
    Block;                                                                     \
  } while (0)

#else

#define prof_segstart(...)
#define prof_segend(...)
#define prof_print(...)
#define prof_segment(Name, Block)                                              \
  do {                                                                         \
    Block;                                                                     \
  } while (0)
#define xprof_segment(Name, Block)                                             \
  do {                                                                         \
    Block;                                                                     \
  } while (0)

#endif
