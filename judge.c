#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#define STDIN 0
#define STDOUT 1
#define ORI_STDIN 3
#define ORI_STDOUT 4

#ifdef DEBUG
    #define DP(format, args...) dprintf(ORI_STDOUT, "[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
    #define DP(args...)
#endif



const char PL_IDXES[4] = {'A', 'B', 'C', 'D'};

typedef struct {
	int id;
	char idx;
	int card_num;
	int fifo_w;
} Player;



int compare_int(const void *i1, const void *i2)
{
	return ((*(int*)i1) - (*(int*)i2));
}


int parse_player_ids(char* ids_str, int* ids_buf)
{
	DP("parse_player_ids:");

	char* str = strdup(ids_str);
	char* pch;

	pch = strtok(str, " ");
	for (int i = 0; i < 4; i++) {
		if (pch) {
			ids_buf[i] = atoi(pch);
		}
		else {
			return -1;
		}
		DP("%d ", ids_buf[i]);
		pch = strtok(NULL, " ");
	}
	DP("\n");

	qsort((void *)ids_buf, 4, sizeof(int), compare_int);
	
	free(str);
	return 0;
}


void init_players(Player* players, int* player_ids)
{ 
	
	Player* p = NULL;
	for (int i = 0; i < 4; i++) {
		p = &players[i];
		p->id = player_ids[i];
		p->idx = PL_IDXES[i];
		p->card_num = 13;

	}
}


int main(int argc, char** argv)
{
	if (argc < 2) {
		DP("judge argc < 2\n");
		return -1;
	}

	char judge_yo[32];
	memset(judge_yo, 0, sizeof(judge_yo));
	sprintf(judge_yo, "Judge:%s", argv[1]);
	DP("%s\n", judge_yo);
	fflush(stdout);

	char buf[512];
	int player_ids[4];
	Player players[4];

	while (1) {
		fflush(stdout);
		memset(buf, 0, sizeof(buf));
		read(STDIN, buf, sizeof(buf));
		DP("%s: read:%s,", judge_yo, buf);

		if (strncmp(buf, "0 0 0 0", strlen("0 0 0 0")) == 0) {
			DP("%s: get byebye\n", judge_yo);
			break;
		}

		if (parse_player_ids(buf, player_ids) != 0) {
			DP("Error: parse_player_ids failed\n");
			continue;
		}

		init_players(players, player_ids);

		char* token = strtok(buf, " ");
		DP("token = \"%s\"\n", token);
		printf("%s", token);
	}
	return 0;
}