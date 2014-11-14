#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argv, char** argc)
{
	if (argv < 4) {
		printf("argv is < 4\n");
		return -1;
	}

	printf("Player %s created.\n", argc[2]);
	fflush(stdout);

}