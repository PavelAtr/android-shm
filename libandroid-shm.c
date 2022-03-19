#include <stdio.h>
#include <stdlib.h>
#include <ashm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cutils/ashmem.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <error.h>
#include <errno.h>
#ifdef __ANDROID__
#include <android/sharedmem.h>
//#include <android/sharedmem_jni.h>
#endif

#define MAX_NAME_LENGTH 128
#define MAX_SHM_FILES 100
#define REGISTRY_ENV "SHM_REG_FD"

void* __real_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);

typedef struct {
    char name[MAX_NAME_LENGTH];
    int fd;
    short nlink;
} finfo;

__attribute__((visibility("hidden"))) int save_registry_fd(int regfd)
{
    char value[10];
    sprintf(value, "%d", regfd);
    return setenv(REGISTRY_ENV, value, 1);
}

__attribute__((visibility("hidden"))) int load_registry_fd()
{
    char* value = getenv(REGISTRY_ENV);
    if (value == NULL)
	return -1;
    return atoi(value);
}

__attribute__((visibility("hidden"))) finfo* load_registry()
{
    int regfd = load_registry_fd();
    if (regfd == -1) return NULL;

    finfo* registry = (finfo*) __real_mmap(NULL, sizeof(finfo)*MAX_SHM_FILES,
	PROT_READ|PROT_WRITE, MAP_SHARED, regfd, 0);
    if (registry == MAP_FAILED)
	return NULL;

    return registry;
}

__attribute__((visibility("hidden"))) finfo* find_empty(finfo* registry)
{
    for (int i = 0; i < MAX_SHM_FILES; i++)
    {
	if (registry[i].nlink <= 0)
	{
	    registry[i].nlink = 0;
	    return &registry[i];
	}
    }

    return NULL;
}

__attribute__((visibility("hidden"))) finfo* find_byname(finfo* registry, const char* fname)
{
    for (int i = 0; i < MAX_SHM_FILES; i++)
    {
	if (strcmp(fname, registry[i].name) == 0)
	    return &registry[i];
    }

    return NULL;
}

int reset_cloexec(int fd)
{

    int oldflags = fcntl(fd, F_GETFD);
    if (oldflags == -1)
	return -1;
    oldflags &= ~FD_CLOEXEC;
    return fcntl (fd, F_SETFD, oldflags);
}

int shm_init()
{
#ifdef __ANDROID__
    int regfd = ASharedMemory_create("registry", sizeof(finfo)*MAX_SHM_FILES);
#else
    int regfd = ashmem_create_region("registry", sizeof(finfo)*MAX_SHM_FILES);
#endif
    if (regfd < 0)
	return -1;
    if (reset_cloexec(regfd) < 0)
	return -1;

    finfo* registry = (finfo*) __real_mmap(NULL, sizeof(finfo)*MAX_SHM_FILES,
	PROT_READ|PROT_WRITE, MAP_SHARED, regfd, 0);
    if (registry == MAP_FAILED)
	return -1;

    for (int i = 0; i < MAX_SHM_FILES; i++)
    {
	char* ashmemname = malloc(sizeof(char) * 8);
	sprintf(ashmemname, "file%d", i);
	strcpy(registry[i].name, "");
#ifdef __ANDROID__
	registry[i].fd = ASharedMemory_create(ashmemname, 10);
#else
	registry[i].fd = ashmem_create_region(ashmemname, 10);
#endif
	if (registry[i].fd < 0) return -1;
	if (reset_cloexec(registry[i].fd) < 0)
	    return -1;
	registry[i].nlink = 0;
    }

    if (save_registry_fd(regfd) != 0) return -1;
    printf("Initialized shm registry fd=%d, %d files\n", regfd,
        MAX_SHM_FILES);

    return 0;
}

int shm_open(const char *name, int oflag, mode_t mode)
{
    finfo* registry = load_registry();
    if (registry == NULL) return -1;

    finfo* file = find_byname(registry, name);
    if (file != NULL) 
    {
	if (oflag & O_CREAT)
	    return -1;
	if (oflag & O_TRUNC)
	    ashmem_resize_region(file->fd, 0);
	file->nlink++;
	printf("opening file \"%s\" fd=%d nlink=%u\n", file->name, file->fd, file->nlink);
	return file->fd;
    } else
    {
	if (oflag & O_CREAT)
	{
	    file = find_empty(registry);
	    if (file == NULL) return -1;
	    if (strlen(name) > MAX_NAME_LENGTH) return -1;
	    strcpy(file->name, name);
	    file->nlink++;
	    printf("creating file \"%s\" fd=%d nlink=%u\n", file->name, file->fd, file->nlink);
	    return file->fd;
	}
	else
	    return -1;
    }

    return -1;
}

int shm_unlink(const char *name)
{
    finfo* registry = load_registry();
    if (registry == NULL) return -1;
    finfo* file = find_byname(registry, name);
    if (file == NULL) return -1;
    file->nlink--;
    if (file->nlink <= 0)
	ashmem_resize_region(file->fd, 0);
    printf("unlink file \"%s\" fd=%d nlink=%u\n", file->name, file->fd, file->nlink);

    return 0;
}

void* shm_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset)
{
    if ((fd >=4) && (fd <= MAX_SHM_FILES + 4)) // Magic numbers :)
	if (ashmem_get_size_region(fd) < length)
	    ashmem_resize_region(fd, length);
#ifdef SYS_mmap2
    return (void*) syscall(SYS_mmap2, addr, length, prot, flags, fd, (size_t) offset >> 12);
#else
    return (void*) syscall(SYS_mmap, addr, length, prot, flags, fd, offset);
#endif
}

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset) __attribute__((alias("shm_mmap")));

int shm_ftruncate(int fd, off_t length)
{
    if ((fd >=4) && (fd <= MAX_SHM_FILES + 4))
    {
        ashmem_resize_region(fd, length);
	return 0;
    } else
	return (int) syscall(SYS_ftruncate, fd, length);
}

int ftruncate(int fd, off_t length) __attribute__((alias("shm_ftruncate")));

int shm_close(int fd)
{
    if ((fd >=4) && (fd <= MAX_SHM_FILES + 4))
        return 0;
    else
	return (int) syscall(SYS_close, fd);
}

int close(int fd) __attribute__((alias("shm_close")));
