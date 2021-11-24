# android-shm

Posix SHM emulation for a process group on Android using /dev/ashmem.
The idea is to launch programs using the shm-launch command, which
opens many empty named file descriptors in /dev/ashmem.
Childs inherit these file descriptors and manipulate them through the library
libandroid-shm, which provides emulation of the shm_open(), shm_unlink() functions.
The functions shm_mmap(), shm_ftruncate(), shm_close() are also provided, since their
behavior in emulation is associated with the need for additional actions.

To compile, run:

make all

Macros can be used for simple editing of the client program:

#include <ashm.h>

#define mmap shm_mmap

#define ftruncate shm_ftruncate

#define close shm_close

LINK with -landroid-shm, and NOT link with -lrt. 

To run programs:

shm-launch "/bin/prog1 arg1 arg2" "/bin/prog2 arg1 arg2" ... 

!!! Launching client programs linked against libandroid-shm.so without using the shm-launch command may result in an unsafe ioctl over file descriptors.

It is possible not to recompile client programs, but use the LD_PRELOAD mechanism:

LD_PRELOAD=libandroid-shm.so shm-launch "/bin/prog1 arg1 arg2" "/bin/prog2 arg1 arg2"

or

shm-launch "LD_PRELOAD=libandroid-shm.so /bin/prog1 arg1 arg2" "LD_PRELOAD=libandroid-shm.so /bin/prog2 arg1 arg2" 

