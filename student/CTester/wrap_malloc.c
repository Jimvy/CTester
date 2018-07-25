/* 
 * Wrapper for malloc, free and calloc
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <stdlib.h>

#include  "wrap.h"

#include <libintl.h> 
#include <locale.h> 
#define _(STRING) gettext(STRING)

#define MSG_SIZE 1000
char msg[MSG_SIZE];


void * __real_malloc(size_t s);
void * __real_calloc(size_t nmemb, size_t s);
void __real_free(void *);
void * __real_realloc(void *ptr, size_t size);


extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;
extern struct wrap_log_t logs;

/**
 * Adds the (ptr, size) pair to the list of regions allocated by malloc.
 * Keeps only MAX_LOG in memory.
 * If ptr is NULL, it is not added to the logs.
 * Currently, returns -1 if there is no more space in the logs.
 */
ssize_t add_malloc(void *ptr, size_t size) {
  // FIXME add the possibility to record more than MAX_LOG pairs
  if(ptr!=NULL && logs.malloc.n < MAX_LOG) {
    logs.malloc.log[logs.malloc.n].size=size;
    logs.malloc.log[logs.malloc.n].ptr=ptr;
    logs.malloc.n++;
    return size;
  } else if (ptr == NULL) {
    return 0;
  } else {
    return -1;
  }
}

/**
 * Removes the (ptr, size) pair of the list of regions allocated by malloc.
 * Returns the size of the malloced region,
 * or -1 if ptr was not allocated by malloc/realloc.
 * 0 can be returned if ptr is NULL or if ptr was allocated with zero size.
 */
ssize_t remove_malloc(void *ptr) {
  if (ptr == NULL) {
    return 0;
  }
  for (int i = 0; i < MAX_LOG; i++) {
    if (logs.malloc.log[i].ptr == ptr) {
      int size = logs.malloc.log[i].size;
      logs.malloc.log[i].size = -1;
      logs.malloc.log[i].ptr = NULL;
      return size;
    }
  }
  return -1;
}

/**
 * Removes the old (old_ptr, oldsize) pair from the malloc logs,
 * and adds the new (new_ptr, newsize) pair to the logs.
 * Returns 0 if everything is ok, -1 if old_ptr is an invalid pointer,
 * -2 if there is no more space in the logs.
 * If it returns 0, stores the memory usage increase between the two in *delta.
 */
int update_realloc_block(void *old_ptr, void *new_ptr, size_t newsize, int *delta) {
  int oldsize = remove_malloc(old_ptr); // also works if ptr is NULL
  if (oldsize == -1)
    return -1;
  int rep = add_malloc(new_ptr, newsize);
  if (rep == -1) {
    return -2;
  }
  *delta = (newsize - oldsize);
  return 0;
}

size_t find_size_malloc(void *ptr) {
  for(int i=0;i<MAX_LOG;i++) {
    if(logs.malloc.log[i].ptr==ptr) 
      return logs.malloc.log[i].size;
  }
  return -1;
}


void * __wrap_malloc(size_t size) {
  if(!wrap_monitoring || !monitored.malloc) {
    return __real_malloc(size);
  }
  stats.malloc.called++;
  stats.malloc.last_params.size=size;
  if(FAIL(failures.malloc)) {
    failures.malloc=NEXT(failures.malloc);
    return failures.malloc_ret;
  }
  failures.malloc=NEXT(failures.malloc);    
  void *ptr=__real_malloc(size);
  stats.memory.used += size;
  stats.malloc.last_return=ptr;
  add_malloc(ptr,size);
  return ptr;
}

void * __wrap_calloc(size_t nmemb, size_t size) {
  if(!wrap_monitoring || !monitored.calloc) {
    return __real_calloc(nmemb, size);
  }
  stats.calloc.called++;
  stats.calloc.last_params.size=size;
  stats.calloc.last_params.nmemb=nmemb;

  if(FAIL(failures.calloc)) {
    failures.calloc=NEXT(failures.calloc);
    return failures.calloc_ret;
  }
  failures.calloc=NEXT(failures.calloc);

  void *ptr=__real_calloc(nmemb,size);
  stats.memory.used+=nmemb*size;
  stats.calloc.last_return=ptr;
  add_malloc(ptr,nmemb*size);
  return ptr;
}

void * __wrap_realloc(void *ptr, size_t size) {
  if(!wrap_monitoring || !monitored.realloc) {
    return __real_realloc(ptr, size);
  }
  stats.realloc.called++;
  stats.realloc.last_params.size=size;
  if(FAIL(failures.realloc)) {
    failures.realloc=NEXT(failures.realloc);
    return failures.realloc_ret;
  }
  failures.realloc=NEXT(failures.realloc);
  void *r_ptr=__real_realloc(ptr,size);
  stats.realloc.last_return=r_ptr;
  int delta = 0;
  int rep = update_realloc_block(ptr, r_ptr, size, &delta);
  if (rep == -1) {
    // there was an error ; FIXME
  } else {
    stats.memory.used += delta;
  }
  return r_ptr;
}

void __wrap_free(void *ptr) {
  if(!wrap_monitoring || !monitored.free) {
    return __real_free(ptr);
  }
  stats.free.called++;
  stats.free.last_params.ptr=ptr;
  if(ptr!=NULL) {
    int delta = remove_malloc(ptr);
    if (delta == -1) {
      // This is not a valid pointer: double free for example
    }
    stats.memory.used -= delta;

    if (FAIL(failures.free))
      failures.free=NEXT(failures.free);
    else // FIXME ne faudrait-il pas faire NEXT(failures.free) dans l'autre cas aussi ?
      __real_free(ptr);
  }
}


int  malloc_allocated() {
  int tot=0;
  for(int i=0;i<MAX_LOG;i++) {
    if(logs.malloc.log[i].ptr!=NULL) {
      tot+=(int) logs.malloc.log[i].size;
    }
  }
  return tot;
}

/*
 * returns true if the address has been managed by malloc, false
 * otherwise (also false if address has been freed)
 */
int malloced(void *addr) {
  for(int i=0;i<logs.malloc.n;i++) {
    if(logs.malloc.log[i].ptr<=addr && 
       (logs.malloc.log[i].ptr+ logs.malloc.log[i].size)>=addr) {
      return true;
    }
  }
  return false;

}
