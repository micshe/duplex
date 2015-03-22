#include<sys/types.h>
#include<unistd.h>		/* for fork() */
#include<sys/socket.h>	/* for socketpair() */
#include<stdlib.h>		/* for exit() */
#include<errno.h>
#include<string.h>		/* for strcmp() strerror() */
#include<stdio.h>

/*
build with:
cc -o duplex duplex.c
*/

int crash(char*string,int err)
{
	fprintf(stderr,"duplex: error: '%s' errno: %d -- %s\n", string, err, strerror(err));
	exit(EXIT_FAILURE); 
	return 0;
} 

int launch(int upstream, int downstream, char*args[])
{
	pid_t child;

	child = fork();
	if(child==-1)
		return -1;
	if(child==0)
	{
		/* child */

		dup2(upstream,0)!=-1 || crash("redirecting stdin failed",errno);
		dup2(downstream,1)!=-1 || crash("redirecting stdout failed",errno);
		/* leave stderr as is */
	
		execvp(args[0], args); 
		exit(EXIT_FAILURE);
	}
	else
		/* parent */
		return 0;
}

int main(int argc, char*args[])
{
	int i;
	int err;

	int upstream;
	int downstream;
	int pair[2];
	int count;

	int start;
	int end;

	upstream = dup(0);	/* stdin */

	count = 0;
	start = 1;
	for(i=1;i<argc;++i)
		/* 
		the duplex | character is --- 
		it _must_ be either space, or
		quote delimited.  i'm not bothering
		to make a more inteligent parser.
		*/
		if(!strcmp(args[i],"---"))
		{
			/* remember the end of this component */
			end = i;
		
			/* prep the args[] array for exevp() */	
			args[i] = NULL;

			/* create the next pipeline */
			socketpair(AF_LOCAL,SOCK_STREAM,0,pair)==0 || crash("sockpair() failed.",errno);

			downstream = pair[0];

			/* launch this component */
			launch(upstream,downstream,&args[start])==0 || crash("launching inner process failed.",errno);
			close(upstream);
			close(downstream);

			/* prepare for launching the next component */
			upstream = pair[1];
			start = end + 1;

			++count;
		}

	/* now we process the last component */
	downstream = dup(1);	/* stdout */
	launch(upstream,downstream,&args[start])==0 || crash("launching final process failed.",errno); 
	close(upstream);
	close(downstream);
	++count;

	/* wait for all children to terminate */
	for(i=0;i<count;++i)
		wait(NULL);

	return EXIT_SUCCESS;
}
