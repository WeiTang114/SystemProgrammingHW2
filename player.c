#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include "utils.h"
#include <time.h>
#include <sys/time.h>

#ifdef DEBUG
    #define DP(format, args...) printf("[%s:%d] "format, __FILE__, __LINE__, ##args)
#else
    #define DP(args...)
    //#define printf(args...)
    //#define dprintf(args...)
#endif
//#define printf(format, args...)

#define my_write(playeridx, fd, format, args...)        \
    do {                                                \
        char msg[64] = {};                              \
        sprintf(msg, format, ##args);                   \
        DP("Player %c write: \"%s\"\n", playeridx, msg);\
        write(fd, msg, sizeof(msg));                    \
    } while(0)

typedef struct {
    int c[15];
    int num;
} Cards;


void my_read(char playeridx, int fd, char* buf, unsigned buflen)
{
    memset(buf, 0, buflen);
    read(fd, buf, buflen);
    DP("Player %c get \"%s\"\n", playeridx, buf);
}


void print_cards(char playeridx, Cards* cards)
{
    printf("Player:%c, Cards:", playeridx);
    for (int i = 0; i < cards->num; i++) {
        printf("%d ", cards->c[i]);
    }
    printf(", num = %d\n", cards->num);
}

/**
 * [find_card description]
 * @param  cards [description]
 * @param  value [description].
 * @param  start the start index (1 to cards->num)
 * @return       idx: from 1 to cards->num, the index of the first card found
 *                    or -1: not found
 */
int find_card(Cards* cards, int value, int start)
{
    int idx = -1;
    for (int i = start - 1; i < cards->num; i++) {
        if (cards->c[i] == value) {
            idx = i + 1;
            break;
        }
    }
    return idx;
}


int remove_card(Cards* cards, int idx) 
{   
    if (idx > cards->num) {
        DP("card idx > card num\n");
        return -1;
    }
    int val = cards->c[idx - 1];
    for (int i = idx - 1; i <= cards->num - 2; i++) {
        cards->c[i] = cards->c[i + 1];
    }
    cards->num --;
    DP("remove_card: idx %d , value = %d\n", idx, val);
    return val;
}


Cards* init_cards(char playeridx, char* cardsmsg)
{
    char* msg = strdup(cardsmsg);
    char* token = NULL;
    int num = 0;
    Cards* cards = (Cards*) malloc(sizeof(Cards));
    memset(cards, 0, sizeof(Cards));

    token = strtok(msg, " ");
    while (token) {
        cards->c[num] = atoi(token);
        num ++;
        token = strtok(NULL, " ");
    }
    cards->num = num;
#ifdef DEBUG
    print_cards(playeridx, cards);
#endif

    for (int idx = 1; idx <= cards->num; idx++) {
        int val = cards->c[idx - 1];
        int nextidx = find_card(cards, val, idx + 1);
        if (nextidx > 0) {
            remove_card(cards, nextidx);
            remove_card(cards, idx);
            idx --;
        }
    }

#ifdef DEBUG
    print_cards(playeridx, cards);
#endif
    free(msg);
    return cards;
}


/**
 * [add_card description]
 * @param  cards [description]
 * @param  value [description]
 * @return       1: eliminated a pair  0: no elimination
 */
int add_card(Cards* cards, int value)
{
    int idx = find_card(cards, value, 1);
    if (idx > 0) {
        remove_card(cards, idx);
        return 1;
    }

    cards->c[cards->num] = value;
    cards->num ++;
    DP("add cards: value = %d\n", value);
    return 0;
}

int main(int argv, char** argc)
{
    if (argv < 4) {
        printf("argv is < 4\n");
        return -1;
    }

    DP("Player %s created.\n", argc[2]);

    char fifo_r_path[64];
    char fifo_w_path[64];
    char buf[64];
    char** tokens = NULL;
    int msg_value = 0;
    char playeridx = 0;
    int rand_key = 0;
    Cards* cards = NULL;

    playeridx = argc[2][0];
    rand_key = atoi(argc[3]);

    sprintf(fifo_w_path, "./judge%s.FIFO", argc[1]);
    sprintf(fifo_r_path, "./judge%s_%c.FIFO", argc[1], playeridx);
    int fifo_w = open(fifo_w_path, O_WRONLY);
    int fifo_r = open(fifo_r_path, O_RDONLY);
    
    struct timeval tv;
    gettimeofday(&tv,NULL);
    srand(tv.tv_usec);

    my_read((char) playeridx, fifo_r, buf, sizeof(buf));
    cards = init_cards(playeridx, buf);
    my_write((char) playeridx, fifo_w, "%c %d %d\n", (char) playeridx, rand_key, cards->num);
    while (1) {
        my_read(playeridx, fifo_r, buf, sizeof(buf));
        tokens = get_tokens(buf);
        
        if (strcmp(tokens[0], "<") == 0) {
            // 1. j -> A : number of B           < 13
            int n_card_nextp = atoi(tokens[1]);
            int get_idx = (rand() % n_card_nextp) + 1;
            DP("Player %c wants to get %d from %d\n", playeridx, get_idx, n_card_nextp);
            msg_value = get_idx;
        }
        else if (strcmp(tokens[0], ">") == 0) {
            // 3. j -> B : card id to get        > 6
            int got_idx = atoi(tokens[1]);
            int value = remove_card(cards, got_idx);
        #ifdef DEBUG
            print_cards(playeridx, cards);
        #endif
            msg_value = value;
        }
        else {
            // 5. j -> A : the card of the id    11
            int got_val = atoi(tokens[0]);
            int elim = add_card(cards, got_val);
            msg_value = elim;
        #ifdef DEBUG
            print_cards(playeridx, cards);
        #endif
        }
        my_write((char) playeridx, fifo_w, "%c %d %d\n", playeridx, rand_key, msg_value);
    }

}