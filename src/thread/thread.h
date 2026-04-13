#pragma once
#include <stdlib.h>

// Common function signature for thread workers
typedef void (*thread_fn_t)(void *);

#ifdef _WIN32
#include <windows.h>
typedef HANDLE thread_t;

// Trampoline context — bridges void(*)(void*) to DWORD WINAPI (*)(LPVOID)
typedef struct {
  thread_fn_t fn;
  void *arg;
} threads_ctx_t;

static inline DWORD WINAPI threads_trampoline(LPVOID ctx) {
  threads_ctx_t *c = (threads_ctx_t *)ctx;
  c->fn(c->arg);
  free(c);
  return 0;
}

static inline int thread_create(thread_t *t, thread_fn_t fn, void *arg) {
  threads_ctx_t *c = (threads_ctx_t *)malloc(sizeof(threads_ctx_t));
  if (!c) return -1;
  c->fn = fn;
  c->arg = arg;
  *t = CreateThread(NULL, 0, threads_trampoline, (LPVOID)c, 0, NULL);
  if (!*t) {
    free(c);
    return -1;
  }
  return 0;
}

static inline int thread_join(thread_t t) {
  WaitForSingleObject(t, INFINITE);
  CloseHandle(t);
  return 0;
}

#else
#include <pthread.h>
typedef pthread_t thread_t;

// Trampoline context — bridges void(*)(void*) to void *(*)(void *)
typedef struct {
  thread_fn_t fn;
  void *arg;
} threads_ctx_t;

static inline void *threads_trampoline(void *ctx) {
  threads_ctx_t *c = (threads_ctx_t *)ctx;
  c->fn(c->arg);
  free(c);
  return NULL;
}

static inline int thread_create(thread_t *t, thread_fn_t fn, void *arg) {
  threads_ctx_t *c = (threads_ctx_t *)malloc(sizeof(threads_ctx_t));
  if (!c) return -1;
  c->fn = fn;
  c->arg = arg;
  int r = pthread_create(t, NULL, threads_trampoline, (void *)c);
  if (r) free(c);
  return r;
}

static inline int thread_join(thread_t t) { return pthread_join(t, NULL); }

#endif
