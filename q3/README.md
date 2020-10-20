# Musical Mayhem

This is a multi threaded simulation of musicians performing at a college event. There are acoustic and electric stages. Musicians can perform on one or both of them. Singers can perform solo, or join other musician. The simulation is implemented as follows.

## Implementation Details

### Musicians

Musicians are modelled as threads. Each musician has s struct to store all information regarding it as follows.

```c
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
```

If musicians are eligible for either of acoustic or electric stages, it simple waits for a semaphore related to the resource[`acoustic`, `electric`, `singer`]. Otherwise, it creates a set of two/three threads to get him/her a accoustic or electric stage or also a filled stage for a joint performance for a singer. These threads have a race condition to choose one of electric/acoustic stages. This race condition is handled by a mutex lock `race_mutex` attached to the musician along with the `status`. A function called `waitStage()` implements the race. After succesful iteration of `waitStage()`, `getStage()` sets up a stage for the performance of the current singer too.

```c
void * waitStage(void * temp){
    waiter * self = (waiter *)temp;
    musician * player = self->player;
    int type = self->type;
    struct timespec ts;
    if(clock_gettime(CLOCK_REALTIME,&ts)==-1){
        perror("gettime ");
        return NULL;
    }
    ts.tv_sec += t;
    ts.tv_nsec = 0;

    int semRet;
    if(self->type == TYPE_ACOUSTIC) semRet = sem_timedwait(&acoustic,&ts);
    else if(self->type == TYPE_ELECTRIC) semRet = sem_timedwait(&electric,&ts);
    else semRet = sem_timedwait(&singer,&ts);

    pthread_mutex_lock(&player->mutex_race); // locked

    if(player->status!=WTP){
        pthread_mutex_unlock(&player->mutex_race);
        if(semRet!=-1){ // decreased the semaphore, so increase now
            if(self->type == TYPE_ACOUSTIC) sem_post(&acoustic);
            else if(self->type == TYPE_ELECTRIC) sem_post(&electric);
            else sem_post(&singer);
        }
        return NULL;
    }
    else{
        player->status = ATP;
        if(semRet == -1)
        {
            printf(RED"%s did not get a stage and left impatiently.\n"NORMAL,player->name);
            pthread_mutex_unlock(&player->mutex_race);
            return NULL;
        }
        
    }

    pthread_mutex_unlock(&player->mutex_race);    //unlocked

    if(self->type!=TYPE_SINGER){
        getStage(type,player);
        sleep(player->play_time);
        postPerformance(player);
    }
    else getSinger(player);

    return NULL;
}

void getStage(int type, musician * player){

    for(int i=0;i<a+e;i++){
        pthread_mutex_lock(&(STAGE[i]->mutex));
        if(STAGE[i]->type==type){
            if(STAGE[i]->status==FR){
                // setting stage

                if(player->type == TYPE_SINGER)STAGE[i]->status = S;
                else STAGE[i]->status = M;
                STAGE[i]->musician = player->id;    // Note: musician == non 2nd

                // setting player(non 2nd)

                player->stage_id = i;
                player->status = PSo;

                printf(YELLOW"%s starting perfomance on %s stage %d for %d secs\n"NORMAL,player->name,STAGE[i]->type == TYPE_ACOUSTIC ? "acoustic" : "electric",player->stage_id,player->play_time);
                if(player->type != TYPE_SINGER)  sem_post(&singer);  // free semaphore for singer

                pthread_mutex_unlock(&(STAGE[i]->mutex));
                return;
            }
        }
        pthread_mutex_unlock(&(STAGE[i]->mutex));
    }
}
```

After the performance, the musician checks if any singer joined him or wants to join him. `postPerformance()` handles this situation.
```c
void postPerformance(musician * self){
    int sing_partner=-1;
    if(self->status == PSi){
        sleep(2);
        sing_partner = STAGE[self->stage_id]->singer;
    }
    else{
        int val = sem_trywait(&singer);
        if(val==-1)
        {
            sleep(2);
            sing_partner = STAGE[self->stage_id]->singer;
            
        }
    }

    if(sing_partner==-1) printf(GREEN"%s is done performing \n"NORMAL,self->name); 
    else printf(GREEN"%s and %s done performing\n"NORMAL,MUSICIAN[sing_partner]->name,self->name);

    // free the stage
    pthread_mutex_lock(&(STAGE[self->stage_id]->mutex));
    STAGE[self->stage_id]->status = FR;
    STAGE[self->stage_id]->singer = -1;   
    STAGE[self->stage_id]->musician = -1;  
    pthread_mutex_unlock(&(STAGE[self->stage_id])->mutex);


    // free the semaphores
    if(STAGE[self->stage_id]->type == TYPE_ACOUSTIC) sem_post(&acoustic);
    else sem_post(&electric);

    pthread_t tc;

    if(sing_partner != -1)
    {
        pthread_create(&tc, NULL, singerTshirt,(void*)MUSICIAN[sing_partner]);
    }

    // go to collect t-shirts!
    self->status = WTT;
    sem_wait(&coordinate);
    sleep(2);
    printf(CYAN"%s collected T-Shirt\n"NORMAL,self->name);
    self->status=TSC;
    sem_post(&coordinate);
    self->status=Ex;
    if(sing_partner!=-1)  pthread_join(tc,NULL);
}

```

### Singers

Singers are implemented by creating three threads to choose between one of the three possible ways of acquiring the stage. If a singer performs solo, the handling is similar to that of a musician, except that another singer cannot join him/her. To deal with the case of singer joining another musician, `getSinger()` is used. It finds a stage for the singer to join.

```c
void getSinger(musician * player){
    for(int i=0;i<a+e;i++){
        pthread_mutex_lock(&(STAGE[i]->mutex));

        if(STAGE[i]->status == M){  // only if a musician is playing!!

        // setting the stage
            STAGE[i]->status = MS;
            STAGE[i]->singer = player->id;

        // setting the other musician
            MUSICIAN[STAGE[i]->musician]->status= PSi;
            player->status = PSi;
            player->stage_id = i;

            printf(BLUE"Singer %s joined %s on %s stage %d. Exceeding time by 2 secs!\n"NORMAL,player->name, MUSICIAN[STAGE[i]->musician]->name,STAGE[i]->type == TYPE_ACOUSTIC ? "acoustic" : "electric",player->stage_id);
            pthread_mutex_unlock(&(STAGE[i]->mutex));
            return;

        }
        pthread_mutex_unlock(&(STAGE[i]->mutex));
    }
}
```

### Stages

Stages are just a set of resources. All information regarding stages are stored in a set of `structs`. And the pointer to these structs are serially stored in a global array.

```c
typedef struct stage{
int id;
int status;
pthread_mutex_t mutex;
int type;
int musician;
int singer;
sem_t sem;
}stage;
```

Various kinds of statuses for each of musician or stage are implemented to maintain a proper synchronisation. They are as follows -

```c
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
```

### T-shirt distribution and club coordinators

This is simply implemented as sempahore `coordinate`. Any performer waits for this semaphore to get hold of tshirt!. The cases of a singer who joined a musician collecting a T-Shirt is handled by a thread (calling the function `singerTshirt()`).

```c
typedef struct coordinator{
int id;
pthread_t tid;
pthread_mutex_t mutex;
}coordinator;


sem_wait(&coordinate);
// printf("%s collecting T-shirt\n",self->name);
sleep(2);
self->status = TSC;
printf("%s collected T-shirt\n",self->name);
sem_post(&coordinate);
```
```c
void* singerTshirt(void* temp)
{
    musician* self = (musician*) temp;
    self->status = WTT;
    sem_wait(&coordinate);
    sleep(2);
    printf(CYAN"%s collected T-Shirt\n"NORMAL,self->name);
    self->status=TSC;
    sem_post(&coordinate);
    self->status=Ex;
}
```