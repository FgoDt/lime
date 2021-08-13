#ifndef __LIME_MEM_H__
#define __LIME_MEM_H__
#include "config.h"

void *lime_malloc(size_t size);

void *lime_mallocz(size_t size);

void lime_free(void *data);
#endif