#ifndef _SHM_H
#define _SHM_H

#include <sys/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

extern int shm_init(); //Used only by parent process
#ifndef _SYS_MMAN_H
#ifndef __ANDROID__
extern int shm_open(const char *name, int oflag, mode_t mode);
extern int shm_unlink(const char *name);
#endif
#endif
#ifdef __ANDROID__
extern int shm_open(const char *name, int oflag, mode_t mode);
extern int shm_unlink(const char *name);
#endif

extern void* shm_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);
extern int shm_ftruncate(int fd, off_t length);
extern int shm_close(int fd);

#endif

#ifdef __cplusplus
}

#endif