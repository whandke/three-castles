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

#define MEM_PLAYER_1 021
#define MEM_PLAYER_2 022
#define MEM_PLAYER_3 023

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

void sem_wait(int sem_id) {

    //printf("[SEMAPHORE] Wait! #%d\n", sem_id);
    struct sembuf semdwn;
    semdwn.sem_num = 0;
    semdwn.sem_op = -1;
    semdwn.sem_flg = 0;
    semop(sem_id, &semdwn, 1);
}

void sem_signal(int sem_id) {
    //printf("[SEMAPHORE] Signal! #%d\n", sem_id);
    struct sembuf semup;
    semup.sem_num = 0;
    semup.sem_op = 1;
    semup.sem_flg = 0;
    semop(sem_id, &semup, 1);
}

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

struct Semaphores {
    int p1;
    int p2;
    int p3;
};

struct Players {
    struct Player *p1;
    struct Player *p2;
    struct Player *p3;
    int p1_mem, p2_mem, p3_mem;
};

struct Unit {
    long type;
    int kind;
    int time;
};

struct MessageQueues queues_init(){
    struct MessageQueues mq;

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

struct Players players_mem_init(){
    struct Players players;

    players.p1_mem = shmget(MEM_PLAYER_1, sizeof(struct Player), IPC_CREAT | 0640);
    players.p2_mem = shmget(MEM_PLAYER_2, sizeof(struct Player), IPC_CREAT | 0640);
    players.p3_mem = shmget(MEM_PLAYER_3, sizeof(struct Player), IPC_CREAT | 0640);
    players.p1 = shmat(players.p1_mem, 0, 0);
    players.p2 = shmat(players.p2_mem, 0, 0);
    players.p3 = shmat(players.p3_mem, 0, 0);

    return players;
}

void send_notification(long id, char *content){
    int mq = msgget(MQ_NOTIFICATIONS, IPC_CREAT | 0640);

    struct NotificationMsg msg;
    int msg_size = sizeof(struct NotificationMsg) - sizeof(long);
    msg.type = id;
    strcpy(msg.content, content);
    msgsnd(mq, &msg, msg_size, 0);
}

void player_init(struct Player * player, long id) {
    printf("[INFO] Initializing Player #%ld\n", id);
    player->type = id;
    player->wins = 0;
    player->gold = 300;
    player->income = 50;
    player->workers = 0;
    player->light_inf = 0;
    player->heavy_inf = 0;
    player->cavalry = 0;
    player->status = STATUS_PREGAME;
    printf("[INFO] Done!\n");
}

void players_init(struct Players players){
    player_init(players.p1, 1l);
    player_init(players.p2, 2l);
    player_init(players.p3, 3l);
}

void connect_with_player(int mq_connection, struct Player *player, int id){
    printf("[CONNECT] Connecting with player #%d...\n", id);
    
    int conn_msg_size = sizeof(struct ConnectionMsg) - sizeof(long);
    struct ConnectionMsg conn_msg;
    msgrcv(mq_connection, &conn_msg, conn_msg_size, CONN_FROM_CLIENT, 0);

    conn_msg.type = CONN_FROM_SERVER;
    conn_msg.player_id = id;

    msgsnd(mq_connection, &conn_msg, conn_msg_size, 0);
    player->status = STATUS_CONNECTED;

    printf("[CONNECT] Player #%d connected!\n", id);
}

void connect_with_players(int mq_status, struct Players players) {
    printf("[INFO] Initializing connections...\n");
    connect_with_player(mq_status, players.p1, 1);
    connect_with_player(mq_status, players.p2, 2);
    connect_with_player(mq_status, players.p3, 3);
    printf("[INFO] Done!\n");    
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

void send_win_msg(long winner_id, int mq_status){
    printf("[INFO] Sending end message...\n");
    struct StatusMsg status_msg;
    status_msg.msg = winner_id;
    int status_msg_size = sizeof(struct StatusMsg) - sizeof(long);
    status_msg.type = 1;
    if(msgsnd(mq_status, &status_msg, status_msg_size, 0) == -1){
        printf("Something is not yes!\n");
    };
}

void check_win_condition(struct Players players, struct MessageQueues mq){
    if(players.p1->wins >= 5)
        send_win_msg(players.p1->type, mq.status);
    if(players.p2->wins >= 5)
        send_win_msg(players.p2->type, mq.status);
    if(players.p3->wins >= 5)
        send_win_msg(players.p3->type, mq.status);
}

void send_player_info(int mq_player_info, struct Player *player, int sem){
    struct Player msg;
    int player_size = sizeof(struct Player) - sizeof(long);
    sem_wait(sem);

    msg = *player;

    if(msgsnd(mq_player_info, &msg, player_size, 0) == -1){
        printf("[ERROR] Sending status to player #%ld\n", player->type);
    };
    sem_signal(sem);
}

void send_info(int mq_player_info, struct Players players, struct Semaphores sems){
    send_player_info(mq_player_info, players.p1, sems.p1);
    send_player_info(mq_player_info, players.p2, sems.p2);
    send_player_info(mq_player_info, players.p3, sems.p3);
}

void player_status(struct Player *p){
    printf("[STATUS] |Player #%ld|\n", p->type);
    printf("[STATUS] |  Vault  | Wins: %d Gold: %d Income: %d\n", p->wins, p->gold, p->income);
    printf("[STATUS] |  Army   | W: %d, L: %d, H: %d, C: %d\n\n", p->workers, p->light_inf, p->heavy_inf, p->cavalry);
}

void p_updater(struct MessageQueues mq, struct Players players, struct Semaphores sems){
    while(1){
        sleep(1);
        check_win_condition(players, mq);
        update_gold(players, sems);

        player_status(players.p1);
        player_status(players.p2);
        player_status(players.p3);

        send_info(mq.player_info, players, sems);
    }
}

void p_production_player(int mq_production, struct Player *player, int sem){
    struct Unit unit;
    int unit_size = sizeof(struct Unit) - sizeof(long);
    while(1){
        msgrcv(mq_production, &unit, unit_size, player->type, 0);
        sleep(unit.time);
        sem_wait(sem);
        switch(unit.kind){
            case UNIT_WORKER:
                player->income += 5;
                player->workers += 1;
                send_notification(player->type, "Worker trained, Sir!");
                break;
            case UNIT_LIGHT_INF:
                player->light_inf += 1;
                send_notification(player->type, "Light Infantry trained, Sir!");
                break;
            case UNIT_HEAVY_INF:
                player->heavy_inf += 1;
                send_notification(player->type, "Heavy Infantry trained, Sir!");
                break;
            case UNIT_CAVALRY:
                player->cavalry += 1;
                send_notification(player->type, "Cavalry trained, Sir!");
                break;
        }
        sem_signal(sem);
    }
}

void p_production(int mq_production, struct Players players, struct Semaphores sems){
    int f_p1, f_p2;
    
    f_p1 = fork();
    if(f_p1 == 0){
        p_production_player(mq_production, players.p1, sems.p1);
    }
    else {
        f_p2 = fork();
        if(f_p2 == 0){
            p_production_player(mq_production, players.p2, sems.p2);
        }
        else {
            p_production_player(mq_production, players.p3, sems.p3);
        }
    }

}

int calculate_cost(struct TrainingMsg msg){
    int unit_cost [] = {150, 100, 250, 550};
    return unit_cost[msg.kind] * msg.amount;
}

void p_training_player(int mq_production, struct Player *player, int sem, struct TrainingMsg msg){
    int unit_time [] = {2, 2, 3, 5};

    int cost = calculate_cost(msg);

    sem_wait(sem);
    if(cost > player->gold){
        send_notification(player->type, "Not enough gold, my Lord!");
        sem_signal(sem);
        return;
    }
    else
        player->gold -= cost;
    sem_signal(sem);

    send_notification(player->type, "Training ordered, Sir!");

    struct Unit unit;
    int unit_size = sizeof(struct Unit) - sizeof(long);
    unit.type = player->type;
    unit.kind = msg.kind;
    unit.time = unit_time[msg.kind];

    for(int i = 0; i < msg.amount; i++){
        msgsnd(mq_production, &unit, unit_size, 0);
    }
}

void p_training(struct MessageQueues mq, struct Players players, struct Semaphores sems){
    struct TrainingMsg training_msg;
    int training_msg_size = sizeof(struct TrainingMsg) - sizeof(long);
    while(1){
        msgrcv(mq.training, &training_msg, training_msg_size, 0, 0);
        printf("[INFO] Training order recieved (player: %d, type: %d, amount: %d)\n", training_msg.player_id, training_msg.kind, training_msg.amount);
        switch(training_msg.player_id){
            case 1:
                p_training_player(mq.production, players.p1, sems.p1, training_msg);
                break;
            case 2:
                p_training_player(mq.production, players.p2, sems.p2, training_msg);
                break;
            case 3:
                p_training_player(mq.production, players.p3, sems.p3, training_msg);
                break;
            default:
                break;
        }
    }

}

float calculate_def(struct Player *def){
    float unit_def [] = {0, 1.2, 3, 1.2};
    float power = 0;

    power += def->light_inf * unit_def[UNIT_LIGHT_INF];
    power += def->heavy_inf * unit_def[UNIT_HEAVY_INF];
    power += def->cavalry * unit_def[UNIT_CAVALRY];

    return power;
}

float calculate_off(struct AttackMsg msg){
    float unit_off [] = {0, 1, 1.5, 3.5};
    float power = 0;

    power += msg.light_inf * unit_off[UNIT_LIGHT_INF];
    power += msg.heavy_inf * unit_off[UNIT_HEAVY_INF];
    power += msg.cavalry * unit_off[UNIT_CAVALRY];

    return power;
}

void attack_finalize(struct Player *off, struct Player *def, int off_sem, int def_sem, struct AttackMsg msg){
    sleep(5);

    float off_power;
    float def_power;
    float ratio;

    sem_wait(off_sem);
    sem_wait(def_sem);

    off_power = calculate_off(msg);
    def_power = calculate_def(def);

    if(off_power > def_power){

        if(def_power == 0){
            off->light_inf += msg.light_inf;
            off->heavy_inf += msg.heavy_inf;
            off->cavalry += msg.cavalry;

            def->light_inf = 0;
            def->heavy_inf = 0;
            def->cavalry = 0;
        }
        else{
            ratio = def_power / off_power;

            printf("[ATTACK] Ratio %f\n", ratio);

            def->light_inf = 0;
            def->heavy_inf = 0;
            def->cavalry = 0;

            off->light_inf += msg.light_inf - (int)(ratio * msg.light_inf);
            off->heavy_inf += msg.heavy_inf - (int)(ratio * msg.heavy_inf);
            off->cavalry += msg.cavalry - (int)(ratio * msg.cavalry);
        }

        off->wins += 1;

        send_notification(off->type, "We crushed them, my Lord!");
        send_notification(def->type, "They've crushed us!");
        printf("[ATTACK] Player #%ld won! L: %d, H: %d, C: %d\n", off->type, off->light_inf, off->heavy_inf, off->cavalry);
    }
    else {
        if(def_power > 0){
            ratio = off_power / def_power;

            def->light_inf -= (int)(ratio * def->light_inf);
            def->heavy_inf -= (int)(ratio * def->heavy_inf);
            def->cavalry -= (int)(ratio * def->cavalry);

            send_notification(off->type, "Our army was too small, Sire. We lost.");
            send_notification(def->type, "We've defended the castle, Sir!");
            printf("[ATTACK] Player #%ld won!\n", def->type);
        }        
    }
    sem_signal(off_sem);
    sem_signal(def_sem);
}

int attack_begin(struct Player *off, int off_sem, struct AttackMsg msg){

    printf("[ATTACK] Light: %d, Heavy: %d, Cavalry: %d\n", msg.light_inf, msg.heavy_inf, msg.cavalry);

    int result = 1;

    sem_wait(off_sem);
    if(off->light_inf < msg.light_inf)
        result = 0;
    if(off->heavy_inf < msg.heavy_inf)
        result = 0;
    if(off->cavalry < msg.cavalry)
        result = 0;

    if(result == 1){
        off->light_inf -= msg.light_inf;
        off->heavy_inf -= msg.heavy_inf;
        off->cavalry -= msg.cavalry;
    }
    sem_signal(off_sem);

    return result;
}

void attack(struct MessageQueues mq, struct Player *off, struct Player *def, int off_sem, int def_sem, struct AttackMsg msg){
    printf("[ATTACK] Begin %ld -> %ld\n", off->type, def->type);
    if(attack_begin(off, off_sem, msg) == 0){
        send_notification(off->type, "We need more soldiers, Sire!");
        printf("[ATTACK] Not enough units!\n");
        return;
    }

    printf("[ATTACK] Legitimate and en route %ld -> %ld\n", off->type, def->type);
    if(fork() == 0){
        attack_finalize(off, def, off_sem, def_sem, msg);
        printf("[ATTACK] Finalized %ld -> %ld\n", off->type, def->type);
        sleep(20);
    }    
}

void p_attack(struct MessageQueues mq, struct Players players, struct Semaphores sems){
    struct AttackMsg attack_msg;
    int attack_msg_size = sizeof(struct AttackMsg) - sizeof(long);
    struct Player *off;
    struct Player *def;
    int off_sem, def_sem;
    while(1){
        printf("[INFO] Waiting for attack order\n");

        msgrcv(mq.attack, &attack_msg, attack_msg_size, 0, 0); 

        printf("[INFO] Attack order recieved (%ld -> %ld)\n", attack_msg.off_id, attack_msg.def_id);

        int o = attack_msg.off_id;
        int d = attack_msg.def_id;

        if(o == 1){
            off = players.p1;
            off_sem = sems.p1;
        }
        else if(o == 2){
            off = players.p2;
            off_sem = sems.p2;
        }
        else if(o == 3){
            off = players.p3;
            off_sem = sems.p3;
        }
        else printf("Error");

        if(d == 1){
            def = players.p1;
            def_sem = sems.p1;
        }
        else if(d == 2){
            def = players.p2;
            def_sem = sems.p2;
        }
        else if(d == 3){
            def = players.p3;
            def_sem = sems.p3;
        }
        else printf("Error2");

        attack(mq, off, def, off_sem, def_sem, attack_msg);

        printf("[INFO] Sending after attack info\n");
        off->status = STATUS_DISCONNECTED;
        send_player_info(mq.player_info, off, off_sem);
        printf("[INFO] After attack info sent\n");
    }
}

void p_await_exit_msg(struct MessageQueues mq){
    struct StatusMsg msg;
    int msg_size = sizeof(struct StatusMsg) - sizeof(long);
    msgrcv(mq.status, &msg, msg_size, 0, 0);
    
    if(msg.msg == 0){
        printf("[INFO] Player disconnected. Closing...\n");
        for(long i = 1; i < 4; i++){
            msg.type = i;
            msgsnd(mq.status, &msg, msg_size, 0);
        }
    }
    else {
        printf("[INFO] Player #%d won! Closing...\n", msg.msg);
        for(long i = 1; i < 4; i++){
            msg.type = i;
            msgsnd(mq.status, &msg, msg_size, 0);
        }
    }
}

int main(){
    printf("[INFO] Starting server...\n");

    int f_updater, f_production, f_training, f_attack;
    signal(SIGINT, SIG_IGN);

    printf("[INFO] Initializing Message Queues...\n");
    struct MessageQueues mq = queues_init();
    printf("[INFO] Done!\n");

    printf("[INFO] Initializing Message Queues...\n");
    struct Semaphores sems = semaphores_init();
    printf("[INFO] Done!\n");

    printf("[INFO] Initializing Shared memory...\n");
    struct Players players = players_mem_init();

    printf("[INFO] Done!\n");

    printf("[INFO] Initializing Players...\n");
    players_init(players);
    printf("[INFO] Done!\n");

    connect_with_players(mq.connection, players);

    send_notification(players.p1->type, "Start!");
    send_notification(players.p2->type, "Start!");
    send_notification(players.p3->type, "Start!");

    f_updater = fork();
    if(f_updater == 0){
        printf("[INFO] Updater process separated!\n");
        p_updater(mq, players, sems);    
    }
    else {
        f_training = fork();
        if(f_training == 0){
            printf("[INFO] Training process separated!\n");
            p_training(mq, players, sems);
        }
        else{
            f_production = fork();
            if(f_production == 0){
                printf("[INFO] Production process separated!\n");
                p_production(mq.production, players, sems);
            }
            else{
                f_attack = fork();
                if(f_attack == 0){
                    printf("[INFO] Attack process separated!\n");
                    p_attack(mq, players, sems);
                }
                else{
                    //Mother process
                    p_await_exit_msg(mq);
                    printf("[INFO] Recieved exit message!\n");
                    printf("[INFO] Closing server!\n");
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
    shmctl(players.p1_mem, IPC_RMID, 0);
    shmctl(players.p2_mem, IPC_RMID, 0);
    shmctl(players.p3_mem, IPC_RMID, 0);

    //Semaphores cleanup
    semctl(sems.p1, 0, IPC_RMID, 1);
    semctl(sems.p2, 0, IPC_RMID, 1);
    semctl(sems.p3, 0, IPC_RMID, 1);

    return 0;
}
