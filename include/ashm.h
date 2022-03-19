#include <sys/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

int shm_init(); //Used only by parent process
int shm_open(const char *name, int oflag, mode_t mode);
int shm_unlink(const char *name);
void* shm_mmap(void *addr, size_t length, int prot, int flags,
               int fd, off_t offset);
extern int shm_ftruncate(int fd, off_t length);
extern int shm_close(int fd);
int reset_cloexec(int fd);

#ifdef __cplusplus
}

#endif
