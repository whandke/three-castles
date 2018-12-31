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
#define MQ_STATUS 204
#define MQ_PRODUCTION 205
#define MQ_CONNECTION 206

#define STATUS_PREGAME 300
#define STATUS_CONNECTED 301
#define STATUS_DISCONNECTED 302

#define CONN_FROM_SERVER 1
#define CONN_FROM_CLIENT 2

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

    return mq;
}

void player_init(struct Player * player, int id) {
    player->type = id;
    player->wins = 0;
    player->gold = 300;
    player->income = 0;
    player->workers = 1;
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

int main(){
    struct MessageQueues mq = queues_init();

    struct Player *player  = malloc(sizeof(*player));
    player_init(player, 0);

    connect_to_server(mq.connection, player);

    return 0;
}