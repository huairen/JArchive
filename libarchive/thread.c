#include "thread.h"

void create_lock( struct archive *arc )
{
	arc->thread_lock = CreateMutex(NULL, FALSE, NULL);
}

void destroy_lock( struct archive *arc )
{
	if(arc->thread_lock)
	{
		CloseHandle(arc->thread_lock);
		arc->thread_lock = NULL;
	}
}

void lock( struct archive *arc )
{
	if(arc->thread_lock)
		WaitForSingleObject(arc->thread_lock, INFINITE);
}

void unlock( struct archive *arc )
{
	if(arc->thread_lock)
		ReleaseMutex(arc->thread_lock);
}
