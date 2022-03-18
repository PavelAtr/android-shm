#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <error.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
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
    fincmd = strchr(fincmd, ' ') + 1;

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
    cargv[j] = NULL;

    printf("execvp(\"%s\", cargv)", cargv[0]);
    pid_t pid = fork();
    if (pid == 0)
    {
	if (execvp(cargv[0], cargv))
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
    char* oldpreload;

    if (shm_init() != 0)
	error(errno, errno, "shm_init() error");

    sleep(10);

    for (int i = 1; i < argc; i++)
    {

	char* tmp = getenv("LD_PRELOAD");
	if (tmp == NULL)
	    oldldpreload = "";
	else
	{
	    oldpreload = malloc(strlen(tmp));
	    strcpy(oldpreload, tmp);
	}


	int pid;
	char* safecmd = malloc(strlen(argv[i]));
	safecmd = strcpy(safecmd, argv[i]);
	if ((pid = launch(safecmd)) == -1)
	    error(errno, errno, "error launch");
	printf("pid=%d\n", pid);

        setenv("LD_PRELOAD", oldldpreload, 1);

	sleep(3);
    }

    while(wait(NULL) != -1) sleep(2);

    return 0;
}
