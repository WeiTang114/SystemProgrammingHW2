#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argv, char** argc)
{
	if (argv < 4) {
		printf("argv is < 4\n");
		return -1;
	}

	printf("Player %s created.\n", argc[2]);
	fflush(stdout);

	char fifo_r_path[64];
	char fifo_w_path[64];
	char buf[64];
	sprintf(fifo_w_path, "./judge%s.FIFO", argc[1]);
	sprintf(fifo_r_path, "./judge%s_%s.FIFO", argc[1], argc[2]);
	int fifo_w = open(fifo_w_path, O_WRONLY);
	int fifo_r = open(fifo_r_path, O_RDONLY);
	read(fifo_r, buf, sizeof(buf));

	printf("Player %s get \"%s\"\n", argc[2], buf);

}