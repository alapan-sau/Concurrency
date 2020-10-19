#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <limits.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <semaphore.h>
#include <errno.h>
#include <string.h>


char* RED =  "\e[0;31m";
char* GREEN =  "\e[0;32m";
char * RESET =  "\e[0m";

# define NYA 0
# define WTP 1
# define ATP 7
# define PSo 2
# define PSi 3
# define WTT 4
# define TSC 5
# define Ex 6

# define FR 0
# define M 1
# define S 2
# define MS 3

#define TYPE_ACOUSTIC 1
#define TYPE_ELECTRIC 2
#define TYPE_BOTH 3
#define TYPE_SINGER 4

typedef struct musician{
int id;
pthread_t tid;
char name[100];
int arrtime;
int stage_id;
int type;
int status;
int play_time;
pthread_mutex_t mutex;
pthread_cond_t cond_mutex;
}musician;


typedef struct stage{
int id;
int status;
pthread_mutex_t mutex;
int type;
int musician;
int singer;
sem_t sem;
}stage;


typedef struct coordinator{
int id;
pthread_t tid;
pthread_mutex_t mutex;
}coordinator;

musician ** MUSICIAN;
stage ** STAGE;
coordinator ** COORDINATOR;

typedef struct waiter{
musician * player;
pthread_t tid;
int type;
}waiter;

musician ** MUSICIAN;
stage ** STAGE;
coordinator ** COORDINATOR;

int num_player;
int num_stage;
// int num_coordinator;

sem_t acoustic;
sem_t electric;
sem_t singer;
sem_t both;
sem_t coordinate;

int k,a,e,c,t1,t2,t;


///////////////////////////////////////////////////////////////////////////////////////////////

void getStage(int type, musician * player){
    for(int i=0;i<a+e;i++){
        if(STAGE[i]->type!=type) continue;
        if(STAGE[i]->status==FR){
            pthread_mutex_lock(&(STAGE[i]->mutex));
            if(STAGE[i]->status==FR){
                STAGE[i]->musician = player->id;
                if(player->type == TYPE_SINGER)STAGE[i]->status = S;
                else STAGE[i]->status = M;
                player->stage_id = i;
                player->status = PSo;
                sem_post(&singer); // position~
                pthread_mutex_unlock(&(STAGE[i]->mutex));
                return;
            }
            pthread_mutex_unlock(&(STAGE[i]->mutex));
        }
    }
}

void * waitStage(void * temp){
    waiter * self = (waiter *)temp;
    musician * player = self->player;

    if(self->type == TYPE_ACOUSTIC) sem_wait(&acoustic);
    else if(self->type == TYPE_ELECTRIC) sem_wait(&electric);
    else sem_wait(&singer);

    pthread_mutex_lock(&player->mutex);
    if(player->status == WTP){
        player->type = self->type;
        player->status = ATP;
        pthread_mutex_unlock(&player->mutex);
        pthread_cond_signal(&player->cond_mutex);
        return NULL;
    }
    if(self->type == TYPE_ACOUSTIC) sem_post(&acoustic);
    else if(self->type == TYPE_ELECTRIC) sem_post(&electric);
    pthread_mutex_unlock(&player->mutex);
    return NULL;
}

void handleSinger(musician * player){
    for(int i=0;i<k;i++){
        if(MUSICIAN[i]->type != TYPE_SINGER) continue;
        if(MUSICIAN[i]->status == ATP || MUSICIAN[i]->status == WTP){
            pthread_mutex_lock(&(MUSICIAN[i]->mutex));
            if(MUSICIAN[i]->status == ATP){
                player->status = PSi;
                MUSICIAN[i]->status = PSi;
                STAGE[player->stage_id]->status = MS;
                printf("Singer %s joins %s in stage %d\n",MUSICIAN[i]->name,player->name,player->stage_id);
                sleep(2);
                STAGE[player->stage_id]->status = FR;
                player->status = WTT;
                MUSICIAN[i]->status = WTT;
                if(player->type == TYPE_ACOUSTIC){
                    printf("Singer %s and Musician %s are done performing on stage %d\n",MUSICIAN[i]->name,player->name,player->stage_id);
                    sem_post(&acoustic);
                }
                else{
                    printf("Singer %s and Musician %s are done performing on stage %d\n",MUSICIAN[i]->name,player->name,player->stage_id);
                    sem_post(&electric);
                }
                pthread_mutex_unlock(&(MUSICIAN[i]->mutex));
                pthread_cond_signal(&(MUSICIAN[i]->cond_mutex));
                return;
            }
            else{
                pthread_mutex_unlock(&(MUSICIAN[i]->mutex));
            }
        }
    }
    if(player->type == TYPE_ACOUSTIC){
        printf("%s performance done on acoustic stage %d\n",player->name,player->stage_id);
        sem_post(&acoustic);
    }
    else{
        printf("%s performance done on electric stage %d\n",player->name,player->stage_id);
        sem_post(&electric);
    }
}

void * musician_procedure(void * temp){
    musician * self = (musician *)temp;
    sleep(self->arrtime);
    self->status = WTP;
    printf("%s arrived \n",self->name);
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts)==-1){
        perror("gettime ");
        return NULL;
    }
    ts.tv_sec += t;
    ts.tv_nsec = 0;

    if(self->type == TYPE_ACOUSTIC){
        if(sem_timedwait(&acoustic,&ts)==-1 && errno == ETIMEDOUT){
            printf("%s didn't get a stage and left\n",self->name);
            return NULL;
        }
        else{
            getStage(TYPE_ACOUSTIC,self);
            printf("%s starting performance on acoustic stage %d\n",self->name,self->stage_id);
            sleep(self->play_time);
            if(sem_trywait(&singer)==0){
                self->status = WTT;
                pthread_mutex_lock(&(STAGE[self->stage_id]->mutex));
                sem_post(&acoustic);
                STAGE[self->stage_id]->status = FR;
                pthread_mutex_unlock(&(STAGE[self->stage_id]->mutex));
                printf("%s performance done on acoustic stage %d\n",self->name,self->stage_id);
            }
            else{
                handleSinger(self);
            }
        }
    }
    else if(self->type == TYPE_ELECTRIC){
        if(sem_timedwait(&electric,&ts)==-1 && errno == ETIMEDOUT){
            printf("%s didn't get a stage and left\n",self->name);
            return NULL;
        }
        else{
            getStage(TYPE_ELECTRIC,self);
            printf("%s starting performance on electric stage %d\n",self->name,self->stage_id);
            sleep(self->play_time);
            if(sem_trywait(&singer)==0){
                self->status = WTT;
                pthread_mutex_lock(&(STAGE[self->stage_id]->mutex));
                STAGE[self->stage_id]->status = FR;
                sem_post(&electric);
                pthread_mutex_unlock(&(STAGE[self->stage_id]->mutex));
                printf("%s performance done on electric stage %d\n",self->name,self->stage_id);
            }
            else{
                handleSinger(self);
            }
        }
    }
    else if(self->type == TYPE_BOTH){
        pthread_mutex_lock(&(self->mutex));
        waiter * waitAc;
        waitAc = malloc(sizeof(waiter));
        waitAc->type = TYPE_ACOUSTIC;
        waitAc->player = self;
        waiter * waitEc;
        waitEc = malloc(sizeof(waiter));
        waitEc->type = TYPE_ELECTRIC;
        waitEc->player = self;
        pthread_create(&(waitAc->tid),NULL,waitStage,(void *)waitAc);
        pthread_create(&(waitEc->tid),NULL,waitStage,(void *)waitEc);
        pthread_cond_timedwait(&(self->cond_mutex),&(self->mutex),&(ts));
        if(self->status != ATP){
            pthread_mutex_unlock(&(self->mutex));
            printf("%s didn't get a stage and left\n",self->name);
            return NULL;
        }
        getStage(self->type,self);
        if(self->type == TYPE_ACOUSTIC){
            printf("%s starting performance on acoustic stage %d\n",self->name,self->stage_id);
        }
        else{
            printf("%s starting performance on electric stage %d\n",self->name,self->stage_id);
        }
        sleep(self->play_time);
        if(sem_trywait(&singer)==0){
            self->status = WTT;
            pthread_mutex_lock(&(STAGE[self->stage_id]->mutex));
            STAGE[self->stage_id]->status = FR;
            if(self->type == TYPE_ACOUSTIC){
                printf("%s performance done on acoustic stage %d\n",self->name,self->stage_id);
                sem_post(&acoustic);
            }
            else{
                printf("%s performance done on electric stage %d\n",self->name,self->stage_id);
                sem_post(&electric);
            }
            pthread_mutex_unlock(&(STAGE[self->stage_id]->mutex));
        }
        else{
            handleSinger(self);
        }
        pthread_mutex_unlock(&(self->mutex));
    }
    else{ // singer
        pthread_mutex_lock(&(self->mutex));
        waiter * waitAc;
        waitAc = malloc(sizeof(waiter));
        waitAc->type = TYPE_ACOUSTIC;
        waitAc->player = self;
        waiter * waitEc;
        waitEc = malloc(sizeof(waiter));
        waitEc->type = TYPE_ELECTRIC;
        waitEc->player = self;
        waiter * waitS;
        waitS = malloc(sizeof(waiter));
        waitS->type = TYPE_SINGER;
        waitS->player = self;
        pthread_create(&(waitAc->tid),NULL,waitStage,(void *)waitAc);
        pthread_create(&(waitEc->tid),NULL,waitStage,(void *)waitEc);
        pthread_create(&(waitS->tid),NULL,waitStage,(void *)waitS);
        pthread_cond_timedwait(&(self->cond_mutex),&(self->mutex),&(ts));
        if(self->status==WTP){
            pthread_mutex_unlock(&(self->mutex));
            printf("%s didn't get a stage and left\n",self->name);
            return NULL;
        }
        if(self->type!=TYPE_SINGER){
            getStage(self->type,self);
            sleep(self->play_time);
            if(self->type == TYPE_ACOUSTIC){
                printf("%s performance done on acoustic stage %d\n",self->name,self->stage_id);
                sem_post(&acoustic);
            }
            else{
                printf("%s performance done on electric stage %d\n",self->name,self->stage_id);
                sem_post(&electric);
            }
        }
        else{
            pthread_cond_wait(&self->cond_mutex, &self->mutex);
        }
        pthread_mutex_unlock(&(self->mutex));
    }
    sem_wait(&coordinate);
    // printf("%s collecting T-shirt\n",self->name);
    sleep(2);
    self->status = TSC;
    printf("%s collected T-shirt\n",self->name);
    sem_post(&coordinate);
    self->status =Ex;
}


int main(){
    printf("Enter k,a,e,c,t1,t2,t\n");
    scanf("%d %d %d %d %d %d %d",&k,&a,&e,&c,&t1,&t2,&t);

    sem_init(&acoustic,0,a);
    sem_init(&electric,0,e);
    sem_init(&coordinate,0,c);

    STAGE = (stage **)malloc(sizeof(stage *) * (a+e));
    MUSICIAN = (musician **)malloc(sizeof(musician *) * (k));

    for(int i=0;i<a+e;i++){
        STAGE[i] = malloc(sizeof(stage));
        if(i<a)STAGE[i]->type = TYPE_ACOUSTIC;
        else STAGE[i]->type = TYPE_ELECTRIC;

        STAGE[i]->id = i;
        STAGE[i]->status = FR;
        pthread_mutex_init(&(STAGE[i]->mutex),NULL);
    }
    printf("Enter the musician details--name,ins,arrtime\n");
    for(int i=0;i<k;i++){
        MUSICIAN[i] = malloc(sizeof(musician));
        scanf("%s",MUSICIAN[i]->name);
        char ins[2];
        scanf("%s",ins);
        if(ins[0] == 'g' || ins[0] == 'p') MUSICIAN[i]->type = TYPE_BOTH;
        else if(ins[0] == 'v')MUSICIAN[i]->type = TYPE_ACOUSTIC;
        else if(ins[0] == 'b')MUSICIAN[i]->type = TYPE_ELECTRIC;
        else MUSICIAN[i]->type = TYPE_SINGER;

        MUSICIAN[i]->status = NYA;
        MUSICIAN[i]->id = i;
        MUSICIAN[i]->play_time = rand()%(t2-t1+1)+t1;
        pthread_mutex_init(&(MUSICIAN[i]->mutex),NULL);
        pthread_cond_init(&(MUSICIAN[i]->cond_mutex),NULL);
        scanf("%d",&(MUSICIAN[i]->arrtime));
    }
    for(int i=0;i<k;i++){
        pthread_create(&(MUSICIAN[i]->tid),NULL,musician_procedure,(void *)MUSICIAN[i]);
    }

    for(int i=0;i<k;i++){
        pthread_join((MUSICIAN[i]->tid),NULL);
    }
    return 0;
}