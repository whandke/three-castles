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

#define UNIT_WORKER 0
#define UNIT_LIGHT_INF 1
#define UNIT_HEAVY_INF 2
#define UNIT_CAVALRY 3

#define REFRESH 1000

struct Player {
    long type;
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
    long def_id;
    long off_id;
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

    fprintf(stderr, "[INFO] Player #%ld connected!\n", player->type);
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

void draw_actions(WINDOW *win, struct Player *player){

    char actions[7][30] = {
        "Train Worker   ",
        "Train LightInf ",
        "Train HeavyInf ",
        "Train Cavalry  ",
        "Attack Player B",
        "Attack Player C",
        "Exit           "
        };


    wclear(win);
    box(win, 0, 0);
    wattron(win, A_REVERSE);
    mvwprintw(win, 1, 1, "[ACTIONS]");
    wattroff(win, A_REVERSE);

    int i = 0;

    for(i = 0; i < 7; i++){
        if(i == player->status)
            wattron(win, A_REVERSE);
        mvwprintw(win, i + 3, 1, actions[i]);
        wattroff(win, A_REVERSE);
    }

    wrefresh(win);
}

void player_status(struct Player *p){
    fprintf(stderr, "[STATUS] |Player #%ld| Status %d\n", p->type, p->status);
    fprintf(stderr, "[STATUS] |  Vault  | Wins: %d Gold: %d Income: %d\n", p->wins, p->gold, p->income);
    fprintf(stderr, "[STATUS] |  Army   | W: %d, L: %d, H: %d, C: %d\n\n", p->workers, p->light_inf, p->heavy_inf, p->cavalry);
}

void recieve_player_info(int mq_player_info, struct Player * player){
    struct Player msg;
    int msg_size = sizeof(struct Player) - sizeof(long);
    if(msgrcv(mq_player_info, &msg, msg_size, player->type, 0) == -1){
        fprintf(stderr, "[Error] Recive player info\n");
        return;
    }

    player->gold = msg.gold;
    player->income = msg.income;
    player->wins = msg.wins;
    player->light_inf = msg.light_inf;
    player->heavy_inf = msg. heavy_inf;
    player->cavalry = msg.cavalry;
    player->workers = msg.workers;

    player_status(player);    
}

void recieve_notifications(int mq_notifications, struct Player * player, WINDOW * info){
    struct NotificationMsg msg;
    int msg_size = sizeof(struct NotificationMsg) - sizeof(long);
    if(msgrcv(mq_notifications, &msg, msg_size, player->type, IPC_NOWAIT) == -1){
        fprintf(stderr, "[Error] Recive notification\n");
        return;
    }
    fprintf(stderr, "[Notif] %s\n", msg.content);
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
    while(1){
        msgrcv(mq_notifications, &msg, msg_size, player->type, 0);
        if(strcmp("Start!", msg.content) == 0) break;
    }    
}

void order_training(struct MessageQueues mq, struct Player *player, int unit_type){
    struct TrainingMsg msg;
    int msg_size = sizeof(struct TrainingMsg) - sizeof(long);
    msg.type = player->type;
    msg.player_id = player->type;
    msg.kind = unit_type;
    msg.amount = 1;

    msgsnd(mq.training, &msg, msg_size, 0);
    fprintf(stderr, "[INFO] Ordered training! Type: %d Amount: %d\n", msg.kind, msg.amount);
}

void order_attack(struct MessageQueues mq, struct Player *player, long enemy_id){
    struct AttackMsg msg;
    int msg_size = sizeof(struct AttackMsg) - sizeof(long);
    msg.type = 5;
    msg.off_id = player->type;
    msg.def_id = enemy_id;
    msg.light_inf = player->light_inf;
    msg.heavy_inf = player->heavy_inf;
    msg.cavalry = player->cavalry;

    msgsnd(mq.attack, &msg, msg_size, 0);
    fprintf(stderr, "[INFO] Ordered attack! Off: %ld Def: %ld Type: %ld\n", msg.off_id, msg.def_id, msg.type);
}

void send_exit_msg(struct MessageQueues mq, struct Player *player){
    struct StatusMsg msg;
    int msg_size = sizeof(struct StatusMsg) - sizeof(long);
    msg.type = player->type;
    msg.msg = 0;

    msgsnd(mq.status, &msg, msg_size, 0);
}

int main(){
    struct MessageQueues mq = queues_init();

    int p_mem = shmget(IPC_PRIVATE, sizeof(struct Player), IPC_CREAT | 0640);
    struct Player *player = shmat(p_mem, 0, 0);

    //struct Player *player  = malloc(sizeof(*player));

    player_init(player, 0);    

    connect_to_server(mq.connection, player);

    long enemy_A;
    long enemy_B;

    if(player->type == 1){
        enemy_A = 2;
        enemy_B = 3;
    }
    else if(player->type == 2){
        enemy_A = 1;
        enemy_B = 3;
    } else if(player->type == 3){
        enemy_A = 1;
        enemy_B = 2;
    }

    char buttons[7][30] = {
        "Train Worker   ",
        "Train LightInf ",
        "Train HeavyInf ",
        "Train Cavalry  ",
        "Attack Player B",
        "Attack Player C",
        "Exit           "
        };

    initscr();
    cbreak();
    noecho();
    curs_set(0);

    WINDOW * info = newwin(5,50,1,1);
    WINDOW * vault = newwin(9, 25, 6, 1);
    WINDOW * barracks = newwin(9, 25, 6, 26);
    WINDOW * actions = newwin(14, 25, 1, 51);

    /*
    keypad(stdscr, FALSE);
    keypad(info, FALSE);
    keypad(actions, TRUE);
    keypad(barracks, FALSE);
    keypad(vault, FALSE);
    */

    nodelay(stdscr, FALSE);
    nodelay(info, FALSE);
    nodelay(actions, TRUE);
    nodelay(barracks, FALSE);
    nodelay(vault, FALSE);

    /*
    leaveok(stdscr, TRUE);
    leaveok(info, TRUE);
    leaveok(actions, TRUE);
    leaveok(barracks, TRUE);
    leaveok(vault, TRUE);
    */

    box(info, 0, 0);
    box(actions, 0, 0);
    box(barracks, 0, 0);
    box(vault, 0, 0);
    wrefresh(info);
    wrefresh(actions);
    wrefresh(barracks);
    wrefresh(vault);

    wait_for_start(mq.notifications, player, info);

    if(fork() == 0){                    
        while(1){            
        draw_vault(vault, player);
        draw_barracks(barracks, player);  
        recieve_notifications(mq.notifications, player, info);   
        recieve_player_info(mq.player_info, player);       

        usleep(250000);
        }       
    }
    else {

        int choice;
        int highlight = 0;
        while(1){            

            usleep(100000);

            int status = recieve_endgame_status(mq.status, player);
            if(status >= 0){
                delwin(vault);
                delwin(barracks);
                delwin(actions);
                delwin(info);
                endwin();
                if(status == 0){
                    printf("Player disconnected, closing game...\n");
                }
                else {
                    printf("Player #%d won!\n", status);
                }
                break;
            }

            box(actions, 0, 0);
            redrawwin(actions);
            
            int i;
            for(i = 0; i < 7; i++) {
                if(i == highlight)
                    wattron(actions, A_REVERSE);
                mvwprintw(actions, i + 1, 1, buttons[i]);
                wattroff(actions, A_REVERSE);
            }
            choice = wgetch(actions);
            switch(choice) {
                case 'w':                    
                    highlight--;
                    if(highlight == -1)
                        highlight = 0;
                    break;
                case 's':
                    highlight++;
                    if(highlight == 7)
                        highlight = 6;
                    break;
                default:
                    break;
            }

            if(choice == 10){
                switch(highlight){
                    case 0:
                        order_training(mq, player, UNIT_WORKER);
                        break;
                    case 1:
                        order_training(mq, player, UNIT_LIGHT_INF);
                        break;
                    case 2:
                        order_training(mq, player, UNIT_HEAVY_INF);
                        break;
                    case 3:
                        order_training(mq, player, UNIT_CAVALRY);
                        break;
                    case 4:
                        order_attack(mq, player, enemy_A);
                        break;
                    case 5:
                        order_attack(mq, player, enemy_B);
                        break;
                    case 6:
                        send_exit_msg(mq, player);
                        break;
                }
            }
                          
        }
    }                   

    shmctl(p_mem, IPC_RMID, 0);   
    
    int my_pid = getpid();
    signal(SIGQUIT, SIG_IGN);
    kill(-my_pid, SIGQUIT);
    sleep(2);
    free(player);
    sleep(2);
    return 0;
}