/* C program for Merge Sort */
#define _POSIX_C_SOURCE 199309L //required for clock
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

char* RED =  "\e[0;91m";
char* GREEN =  "\e[0;92m";
char * RESET =  "\e[0m";
char* YELLOW = "\033[1;93m";
char* BLUE = "\033[1;94m";
char* MAGENTA = "\033[1;95m";
char* CYAN = "\033[1;96m";

# define STATE_VACCINATING 0
# define STATE_ALLOTING 1

# define STATE_AVAILABLE 1
# define STATE_UNAVAILABLE 0


int n,m,o;

int REM_STUDENTS;
pthread_mutex_t rem_students;

typedef struct company{
int id;
pthread_t tid;
int rem_vz;
int batches;
float curr_prob;
pthread_mutex_t mutex;
int state;
int perbatch;
}company;

typedef struct student{
int id;
pthread_t tid;
int round;
}student;

typedef struct vz{
int id;
pthread_t tid;
int rem_vacc;
int slots;
int* alloted_students;
pthread_mutex_t mutex;
int state;
float curr_prob;
}vz;

company ** COMPANY;
student ** STUDENT;
vz ** VZ;