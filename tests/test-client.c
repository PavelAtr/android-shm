#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>
#include <ashm.h>

int main(int argc, char** argv)
{
    for (int i = 0; i < argc; i++)
        printf("%s\n", argv[i]);

    int fd = shm_open("testfile", O_RDONLY, 0);
    if (fd == -1) error(errno, errno, "error shm_open()");

    char* str = mmap(NULL, 100, PROT_READ|PROT_WRITE, MAP_SHARED,
                  fd, 0);

    if (str == MAP_FAILED)
        error(errno, errno, "error mmap()");

    printf("%s\n", str);

    shm_unlink("testfile");

    return 0;
}
