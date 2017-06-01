#include <assert.h>

#include "vheap.h"
#include "util.h"

#define VHEAP_MAX_NUM 10
static vheap_t vheaplist[VHEAP_MAX_NUM];

static vheap_t* _get_vheap(int id)
{
	// TOOD: fix me
	return NULL;
}

int vheap_init(int totalsize)
{
	// TOOD: fix me
	memset(vheaplist, 0, sizeof(vheaplist));

	int i;
	for (i=0;i<VHEAP_MAX_NUM;i++)
	{
		pthread_mutex_init(&vheaplist[i].vheap_mutex, NULL);
		vheaplist[i].vheap_blocknum = 0;
	}
}

void* vheap_malloc(int id, int size)
{
#if 1
	assert(size>0);

	void* mem = malloc(size);
	assert(mem);

	pthread_mutex_lock(&vheaplist[id].vheap_mutex);
	vheaplist[id].vheap_blocknum ++;
	pthread_mutex_unlock(&vheaplist[id].vheap_mutex);

	return mem;
#else
	// TOOD: fix me
	vheap_t* heap = _get_vheap(id);
	if (heap == NULL)
	{
		error("Failed to get vheap id %d\n", id);
	}

	pthread_mutex_lock(&heap->vheap_mutex);

	pthread_mutex_unlock(&heap->vheap_mutex);
#endif
}

void* vheap_realloc(int id, void* src, int size)
{
	// TOOD: fix me
	void* dst = realloc(src, size);
	// NOTE: "realloc" should not operate "vheap_blocknum" since it's just a replacement

	return dst;
}

void vheap_free(int id, void* addr)
{
#if 1
	//assert(addr);
	if (addr == NULL)
	{
		debug("Warning: a NULL vheap to release");
		return;
	}

	free(addr);
	addr = NULL;

	pthread_mutex_lock(&vheaplist[id].vheap_mutex);
	assert(vheaplist[id].vheap_blocknum > 0);
	vheaplist[id].vheap_blocknum --;
	pthread_mutex_unlock(&vheaplist[id].vheap_mutex);
#else
	// TOOD: fix me
	vheap_t* heap = _get_vheap(id);
	if (heap == NULL)
	{
		error("Failed to get vheap id %d\n", id);
	}

	pthread_mutex_lock(&heap->vheap_mutex);

	pthread_mutex_unlock(&heap->vheap_mutex);
#endif
}

void vheap_dump(int id)
{
	printf("VHEAP ID       Remaining blocks\n");
	int i;
	for (i=0;i<VHEAP_MAX_NUM;i++)
	{
		pthread_mutex_lock(&vheaplist[i].vheap_mutex);
		if (vheaplist[i].vheap_blocknum != 0)
			printf("%8d               %8d\n", i, vheaplist[i].vheap_blocknum);
		pthread_mutex_unlock(&vheaplist[i].vheap_mutex);
	}

}