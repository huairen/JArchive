#ifndef _THREAD_H_
#define _THREAD_H_

#include "archive_type.h"

void create_lock( struct archive *arc );
void destroy_lock( struct archive *arc );

void lock( struct archive *arc );
void unlock( struct archive *arc );

#endif                                                  