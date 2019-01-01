#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>

#include <ncurses.h>

#define MEM_PLAYER_1 001
#define MEM_PLAYER_2 002
#define MEM_PLAYER_3 003

#define SEM_PLAYER_1 101
#define SEM_PLAYER_2 102
#define SEM_PLAYER_3 103

#define MQ_PLAYER_INFO 200
#define MQ_NOTIFICATIONS 201
#define MQ_ATTACK 202
#define MQ_TRAINING 203
#define MQ_STATUS 504
#define MQ_PRODUCTION 205
#define MQ_CONNECTION 206

#define STATUS_PREGAME 300
#define STATUS_CONNECTED 301
#define STATUS_DISCONNECTED 302

#define CONN_FROM_SERVER 1
#define CONN_FROM_CLIENT 2

#define REFRESH 1000

struct Player {
    int type;
    int wins;
    int gold;
    int income;
    int workers;
    int light_inf;
    int heavy_inf;
    int cavalry;
    int status;
};

struct ConnectionMsg {
    long type;
    int player_id;
};

struct StatusMsg {
    long type;
    int msg;
};

struct TrainingMsg {
    long type;
    int player_id;
    int kind;
    int amount;
};

struct AttackMsg {
    long type;
    int def_id;
    int off_id;
    int light_inf;
    int heavy_inf;
    int cavalry;
};

struct NotificationMsg {
    long type;
    char content [50];
};

struct MessageQueues {
    int player_info;
    int notifications;
    int attack;
    int training;
    int status;
    int production;
    int connection;
};

struct MessageQueues queues_init(){
    struct MessageQueues mq;

    mq.player_info = msgget(MQ_PLAYER_INFO, IPC_CREAT | 0640);
    mq.notifications = msgget(MQ_NOTIFICATIONS, IPC_CREAT | 0640);
    mq.attack = msgget(MQ_ATTACK, IPC_CREAT | 0640);
    mq.training = msgget(MQ_TRAINING, IPC_CREAT | 0640);
    mq.status = msgget(MQ_STATUS, IPC_CREAT | 0640);
    mq.production = msgget(MQ_PRODUCTION, IPC_CREAT | 0640);
    mq.connection = msgget(MQ_CONNECTION, IPC_CREAT | 0640);

    msgctl(MQ_PLAYER_INFO, IPC_RMID, 0);
    msgctl(MQ_NOTIFICATIONS, IPC_RMID, 0);
    msgctl(MQ_ATTACK, IPC_RMID, 0);
    msgctl(MQ_TRAINING, IPC_RMID, 0);
    msgctl(MQ_STATUS, IPC_RMID, 0);
    msgctl(MQ_PRODUCTION, IPC_RMID, 0);
    msgctl(MQ_CONNECTION, IPC_RMID, 0);

    mq.player_info = msgget(MQ_PLAYER_INFO, IPC_CREAT | 0640);
    mq.notifications = msgget(MQ_NOTIFICATIONS, IPC_CREAT | 0640);
    mq.attack = msgget(MQ_ATTACK, IPC_CREAT | 0640);
    mq.training = msgget(MQ_TRAINING, IPC_CREAT | 0640);
    mq.status = msgget(MQ_STATUS, IPC_CREAT | 0640);
    mq.production = msgget(MQ_PRODUCTION, IPC_CREAT | 0640);
    mq.connection = msgget(MQ_CONNECTION, IPC_CREAT | 0640);

    return mq;
}

void player_init(struct Player * player, int id) {
    player->type = id;
    player->wins = 0;
    player->gold = 0;
    player->income = 0;
    player->workers = 0;
    player->light_inf = 0;
    player->heavy_inf = 0;
    player->cavalry = 0;
    player->status = STATUS_PREGAME;
}

void connect_to_server(int mq_connection, struct Player *player){
    printf("[INFO] Connecting to server...\n");

    struct ConnectionMsg conn_msg;
    conn_msg.type = CONN_FROM_CLIENT;
    conn_msg.player_id = 0;

    int conn_msg_size = sizeof(struct ConnectionMsg) - sizeof(long);
    msgsnd(mq_connection, &conn_msg, conn_msg_size, 0);
    
    msgrcv(mq_connection, &conn_msg, conn_msg_size, CONN_FROM_SERVER, 0);

    player->type = conn_msg.player_id;
    player->status = STATUS_CONNECTED;

    printf("[INFO] Player #%d connected!\n", player->type);
}

void draw_info(WINDOW *win){
    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[INFO]");
    wattroff(win, A_REVERSE);
    wrefresh(win);
}

void draw_info_s(WINDOW *win, char * text){
    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[INFO]");
    wattroff(win, A_REVERSE);
    mvwprintw(win, 3, 1, "%s", text);
    wrefresh(win);
}

void draw_vault(WINDOW *win, struct Player * player){
    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[VAULT #%d]", player->type);
    wattroff(win, A_REVERSE);
    mvwprintw(win, 3, 1, "Wins:   %d", player->wins);
    mvwprintw(win, 5, 1, "Gold:   %d", player->gold);
    mvwprintw(win, 6, 1, "Income: %d", player->income);    
    wrefresh(win);
}

void draw_barracks(WINDOW *win, struct Player * player){
    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[BARRACKS]");
    wattroff(win, A_REVERSE);
    mvwprintw(win, 3, 1, "Workers:   %d", player->workers);
    mvwprintw(win, 4, 1, "Light Inf: %d", player->light_inf);
    mvwprintw(win, 5, 1, "Heavy Inf: %d", player->heavy_inf);
    mvwprintw(win, 6, 1, "Cavalry:   %d", player->cavalry);
    wrefresh(win);
}

void draw_actions(WINDOW *win){
    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[ACTIONS]");
    wattroff(win, A_REVERSE);

    mvwprintw(win, 2, 1, "X - number");

    mvwprintw(win, 3, 1, "Train");
    mvwprintw(win, 4, 1, "Worker:    w X");
    mvwprintw(win, 5, 1, "LightInf:  l X");
    mvwprintw(win, 6, 1, "HeavyInf:  h X");
    mvwprintw(win, 7, 1, "Cavalry:   c X");

    mvwprintw(win, 9, 1,  "Attack");
    mvwprintw(win, 10, 1, "       player l h c");
    mvwprintw(win, 11, 1, "            a X X X");
    mvwprintw(win, 12, 1, "            b X X X");

    wrefresh(win);
}

void recieve_player_info(int mq_player_info, struct Player * player){
    struct Player msg;
    int msg_size = sizeof(struct Player) - sizeof(long);
    if(msgrcv(mq_player_info, &msg, msg_size, player->type, IPC_NOWAIT) == -1)
        return;
    player->gold = msg.gold;
}

void recieve_notifications(int mq_notifications, struct Player * player, WINDOW * info){
    struct NotificationMsg msg;
    int msg_size = sizeof(struct NotificationMsg) - sizeof(long);
    if(msgrcv(mq_notifications, &msg, msg_size, player->type, IPC_NOWAIT) == -1)
        return;
    draw_info_s(info, msg.content);
}

int recieve_endgame_status(int mq_status, struct Player * player){
    struct StatusMsg msg;
    int msg_size = sizeof(struct StatusMsg) - sizeof(long);
    if(msgrcv(mq_status, &msg, msg_size, player->type, IPC_NOWAIT) == -1)
        return -1;
    return msg.msg;
}

void wait_for_start(int mq_notifications, struct Player * player, WINDOW * info){
    struct NotificationMsg msg;
    int msg_size = sizeof(struct NotificationMsg) - sizeof(long);
    msgrcv(mq_notifications, &msg, msg_size, player->type, 0);
    draw_info_s(info, msg.content);
}

int main(){
    struct MessageQueues mq = queues_init();

    struct Player *player  = malloc(sizeof(*player));
    player_init(player, 0);

    connect_to_server(mq.connection, player);

    initscr();
    cbreak();
    curs_set(0);
    WINDOW * info = newwin(5,50,1,1);
    WINDOW * vault = newwin(9, 25, 6, 1);
    WINDOW * barracks = newwin(9, 25, 6, 26);
    WINDOW * actions = newwin(14, 25, 1, 51);

    wait_for_start(mq.notifications, player, info);
    draw_vault(vault, player);
    draw_barracks(barracks, player);

    if(fork() == 0){
        while(1){
            recieve_notifications(mq.notifications, player, info);
            recieve_player_info(mq.player_info, player);
            draw_vault(vault, player);
            draw_barracks(barracks, player);

            usleep(100000);
        }        
    }
    else {
        while(1){
            int status = recieve_endgame_status(mq.status, player);
            if(status >= 0){
                delwin(vault);
                delwin(barracks);
                delwin(actions);
                delwin(info);
                endwin();
                if(status == 0){
                    printf("Player desconnected, closing game...\n");
                }
                else {
                    printf("Player #%d won!\n", status);
                }
                break;
            }


            
        }
    }                      
    
    int my_pid = getpid();
    signal(SIGQUIT, SIG_IGN);
    kill(-my_pid, SIGQUIT);
    sleep(2);
    free(player);
    sleep(2);
    return 0;
}