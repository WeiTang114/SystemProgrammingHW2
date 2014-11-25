#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <utils.h>
#include <sys/time.h>

#define STDIN 0
#define STDOUT 1
#define ORI_STDIN 3
#define ORI_STDOUT 4

#ifdef DEBUG
    #define DP(format, args...) dprintf(ORI_STDOUT, "[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
    #define DP(args...)
#endif


#define my_write(fd, format, args...)        \
    do {                                                \
        char msg[64] = {};                              \
        sprintf(msg, format, ##args);                   \
        write(fd, msg, sizeof(msg));                    \
    } while(0)


const char PL_IDXES[4] = {'A', 'B', 'C', 'D'};

typedef struct {
    int id;
    char idx;
    int card_num;
    int rand_key;
    int pid;
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

void clear_fifos(char* judge_id) {
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
        p = &players[i];
        p->id = player_ids[i];
        p->idx = PL_IDXES[i];
        p->card_num = 0;
        p->rand_key = rand_keys[i];
        p->fifo_w = 0;

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


Player* get_player_by_idx(Player* players, char idx)
{
    for (int i = 0; i < 4; i++) {
        if (players[i].idx == idx) {
            return &players[i];
        }
    }
    return NULL;
}

void shuffle(int *array, size_t n)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);
    
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

void deal_cards(Player* players)
{
    int indexarr[53];
    int cards_tmp[53];
    int cards[53];
    char cards_buf[4][64] = {{}, {}, {}, {}};
    int index[4][2] = {{0, 13}, {14, 26}, {27, 39}, {40, 52}};

    DP("deal\n");

    // initialize cards
    for (int i = 0; i < 53; i++) {
        indexarr[i] = i;
        cards_tmp[i] = (i / 4) + 1;
    }   
    cards_tmp[52] = 0;

    // shuffle the card indexes
    shuffle(indexarr, 53);
    for (int j = 0; j < 53; j++) {
        cards[j] = cards_tmp[indexarr[j]];
    }

    // put cards to strings
    for (int k = 0; k < 4; k++) {
        memset(cards_buf[k], 0, sizeof(cards_buf[k]));
        for (int cd = index[k][0]; cd <= index[k][1]; cd++) {
            sprintf(cards_buf[k], "%s%d ", cards_buf[k], cards[cd]);
        }
        cards_buf[k][strlen(cards_buf[k]) - 1] = '\n';
    }

    // send
    for (int m = 0; m < 4; m++) {
        write(players[m].fifo_w, cards_buf[m], sizeof(cards_buf[m]));
        players[m].card_num = index[m][1] - index[m][0] + 1;
    }

}


int get_first_has_card(Player* players, int start)
{
    int cnt = 0;
    int pos = start;
    int index = -1;
    while (cnt < 4) {
        if (players[pos].card_num > 0) {
            index = pos;
            break;
        }
        pos = (pos + 1) % 4;
        cnt ++;
    }
    return index;
}

int get_next_players(Player* players, int* next_player_idxes)
{
    int* pl = next_player_idxes;
    int second = -1;
    if (pl[0] == -1 && pl[1] == -1) {
        pl[0] = get_first_has_card(players, 0);
    }
    else {
        pl[0] = get_first_has_card(players, (pl[0] + 1) % 4);
    }
    second = get_first_has_card(players, (pl[0] + 1) % 4);
    if (second != pl[0]) {
        pl[1] = second;
        return 2;
    }
    else {
        pl[1] = -1;
        return 1;
    }
}


int check_message(Player* pl, char** msg_tokens) {
    if (pl->idx != msg_tokens[0][0]) {
        DP("ERROR: receive message from wrong player:%s\n", msg_tokens[0]);
        sleep(3);
        return 0;
    }
    if (pl->rand_key != atoi(msg_tokens[1])) {
        DP("CHEATING!\n");
        return 0;
    }
    return 1;
}


char** my_read_from_player(int fd, Player* pl, char* buf, unsigned buflen)
{ 
    char** tokens;
    do {
        memset(buf, 0, buflen);
        read(fd, buf, buflen);
        tokens = get_tokens(buf);
    } while (!check_message(pl, tokens));
    return tokens;
}



/**
 * [run_game description]
 * @param  judge_id
 * @param  players
 * @return the loser's id (the "id" field in Player)
 */
int run_game(char* judge_id, Player* players)
{
    int loser_id = players[0].id;
    int fifo_j = 0;
    char fifojudge[32] = {};
    char fifoplayer[32] = {};

    DP("run_game() called\n");

    sprintf(fifojudge, "./judge%s.FIFO", judge_id);
    fifo_j = open(fifojudge, O_RDWR);
    for (int i = 0; i < 4; i++) {
        memset(fifoplayer, 0, sizeof(fifoplayer));
        sprintf(fifoplayer, "./judge%s_%c.FIFO", judge_id, PL_IDXES[i]);
        players[i].fifo_w = open(fifoplayer, O_RDWR);
    }

    deal_cards(players);


    int curr_pl[2] = {-1, -1};  // player index(0~3), the first is to get, and the second is to give
    char bufr[64] = {};
    char** tokens = NULL;


    for (int j = 0; j < 4; j++) {
        memset(bufr, 0, sizeof(bufr));
        read(fifo_j, bufr, sizeof(bufr));
        DP("Judge get %s\n", bufr);
        tokens = get_tokens(bufr);
        Player* pl = get_player_by_idx(players, tokens[0][0]);
        if (!check_message(pl, tokens)) {
            DP("Init Cheat\n");
        }
        pl->card_num = atoi(tokens[2]);
    }

    while (1) {
        int num = get_next_players(players, curr_pl);
        //DP("%d. %d\n", curr_pl[0], curr_pl[1]);
        if (num == 1) {
            loser_id = players[curr_pl[0]].id;
            DP("break run_game loop\n");
            break;
        }

        Player* pl0 = &players[curr_pl[0]];
        Player* pl1 = &players[curr_pl[1]];
        DP("This turn: %c gets from %c (%d cards)\n", pl0->idx, pl1->idx, pl1->card_num);

        // 1. j -> A : number of B           < 13
        my_write(pl0->fifo_w, "< %d\n", pl1->card_num);

        // 2. j <- A : card id to get        A 65535 6
        my_read_from_player(fifo_j, pl0, bufr, sizeof(bufr));

        // 3. j -> B : card id to get        > 6
        my_write(pl1->fifo_w, "> %s\n", tokens[2]);

        // 4. B -> j : the card of the id    B 65534 11
        my_read_from_player(fifo_j, pl1, bufr, sizeof(bufr));
        pl1->card_num --;

        // 5. j -> A : the card of the id    11
        my_write(pl0->fifo_w, "%s\n", tokens[2]);
        pl0->card_num ++;

        // 6. A -> j : whether eliminated    A 65535 1
        my_read_from_player(fifo_j, pl0, bufr, sizeof(bufr));
        int elim = atoi(tokens[2]);
        if (elim) {
            pl0->card_num -= 2;
        }
    }

    return loser_id;
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
    int loser_id = -1;

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

        loser_id = run_game(judge_id, players);

        DP("loser = %d\n", loser_id);
        printf("%d\n", loser_id);
        fflush(stdout);

        int status;
        for (int i = 0; i < 4; i++) {
            kill(players[i].pid, SIGKILL);
            waitpid(players[i].pid, &status, 0);
        }
        clear_fifos(judge_id);
    }

    return 0;
}