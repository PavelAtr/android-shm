#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ashm.h>
#include <sys/wait.h>

int launch(char* cmd)
{
    printf("launching: %s\n", cmd);
    size_t cmdlen = strlen(cmd);
    char* newcmd = malloc(sizeof(char) * cmdlen);
    strcpy(newcmd, cmd);
    int j = 0;
    for (int i = 0; i < cmdlen; i++)
    {
        if (newcmd[i] == ' ') j++;
    }
    j++;

    char** cargv = (char**) malloc(sizeof(char*) * j + 1);

    j = 0;
    char* tok = strtok(newcmd, " ");
    cargv[j] = tok;
    j++;
    while((tok = strtok(NULL, " ")) != NULL)
    {
        cargv[j] = tok;
        j++;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
	if (execv(cargv[0], cargv))
	    return -1;

    } else
    {
	if (pid == -1) return -1;
	return pid;
    }

    return 0;
}



int main(int argc, char** argv)
{

    if (shm_init() != 0)
	error(errno, errno, "shm_init() error");

//    setenv("LD_PRELOAD", "/usr/lib/i386-linux-gnu/libandroid-shm.so:/usr/lib/i386-linux-gnu/libshmmmap.so", 1);

    for (int i = 1; i < argc; i++)
    {
	int pid;
	if ((pid = launch(argv[i])) == -1)
	    error(errno, errno, "error launch");
	printf("pid=%d\n", pid);

	sleep(3);
    }

    while(wait(NULL) != -1) sleep(2);

    return 0;
}
