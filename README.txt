README
Systems Programming HW2
B00901067 李唐
--

= Build =
    - $ make
    - $ make DEBUG=1
    - $ make clean

= Execute =
    - $ ./organizer <judge_num> <player_num>  
    - result: 
        5 3 1 ... 2
        
= What I have done =
    1. organizer:
        organizer initializes judges, initializes players, dispatches 
        games to free judges and at last outputs the result. It first 
        fork judges, open pipes, and redirects stdin and stdout of the 
        judge processes to the read and write pipes seperately. Then, 
        it execl() the judges and dispatches the games.
        
    2. judge:
        judge reads the commands from organizer from stdin (redirected 
        to the pipe) and starts games. It first initializes 4 player 
        processes, and deals cards randomly to them. Then, judge and 
        the players communicates as specialized through fifos. If 
        judge finds some player sending a message with a wrong key, 
        it skips the message and read() the next one. When the game 
        is done, judge send the result to organizer through stdout.
        
    3. player:
        player are forked by judge and killed by judge. It reads the 
        first message to initialize its cards, and then iteragely 
        read messages, checks the command and does its specialized 
        works. 
        
    4. others:
        - random numbers: I used rand() in 2 cases: one for judges' 
            shuffling cards, and the other for players' getting cards
            from another player. I used current timestamp in ns 
            instead of seconds as the seed to generate random numbers.
            This makes the cases random enough, or the cards in 2 
            games may be distributed identically.
    
