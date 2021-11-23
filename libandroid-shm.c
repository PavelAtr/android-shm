#define _GNU_SOURCE
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
#include <dlfcn.h>

#define MAX_NAME_LENGTH 128
#define MAX_SHM_FILES 100
#define REGISTRY_FILE "/run/shmregistryfd"

static int max_shm_files = 0;

void* __real_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset);

typedef struct {
    char name[MAX_NAME_LENGTH];
    int fd;
    short nlink;
} finfo;

__attribute__((visibility("hidden"))) int save_registry_fd(int regfd)
{
/*    int fd = open(REGISTRY_FILE, O_CREAT|O_TRUNC|O_RDWR);
    if (fd == -1) return -1;
    dprintf(fd, "%d\n", regfd);
    close(fd);*/

    return 0;
}

__attribute__((visibility("hidden"))) int load_registry_fd()
{
/*    int regfd = -1;
    FILE* f = fopen(REGISTRY_FILE, "r");
    if (f == NULL) return -1;
    fscanf(f, "%d", &regfd);
    fclose(f);

    return regfd;*/
    return 3; // Magic numbers :)
}

__attribute__((visibility("hidden"))) finfo* load_registry()
{
    int regfd = load_registry_fd();
    if (regfd == -1) return NULL;

    finfo* registry = (finfo*) __real_mmap(NULL, sizeof(finfo)*max_shm_files,
	PROT_READ|PROT_WRITE, MAP_SHARED, regfd, 0);
    if (registry == MAP_FAILED) 
	return NULL;

    return registry;
}

__attribute__((visibility("hidden"))) finfo* find_empty(finfo* registry)
{
    for (int i = 0; i < max_shm_files; i++)
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
    for (int i = 0; i < max_shm_files; i++)
    {
	if (strcmp(fname, registry[i].name) == 0)
	    return &registry[i];
    }

    return NULL;
}

int maxfiles_init()
{
    char* mfiles;

    if (max_shm_files == 0)
	if ((mfiles = getenv("MAX_SHM_FILES")) != NULL)
	max_shm_files = atoi(mfiles);

    if (max_shm_files == 0)
	max_shm_files = MAX_SHM_FILES;

    return 0;
}


int shm_init()
{
    maxfiles_init();

    int regfd = ashmem_create_region("registry", sizeof(finfo)*max_shm_files);
    if (regfd == -1) return -1;
    finfo* registry = (finfo*) __real_mmap(NULL, sizeof(finfo)*max_shm_files,
	PROT_READ|PROT_WRITE, MAP_SHARED, regfd, 0);
    if (registry == MAP_FAILED) return -1;

    for (int i = 0; i < max_shm_files; i++)
    {
	char* ashmemname = malloc(sizeof(char) * 8);
	sprintf(ashmemname, "file%d", i);
	strcpy(registry[i].name, "");
	registry[i].fd = ashmem_create_region(ashmemname, 0);
	if (registry[i].fd == -1) return -1;
	registry[i].nlink = 0;
    }

    if (save_registry_fd(regfd) != 0) return -1;
    printf("Initialized shm registry fd=%d, %d files\n", load_registry_fd(), max_shm_files);

    return 0;
}

int shm_open(const char *name, int oflag, mode_t mode)
{
    maxfiles_init();

    finfo* registry = load_registry();
    if (registry == NULL) return -1;

    finfo* file = find_byname(registry, name);
    if (file != NULL) 
    {
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
	return -1;
    }

    return -1;
}

int shm_unlink(const char *name)
{
    maxfiles_init();

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

static void* (*mmap_orig)(void*,size_t, int, int, int, off_t);
static int (*ftruncate_orig)(int, off_t);
static int (*close_orig)(int fd);

void* shm_mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset)
{
    maxfiles_init();

    if ((fd >=4) && (fd <= max_shm_files + 4)) // Magic numbers :)
	if (ashmem_get_size_region(fd) < length)
	    ashmem_resize_region(fd, length);

    if(!mmap_orig)
	mmap_orig = dlsym(RTLD_NEXT, "mmap");

    return mmap_orig(addr, length, prot, flags, fd, offset);
}

void* mmap(void *addr, size_t length, int prot, int flags,
                  int fd, off_t offset) __attribute__((alias("shm_mmap")));

int shm_ftruncate(int fd, off_t length)
{
    maxfiles_init();

    if ((fd >=4) && (fd <= max_shm_files + 4))
    {
        ashmem_resize_region(fd, length);
	return 0;
    } else
    {
	if(!ftruncate_orig)
	    ftruncate_orig = dlsym(RTLD_NEXT, "ftruncate");
	return ftruncate_orig(fd, length);
    }
}

int ftruncate(int fd, off_t length) __attribute__((alias("shm_ftruncate")));

int shm_close(int fd)
{
    maxfiles_init();

    if ((fd >=4) && (fd <= max_shm_files + 4))
        return 0;
    else
    {
	if(!close_orig)
	    close_orig = dlsym(RTLD_NEXT, "close");
	return close_orig(fd);
    }
}

int close(int fd) __attribute__((alias("shm_close")));
