#ifndef TASKPOOL_H
#define TASKPOOL_H

#include "util.h"

typedef void* TaskPoolHandle;
typedef void* TaskHandle;

TaskPoolHandle taskpool_create(int initcount);
BOOL taskpool_request(TaskPoolHandle handle, void(*fn)(void*), void* arg);
#endif