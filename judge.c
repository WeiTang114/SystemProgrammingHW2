#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>

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
	int rand_key;
	int pid;
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


int create_fifos(char* judge_id)
{
	char fifojudge[32] = {};
	char fifoplayer[32] = {};

	sprintf(fifojudge, "./judge%s.FIFO", judge_id);
	mkfifo(fifojudge, 0666);

	for (int i = 0; i < 4; i++) {
		memset(fifoplayer, 0, sizeof(fifoplayer));
		sprintf(fifoplayer, "./judge%s_%c.FIFO", judge_id, PL_IDXES[i]);
		mkfifo(fifoplayer, 0666);
	}
	return 0;
}

void clean_fifos(char* judge_id) {
	char fifojudge[32] = {};
	char fifoplayer[32] = {};

	sprintf(fifojudge, "./judge%s.FIFO", judge_id);
	remove(fifojudge);

	for (int i = 0; i < 4; i++) {
		memset(fifoplayer, 0, sizeof(fifoplayer));
		sprintf(fifoplayer, "./judge%s_%c.FIFO", judge_id, PL_IDXES[i]);
		remove(fifoplayer);
	}
}

void init_players(char* judge_id, Player* players, int* player_ids)
{ 
	
	Player* p = NULL;
	int pid = 0;

	// gen random keys for 4 players
	srand(time(NULL));
	int rand_keys[4] = {-1, -1, -1, -1};
	for (int r = 0; r < 4; r++) {
		int key = -5;
		do {
			key = rand() % 65536;
		} while (key == rand_keys[0] || key == rand_keys[1] || key == rand_keys[2] || key == rand_keys[3]);
		rand_keys[r] = key;
	}

	for (int i = 0; i < 4; i++) {
		DP("fofofofo\n");
		p = &players[i];
		p->id = player_ids[i];
		p->idx = PL_IDXES[i];
		p->card_num = 0;
		p->rand_key = rand_keys[i];

		pid = fork();
		if (pid < 0) {
			perror("fork player");
		}
		else if (pid == 0) {
			dup2(ORI_STDIN, 0);
			dup2(ORI_STDOUT, 1);
			for (int i = 3; i < 128; i++) {
				close(i);
			}

			char rand_key_str[8] = {};
			char pl_idx_str[8] = {};
			sprintf(rand_key_str, "%d", p->rand_key);
			pl_idx_str[0] = p->idx;
			
			execl("./player", "./player", judge_id, pl_idx_str, rand_key_str,  (char *)0);
		}
		else {
			p->pid = pid;
		}
	}
}


int main(int argc, char** argv)
{
	if (argc < 2) {
		DP("judge argc < 2\n");
		return -1;
	}


	char judge_id[8] = {};
	char buf[512] = {};
	int player_ids[4] = {};
	Player players[4];
	sprintf(judge_id, "%s", argv[1]);

	char judge_yo[32];
	memset(judge_yo, 0, sizeof(judge_yo));
	sprintf(judge_yo, "Judge:%s", judge_id);
	DP("%s\n", judge_yo);

	while (1) {
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


		create_fifos(judge_id);
		init_players(judge_id, players, player_ids);

		char* token = strtok(buf, " ");
		DP("token = \"%s\"\n", token);
		printf("%s", token);
		fflush(stdout);
	}

	int status;
	for (int i = 0; i < 4; i++) {
		waitpid(players[i].pid, &status, 0);
	}
	return 0;
}