#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <ashm.h>
#include <sys/wait.h>

char* oldldpreload;

char* setpreload(char* cmd)
{
    char* fincmd = malloc(strlen(cmd));
    char* ldpreload = malloc(strlen(cmd));

    sscanf(cmd, "LD_PRELOAD=%s %s", ldpreload, fincmd);

    setenv("LD_PRELOAD", ldpreload, 1);

    strcpy(fincmd, cmd);
    fincmd = index(fincmd, ' ') + 1;

    return fincmd;
}

int launch(char* cmd)
{
    printf("launching: %s\n", cmd);

    size_t cmdlen = strlen(cmd);
    char* newcmd = malloc(sizeof(char) * cmdlen);
    strcpy(newcmd, cmd);

    char* endcmd;

    if (strncmp(newcmd, "LD_PRELOAD", 10) == 0)
    {
	endcmd = setpreload(newcmd);
    } else
    {
	endcmd = newcmd;
    }

    int j = 0;
    for (int i = 0; i < cmdlen; i++)
    {
        if (endcmd[i] == ' ') j++;
    }
    j++;

    char** cargv = (char**) malloc(sizeof(char*) * j + 1);

    j = 0;
    char* tok = strtok(endcmd, " ");
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

    for (int i = 1; i < argc; i++)
    {

	oldldpreload = getenv("LD_PRELOAD");
	if (oldldpreload == NULL)
	    oldldpreload = "";

	int pid;
	if ((pid = launch(argv[i])) == -1)
	    error(errno, errno, "error launch");
	printf("pid=%d\n", pid);

        setenv("LD_PRELOAD", oldldpreload, 1);

	sleep(3);
    }

    while(wait(NULL) != -1) sleep(2);

    return 0;
}
