#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#ifdef DEBUG
    #define DP(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
    #define DP(args...)
#endif

typedef enum {
	NEW, 
	PLAYING, 
	PLAYED
} GAME_STATE;

typedef struct {
	int players[4];
	int loser_idx;  // 0 to 3
	GAME_STATE state;
} Game;

typedef struct {
	int id;
	int pid;
	int fd_write;
	int fd_read;
	int busy;
	Game* game;
} Judge;

typedef struct {
	int id;
	int playing;
	int score;
} Player;

static int g_judge_num = 0;
static int g_player_num = 0;
static int g_game_num = 0;
static Game* g_games = NULL;
static Judge* g_judges = NULL;
static Player* g_players = NULL;

static int get_game_num(int player_num);
static Game* init_games(int game_num, int player_num);
static Judge* init_judges(int judge_num);
static Player* init_players(int player_num);
static void wait_judges(Judge* judges, int judge_num);

static void start_game(Game* games, int game_num, Judge* judges, int judge_num, Player* players, int player_num);
static void print_result(Player* players, int player_num);

int main(int argc, char* argv[])
{
	if (argc < 3) {
		printf("Argument number is < 3: %d, Exiting.\n", argc);
		return -1;
	}

	g_judge_num = atoi(argv[1]);
	g_player_num = atoi(argv[2]);
	g_game_num = get_game_num(g_player_num);
	g_games = init_games(g_game_num, g_player_num);
	g_judges = init_judges(g_judge_num);
	g_players = init_players(g_player_num);


	/*
	for (int i = 0; i < g_game_num; i++) {
		printf("%d %d %d %d\n", g_games[i].players[0],g_games[i].players[1],g_games[i].players[2],g_games[i].players[3] );
	}
	*/

	start_game(g_games, g_game_num, g_judges, g_judge_num, g_players, g_player_num);
	
	DP("after start_game\n");
	print_result(g_players, g_player_num);


	wait_judges(g_judges, g_judge_num);
	free(g_games);
	free(g_judges);
	free(g_players);
}


static int get_game_num(int player_num) 
{
	int pn = player_num;
	// C(pl, 4)
	return pn * (pn - 1) * (pn - 2) * (pn - 3) / (1 * 2 * 3 * 4);
}

static Game* init_games(int game_num, int player_num)
{
	Game* games = (Game*) calloc(game_num, sizeof(Game));
	int m;
	for (m = 0; m < game_num; m++) {
		games[m].loser_idx = -1;
		games[m].state = NEW;
	}

	int i, j, k, l, cnt;
	cnt = 0;
	for (i = 1; i <= player_num - 3; i++) {
		for (j = i + 1; j <= player_num - 2; j++) {
			for (k = j + 1; k <= player_num - 1; k++) {
				for (l = k + 1; l <= player_num; l++) {
					games[cnt].players[0] = i;
					games[cnt].players[1] = j;
					games[cnt].players[2] = k;
					games[cnt].players[3] = l;
					cnt ++;
				}
			}
		}
	}

	return games;
}


static Judge* init_judges(int judge_num)
{
	Judge* judges = (Judge*) calloc(judge_num, sizeof(Judge));
	int fds_w[2] = {0, 0};
	int fds_r[2] = {0, 0};
	int pid = 0;
	char id_buf[8];
	for (int i = 0; i < judge_num; i++) {
		if (pipe(fds_w) == -1) {
			perror("pipe");
			return NULL;
		}
		if (pipe(fds_r) == -1) {
			perror("pipe");
			return NULL;
		}

		pid = fork();
		if (pid == -1) {
			perror("fork");
            exit(EXIT_FAILURE);
		}
		if (pid == 0) { // child
			close(fds_w[1]);
			close(fds_r[0]);

			// dup stdin & stdout to a far-away fd
			// (fd 3 4 5 6 are occupied by fds_w and fds_r)
			dup2(0, 111);
			dup2(1, 112);

			dup2(fds_w[0], 0); // child's read
			dup2(fds_r[1], 1); // child's write

			// dup stdin & stdout back to 3 & 4
			dup2(111, 3);
			dup2(112, 4);

			memset(id_buf, 0, sizeof(id_buf));
			sprintf(id_buf, "%d", i + 1);
			execl("./judge", "./judge", id_buf, (char *)0);
			//exit(EXIT_SUCCESS);
		} 
		else { // parent
			close(fds_w[0]);
			close(fds_r[1]);
			judges[i].id = i + 1;
			judges[i].fd_read = fds_r[0];
			judges[i].fd_write = fds_w[1];
			judges[i].pid = pid;
			judges[i].busy = 0;
			judges[i].game = NULL;

			DP("New judge: %d  pid:%d  fd W:%d  R:%d\n", judges[i].id, pid, judges[i].fd_write, judges[i].fd_read);
		}
	}
	return judges;
}



static Player* init_players(int player_num) 
{
	Player* players = (Player*) calloc(player_num, sizeof(Player));
	for (int i = 0; i < player_num; i++) {
		players[i].id = i + 1;
		players[i].score = 0;
		players[i].playing = 0;
	}
	return players;
}


static void wait_judges(Judge* judges, int judge_num)
{
	int status;
	for (int i = 0; i < judge_num; i++) {
		waitpid(judges[i].pid, &status, 0);
	}
}


static int all_games_played(Game* games, int game_num)
{
	for (int i = 0; i < game_num; i++) {
		if (games[i].state != PLAYED) {
			return 0;
		}
	}
	return 1;
}


static Game* find_new_game(Game* games, int game_num, Player* players, int player_num)
{
	Game* game = NULL;
	for (int i = 0; i < game_num; i++) {
		if (games[i].state == NEW) {
			game = &games[i];
			break;
		}
	}
	return game;
}


static Judge* find_free_judge(Judge* judges, int judge_num)
{
	Judge* judge = NULL;
	for (int i = 0; i < judge_num; i++) {
		if (!judges[i].busy) {
			judge = &judges[i];
			break;
		}
	}
	return judge;
}

/**
 * [find_judge_by_fd description]
 * @param  fd [description]
 * @param  rw 0:r  1:w
 * @return    [description]
 */
static Judge* find_judge_by_fd(int fd, int rw, Judge* judges, int judge_num)
{
	int thisfd = 0;
	for (int i = 0; i < judge_num; i++) {
		thisfd = rw ? judges[i].fd_write : judges[i].fd_read;
		if (fd == thisfd) {
			return &judges[i];
		}
	}
	return NULL;
}

static void start_game(Game* games, int game_num, Judge* judges, int judge_num, Player* players, int player_num)
{
	fd_set master_set;
	fd_set read_set;
	struct timeval to_interval;
	int read_max_fd = 0;

	FD_ZERO(&master_set);
	for (int i = 0; i < judge_num; i++) {
		int fd = judges[i].fd_read;
		FD_SET(fd, &master_set);
		if (fd > read_max_fd) {
			read_max_fd = fd;
		}
	}

	while (1) {
		int ret = 0;
		int all_played = 0;
		while (1) {
			Judge* judge = find_free_judge(judges, judge_num);
			if (!judge){
				break;
			}
		
			Game* game = find_new_game(games, game_num, players, player_num);	
			if (game) {
				DP("found new game:%d %d %d %d, judge:%d\n", game->players[0],game->players[1],game->players[2],game->players[3], judge->id);
				judge->game = game;
				judge->busy = 1;
				judge->game->state = PLAYING;
				char buf[64];
				memset(buf, 0, sizeof(buf));
				for (int i = 0; i < 4; i++) {
					sprintf(buf, "%s%d ", buf, players[game->players[i] - 1].id);
				}
				DP("players:%s\n", buf);
				buf[strlen(buf) - 1] = '\0';

				write(judge->fd_write, buf, strlen(buf));
			}
			else { 
				if (all_games_played(games, game_num)) {
					all_played = 1;
				}
				break;
			}
		}

		if (all_played) {
			DP("All Game Played!\n");					
			char* bye = "0 0 0 0\n";
			for (int kk = 0; kk < judge_num; kk++) {
				write(judges[kk].fd_write, bye, strlen(bye));
			}
			break;
		}


		read_set = master_set;
        to_interval.tv_sec = 0;
        to_interval.tv_usec = 100;

		ret = select(read_max_fd + 1, &read_set, NULL, NULL, &to_interval);
		
		if (-1 == ret) {
			perror("select");
			continue;
		}
		else if (0 == ret) { //timeout
			continue;
		}
		for (int j = 0; j <= read_max_fd; j++) {
			if (FD_ISSET(j, &read_set)) {
				DP("FD_ISSET: %d\n", j);

				int r = 0;
				static char buf[512];
				Judge* judge = NULL;
				int loser = 0;

				judge = find_judge_by_fd(j, 0, judges, judge_num);
				memset(buf, 0, sizeof(buf));

				r = read(j, buf, sizeof(buf));
				if (r < 0){
					printf("read error\n");
				}
				else if (r == 0) {
					printf("read eof\n");
				}
				else {
					DP("Organizer get from pipe:%s\n", buf);

					loser = atoi(buf);
					DP("game ok: judge:%d, loser:%d\n", judge->id, loser);
					players[loser - 1].score --;
					judge->busy = 0;
					judge->game->state = PLAYED;
					judge->game = NULL;
				}
			}
		}
	}
}


int compare_player(const void *p1, const void *p2)
{
	return (((Player*)p1)->score - ((Player*)p2)->score);
}

static void print_result(Player* players, int player_num)
{
	qsort((void *)players, player_num, sizeof(Player), compare_player);
	for (int i = 0; i < player_num; i++) {
		DP("%d:%d ", players[i].id, players[i].score);
	}
	DP("\n");
	for (int i = 0; i < player_num; i++) {
		printf("%d ", players[i].id);
	}

	printf("\n");
}



