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

void sem_wait(int sem_id) {
    struct sembuf semdwn;
    semdwn.sem_num = 0;
    semdwn.sem_op = -1;
    semdwn.sem_flg = 0;
    semop(sem_id, &semdwn, 1);
}

void sem_signal(int sem_id) {
    struct sembuf semup;
    semup.sem_num = 0;
    semup.sem_op = 1;
    semup.sem_flg = 0;
    semop(sem_id, &semup, 1);
}

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

struct Semaphores {
    int p1;
    int p2;
    int p3;
};

struct Players {
    struct Player *p1;
    struct Player *p2;
    struct Player *p3;
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

struct Semaphores semaphores_init(){
    struct Semaphores sem;

    sem.p1 = semget(SEM_PLAYER_1, 1, IPC_CREAT | 0640);
    sem.p2 = semget(SEM_PLAYER_2, 1, IPC_CREAT | 0640);
    sem.p3 = semget(SEM_PLAYER_3, 1, IPC_CREAT | 0640);

    semctl(sem.p1, 0, SETVAL, 1);
    semctl(sem.p2, 0, SETVAL, 1);
    semctl(sem.p3, 0, SETVAL, 1);

    return sem;
}

void player_init(struct Player * player, int id) {
    player->type = id;
    player->wins = 0;
    player->gold = 300;
    player->income = 10;
    player->workers = 1;
    player->light_inf = 0;
    player->heavy_inf = 0;
    player->cavalry = 0;
    player->status = STATUS_PREGAME;
}

void players_init(struct Players players){
    player_init(players.p1, 1);
    player_init(players.p2, 2);
    player_init(players.p3, 3);
}

void connect_with_player(int mq_connection, struct Player *player, int id){
    printf("[INFO] Connecting with player #%d...\n", id);
    
    int conn_msg_size = sizeof(struct ConnectionMsg) - sizeof(long);
    struct ConnectionMsg conn_msg;
    msgrcv(mq_connection, &conn_msg, conn_msg_size, CONN_FROM_CLIENT, 0);

    conn_msg.type = CONN_FROM_SERVER;
    conn_msg.player_id = id;

    msgsnd(mq_connection, &conn_msg, conn_msg_size, 0);
    player->status = STATUS_CONNECTED;

    printf("[INFO] Player #%d connected!\n", id);
}

void connect_with_players(int mq_status, struct Players players) {
    connect_with_player(mq_status, players.p1, 1);
    connect_with_player(mq_status, players.p2, 2);
    connect_with_player(mq_status, players.p3, 3);
    printf("[INFO] All players have joined the game!\n");    
}

void update_gold_player(struct Player *player, int sem){
    sem_wait(sem);
    player->gold += player->income;
    sem_signal(sem);
}

void update_gold(struct Players players, struct Semaphores sems){
    update_gold_player(players.p1, sems.p1);
    update_gold_player(players.p2, sems.p2);
    update_gold_player(players.p3, sems.p3);
}

void p_updater(struct Players players, struct MessageQueues mq, struct Semaphores sems){
    while(1){
        sleep(1);
        //check_win_condition(players); //todo
        update_gold(players, sems);
        printf("[INFO] Gold status |#1:  %d |#2:  %d |#3:  %d |\n", players.p1->gold, players.p2->gold, players.p3->gold);
        //send_player_info(mq.player_info, players, sems); //todo
    }
}

int main(){
    printf("[INFO] Starting server...\n");

    signal(SIGINT, SIG_IGN);

    struct MessageQueues mq = queues_init();
    struct Semaphores sems = semaphores_init();

    int f_updater, f_production, f_training, f_attack;

    struct Players players;

    int p1_ma = shmget(MEM_PLAYER_1, sizeof(struct Player), IPC_CREAT | 0640);
    int p2_ma = shmget(MEM_PLAYER_2, sizeof(struct Player), IPC_CREAT | 0640);
    int p3_ma = shmget(MEM_PLAYER_3, sizeof(struct Player), IPC_CREAT | 0640);
    players.p1 = shmat(p1_ma, 0, 0);
    players.p2 = shmat(p2_ma, 0, 0);
    players.p3 = shmat(p3_ma, 0, 0);

    players_init(players);

    connect_with_players(mq.connection, players);

    f_updater = fork();
    if(f_updater == 0){
        printf("[INFO] Updater process separated!\n");
        p_updater(players, mq, sems);        
    }
    else {
        f_production = fork();
        if(f_production == 0){
            printf("[INFO] Production process separated!\n");
            //p_production();//todo
        }
        else{
            f_training = fork();
            if(f_training == 0){
                printf("[INFO] Training process separated!\n");
                //p_training();//todo
            }
            else{
                f_attack = fork();
                if(f_attack == 0){
                    printf("[INFO] Attack process separated!\n");
                    //p_attack();//todo
                }
                else{
                    //Mother process
                    //p_await_exit_msg();//todo
                    sleep(10);
                    printf("[INFO] Recieved exit message!\n");
                }
            }

        }
    }

    //Ending sequence
    int my_pid = getpid();
    signal(SIGQUIT, SIG_IGN);
    kill(-my_pid, SIGQUIT);
    printf("[INFO] PID-%d memory cleanup...\n", my_pid);
    sleep(2);

    //Messege queues cleanup
    msgctl(MQ_PLAYER_INFO, IPC_RMID, 0);
    msgctl(MQ_NOTIFICATIONS, IPC_RMID, 0);
    msgctl(MQ_ATTACK, IPC_RMID, 0);
    msgctl(MQ_TRAINING, IPC_RMID, 0);
    msgctl(MQ_STATUS, IPC_RMID, 0);
    msgctl(MQ_PRODUCTION, IPC_RMID, 0);
    msgctl(MQ_CONNECTION, IPC_RMID, 0);
    
    //Shared memory cleanup
    shmctl(p1_ma, IPC_RMID, 0);
    shmctl(p2_ma, IPC_RMID, 0);
    shmctl(p3_ma, IPC_RMID, 0);

    //Semaphores cleanup
    semctl(sems.p1, 0, IPC_RMID, 1);
    semctl(sems.p2, 0, IPC_RMID, 1);
    semctl(sems.p3, 0, IPC_RMID, 1);

    printf("Closing server!\n\n");
    sleep(1);

    return 0;
}
