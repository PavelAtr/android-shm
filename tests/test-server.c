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
    int fd = shm_open("testfile", O_CREAT|O_RDWR, 0);
    if (fd == -1) error(errno, errno, "error shm_open()");

    char* str = shm_mmap(NULL, 100, PROT_READ|PROT_WRITE, MAP_SHARED,
                  fd, 0);
    if (str == MAP_FAILED)
        if (fd == -1) error(errno, errno, "error mmap()");

    strcpy(str, "Exchange worked!");

    return 0;
}
