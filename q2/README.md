# Back To College

## Introduction

This is a multi-threaded simulation of a vaccination process involving a proper synchronisation among various systems like supply from Pharmaceutical Companies to Vaccination Zones, vaccination of students at these zones etc involving concurrency and parallelisation. The various threads use mutex locks and busy-waitings to communicate and synchronize and simulate the process coherently.

## Implementation

The simulation consists of three components namely, Pharmaceutical Companies, Vaccination Zones and Students (of multiple instances), each of which corresponds to a thread in the virtual simulation.

The information regarding every instance of these components are stored in dynamically allocated `structs`. The pointer to these structs are also serially stored in globally declared arrays to use and pass on to various threads and functions at ease.

### Pharmaceutical Companies

```c
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

company ** COMPANY;
```
The information regarding each Pharma Company is stored into a `struct` type `company`. Also, pointers to these structs are serially stored in a global array `COMPANY`.<br>
The Pharma Company makes `batches` number of vaccine batch, each containing `perbatch` vaccines having a probability of `curr_prob` in some random amount of time. The company(`state == STATUS_AVAILABLE`) then delivers all of its batches to the Vaccination Zones and `busy waits` untill all the prepared vaccines are consumed by students.<br>

```c
void * company_procedure(void * temp){      // the company thread
    company* self = (company*)temp;
    while(1){
        printf("%sPharmaceutical Company %d preparing vaccines%s\n",YELLOW,self->id,RESET);
        delay_random();
        pthread_mutex_lock(&(self->mutex));
        self->batches = rand() % 5 + 1;     // preparing batches
        self->perbatch = rand() % 10 + 10;  // preparing vaccines per batch
        printf("%sPharmaceutical Company %d has prepared %d batches of vaccines having %d vaccines in each batch which have success probability %0.2f.Waiting for all the vaccines to be used to resume production.%s\n",GREEN,self->id,self->batches,self->perbatch,self->curr_prob,RESET);
        self->state = STATE_AVAILABLE;      // allows the vz's to take one batch each
        pthread_mutex_unlock(&(self->mutex));


        // wait for all the batches to be taken and all vzs to be done using them!
        while(self->rem_vz > 0 || self->batches > 0);
        printf("%sAll vaccines made by Pharma Company %d are used %s",GREEN,self->id,RESET);
        // all vaccines used, change state to unavailable
        self->state = STATE_UNAVAILABLE;
    }
}
```
NOTE : `while(self->rem_vz > 0 || self->batches > 0);` is a busy waiting to wait untill all vaccines are consumed<br><br>

```c
int selectCOMPANY(){                // vc chooses a company to get alloted
    while(1){
        for(int i=0;i<n;i++){
            if(COMPANY[i]->state==STATE_AVAILABLE && COMPANY[i]->batches>0){
                return i;
            }
        }
    }
}

company* allotCOMPANY(){            // vz gets alloted to a company
    while(1){
        company* selected_company = COMPANY[selectCOMPANY()];
        pthread_mutex_lock(&(selected_company->mutex));
        if(selected_company->batches>0 && selected_company->state==STATE_AVAILABLE){
            company * alloted_company = selected_company;
            alloted_company->batches--;
            alloted_company->rem_vz++;
            pthread_mutex_unlock(&(selected_company->mutex));
            return alloted_company;
        }
        else{
            pthread_mutex_unlock(&(selected_company->mutex));
        }
    }
}
```
The Vaccine batches are dispatched and sent to Vaccination Zones by the function `allotCOMPANY()` which chooses a serving Pharma Company by calling the `selectCOMPANY()` function corresponing to each Vaccination Zone when running out of vaccines.

### Vaccination Zones
```c
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

vz ** VZ
```
After getting some batches of vaccines from a pharmaceutical company, it tries to open some slots (`state == STATE_ALLOTING`) for the students and wait for them to get alloted by using a busy waiting.
```c
void * vz_procedure(void * temp){                       // vz thread
    vz * self = (vz *)temp;
    company* alloted_company;
    while(1){
        // taking batch from company
        alloted_company = allotCOMPANY();
        self->curr_prob = alloted_company->curr_prob;   // probability
        self->rem_vacc = alloted_company->perbatch;     // # of vaccines
        printf("%sPharmaceutical Company %d has delivered %d vaccines to Vaccination zone %d, starting to allot students here at this zone now%s\n",CYAN,alloted_company->id,alloted_company->perbatch,self->id,RESET);

        // giving students vaccine
        while(1){
            // while(REM_STUDENTS==0);
            pthread_mutex_lock(&rem_students);
            int k = min(REM_STUDENTS, self->rem_vacc);
            k = min(k,8);
            REM_STUDENTS-=k;
            pthread_mutex_unlock(&rem_students);

            pthread_mutex_lock(&(self->mutex));
            self->state = STATE_ALLOTING;
            if(k==0){
                pthread_mutex_unlock(&(self->mutex));
                continue;   // if k==0, dont allot!
            }
            self->slots = k;
            self->rem_vacc-=k;
            // this k is minimum of rem_students, rem_vacc, maximum_limit allowed
            printf("%sVaccination Zone %d is ready to vaccinate with %d slots.%s\n",YELLOW,self->id,k,RESET);

            pthread_mutex_unlock(&(self->mutex));

            // wait untill every slot is alloted;
            while(self->slots > 0);
            // waiting stopped => all slots alloted


            pthread_mutex_lock(&(self->mutex));
            printf("%sVaccination Zone %d entering Vaccination Phase%s\n",GREEN,self->id,RESET);
            self->state = STATE_VACCINATING;

            // wait for all alloted students are vaccinated
            for(int i=0;i<o;i++){
                while(self->alloted_students[i]==1);
            }
            // waiting stopped => all are vaccinated

            printf("%sVaccination Zone %d exiting Vaccination Phase%s\n",RED,self->id,RESET);
            pthread_mutex_unlock(&(self->mutex));

            if(self->rem_vacc == 0){    // if vaccines ended, go to allot to a company again
                printf("%sVaccination Zone %d has run out of vaccines%s\n",RED,self->id,RESET);
                pthread_mutex_lock(&(alloted_company->mutex));
                alloted_company->rem_vz--;
                pthread_mutex_unlock(&(alloted_company->mutex));
                break;
            }
        }
    }
}
```
Once all the vaccination slots are filled, it goes into `STATE_VACCINATION`. In this phase, it doesn,t allot anymore students but, vaccinate all students that were alloted in the previous allocation phase. Two busy waits and mutex locks are involved in the process to synchronise the students and the zone while vaccinating and allocating.

```c
int selectVZ(){         // select a vz to get alloted
    while(1){
        for(int i=0;i<m;i++){
            if(VZ[i]->state==STATE_ALLOTING && VZ[i]->slots > 0){
                return i;
            }
        }
    }
}

vz *allotVZ(student* self){     // allots to a vaccination zone
    while(1){
        vz* selected_vz = VZ[selectVZ()];
        pthread_mutex_lock(&(selected_vz->mutex));
        if(selected_vz->state == STATE_ALLOTING && selected_vz->slots > 0){
            vz * alloted_vz = selected_vz;
            alloted_vz->slots--;
            alloted_vz->alloted_students[self->id]=1;
            printf("%sStudent %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated%s\n",YELLOW,self->id,alloted_vz->id,RESET);
            pthread_mutex_unlock(&(selected_vz->mutex));
            return alloted_vz;
        }
        else{
            pthread_mutex_unlock(&(selected_vz->mutex));
        }
    }
}
```
The `allotVZ()` allots a student to a vaccination zone in its allocation phase. To select an available vaccination zone, it uses the `selectVZ()` function.

### Students

```c
typedef struct student{
int id;
pthread_t tid;
int round;
}student;

student ** STUDENT
```
The students attempt for a maximum of 3 rounds of vaccination untill successfully done. They wait untill all the slots of the alloted vaccination zone are filled using a busy wait `waitVZ()`. They they get vaccinated to move towards the `test()` which results into either a `positive or negative` attmept.

```c
void *student_procedure(void * temp){
    delay_random();
    while (1)
    {
        student* self = (student *)temp;

        printf("%sStudent %d has arrived for his round %d of Vaccination%s\n",CYAN,self->id,self->round,RESET);
        printf("%sStudent %d is waiting to be allocated a slot on a Vaccination Zone%s\n",YELLOW,self->id,RESET);

        vz* alloted_vz  = allotVZ(self);    // allots the vz
        waitVZ(alloted_vz);                 // waits for all allotments of alloted vz

        printf("%sStudent %d on Vaccination Zone %d has been vaccinated which has success probability %0.2f.%s\n",BLUE,self->id,alloted_vz->id,alloted_vz->curr_prob,RESET);
        alloted_vz->alloted_students[self->id]=0;  // loop of vz cooresponding to this student stopped!

        // student go to test!
        int result = test(alloted_vz,self);
        if(result){
            // tested +ve, so exit
            printf("%sStudent %d has tested 'POSITIVE' for antibodies.%s\n",GREEN,self->id,RESET);
            return NULL;
        }
        else{
            printf("%sStudent %d has tested 'NEGATIVE' for antibodies.%s\n",RED,self->id,RESET);
            if(self->round == 4)
            {
                // tested -ve for more than 3 times, so exit
                printf("%sStudent %d went home for another online semester%s\n",RED,self->id,RESET);
                return NULL;
            }
            // continue to next vaccination session
            pthread_mutex_lock(&rem_students);
            REM_STUDENTS++;
            pthread_mutex_unlock(&rem_students);
        }
    }
    return NULL;
}
```
<br>
NOTE : `REM_STUDENTS` is a mutex protected variable(rem_students) storing the remaining number of students present in the simulation.
<br>

### Sample Test Case

```
Enter n,m,k
2 3 4
Enter the corresponding probabilities of each pharma company
0.3 0.6

------------ STARTING SIMULATION ---------------
Pharmaceutical Company 1 preparing vaccines
Pharmaceutical Company 0 preparing vaccines
Student 1 has arrived for his round 1 of Vaccination
Student 1 is waiting to be allocated a slot on a Vaccination Zone
Student 2 has arrived for his round 1 of Vaccination
Student 2 is waiting to be allocated a slot on a Vaccination Zone
Pharmaceutical Company 1 has prepared 5 batches of vaccines having 18 vaccines in each batch which have success probability 0.60.Waiting for all the vaccines to be used to resume production.
Pharmaceutical Company 1 has delivered 18 vaccines to Vaccination zone 1, starting to allot students here at this zone now
Pharmaceutical Company 1 has delivered 18 vaccines to Vaccination zone 2, starting to allot students here at this zone now
Pharmaceutical Company 1 has delivered 18 vaccines to Vaccination zone 0, starting to allot students here at this zone now
Vaccination Zone 1 is ready to vaccinate with 4 slots.
Student 1 assigned a slot on the Vaccination Zone 1 and waiting to be vaccinated
Student 2 assigned a slot on the Vaccination Zone 1 and waiting to be vaccinated
Pharmaceutical Company 0 has prepared 4 batches of vaccines having 19 vaccines in each batch which have success probability 0.30.Waiting for all the vaccines to be used to resume production.
Student 0 has arrived for his round 1 of Vaccination
Student 0 is waiting to be allocated a slot on a Vaccination Zone
Student 0 assigned a slot on the Vaccination Zone 1 and waiting to be vaccinated
Student 3 has arrived for his round 1 of Vaccination
Student 3 is waiting to be allocated a slot on a Vaccination Zone
Student 3 assigned a slot on the Vaccination Zone 1 and waiting to be vaccinated
Vaccination Zone 1 entering Vaccination Phase
Student 3 on Vaccination Zone 1 has been vaccinated which has success probability 0.60.
Student 3 has tested 'POSITIVE' for antibodies.
Student 1 on Vaccination Zone 1 has been vaccinated which has success probability 0.60.
Student 0 on Vaccination Zone 1 has been vaccinated which has success probability 0.60.
Student 0 has tested 'NEGATIVE' for antibodies.
Student 0 has arrived for his round 2 of Vaccination
Student 0 is waiting to be allocated a slot on a Vaccination Zone
Student 2 on Vaccination Zone 1 has been vaccinated which has success probability 0.60.
Student 2 has tested 'POSITIVE' for antibodies.
Student 1 has tested 'POSITIVE' for antibodies.
Vaccination Zone 1 exiting Vaccination Phase
Vaccination Zone 0 is ready to vaccinate with 1 slots.
Student 0 assigned a slot on the Vaccination Zone 0 and waiting to be vaccinated
Vaccination Zone 0 entering Vaccination Phase
Student 0 on Vaccination Zone 0 has been vaccinated which has success probability 0.60.
Student 0 has tested 'POSITIVE' for antibodies.
Vaccination Zone 0 exiting Vaccination Phase


-------------ENDING SIMULATION ------------------
```