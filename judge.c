#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char** argv)
{
	if (argc < 2) {
		printf("judge argc < 2\n");
		return -1;
	}

	while (1) {
		char judge_yo[32];
		char buf[512];
		char *token;
		memset(judge_yo, 0, sizeof(judge_yo));
		sprintf(judge_yo, "Judge:%s", argv[1]);
		printf("%s\n", judge_yo);

		usleep(0);

		read(3, buf, sizeof(buf));

		printf("%s: read:%s,", judge_yo, buf);

		if (strncmp(buf, "0 0 0 0", strlen("0 0 0 0")) == 0) {
			printf("%s: get byebye\n", judge_yo);
			break;
		}

		token = strtok(buf, " ");
		printf("token = \"%s\"\n", token);

		write(4, token, strlen(token) + 1);
	}
	return 0;
}