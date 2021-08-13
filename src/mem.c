#include "mem.h"

static int64_t mem_count = 0;
void *lime_malloc(size_t size)
{
	mem_count++;
	void *data = malloc(size);
	return data;
}

void *lime_mallocz(size_t size)
{
	mem_count++;
	void *data = malloc(size);
	memset(data, 0, size);
	return data;
}

void lime_free(void *data)
{
	mem_count--;
	free(data);
}