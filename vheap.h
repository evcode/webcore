#ifndef VHEAP_H
#define VHEAP_H

#include "pthread.h"

typedef struct
{
	int vheap_id;
	char vheap_name[16];
	void* vheap_addr;
	int vheap_volume;

	int vheap_blocknum; // the mem block allocated

	pthread_mutex_t vheap_mutex;
} vheap_t;

int vheap_init(int totalsize);
void* vheap_malloc(int id, int size);
void* vheap_realloc(int id, void* src, int size);
void vheap_free(int id, void*);
void vheap_dump(int id);
#endif