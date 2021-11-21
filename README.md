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
