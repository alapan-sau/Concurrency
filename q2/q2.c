#include "header.h"


int min(int a, int b){              // Utility function to calculate min
    if(a<b)return a;
    return b;
}

////////////////////////// company ////////////////////////////

void delay_random(){
    int t = rand() % 3  + 2;
    sleep(t);
}

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



//////////////////////////////// vz ////////////////////////////////////////

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


///////////////////////////////////// students ///////////////////////////////

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

void waitVZ(vz *alloted_vz){
    // wait for the vz to start vaccinating after all allocations are done
    while(alloted_vz->state!=STATE_VACCINATING);
    return;
}

int test(vz *alloted_vz,student *self){         // test results based on probability of the vaccine taken
    float test = (float)rand()/(float)RAND_MAX;
    if(test < alloted_vz->curr_prob){
        return 1;
    }
    else{
        self->round++;
        return 0;
    }
}

void *student_procedure(void * temp){
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

int company_init(int i,float prob){
    COMPANY[i]->batches=0;
    COMPANY[i]->curr_prob = prob;
    COMPANY[i]->id=i;
    pthread_mutex_init(&(COMPANY[i]->mutex),NULL);
    COMPANY[i]->state=-1;
    COMPANY[i]->rem_vz=0;
    COMPANY[i]->perbatch=0;
    return 0;
}

int student_init(int i){
    STUDENT[i]->id = i;
    STUDENT[i]->round=1;
    return 0;
}

int vz_init(int i){
    VZ[i]->alloted_students = (int*)malloc(o * sizeof(int));
    for(int q=0;q<o;q++)VZ[i]->alloted_students[q]=0;
    VZ[i]->curr_prob= 0;
    pthread_mutex_init(&(VZ[i]->mutex),NULL);
    VZ[i]->rem_vacc=0;
    VZ[i]->slots=0;
    VZ[i]->state=-1;
    VZ[i]->id = i;
    return 0;
}


/////////////////////////////////////////////////////////////////////////


int main(){

    printf("Enter n,m,k\n");
    scanf("%d %d %d",&n,&m,&o);
    float prob[n];
    printf("Enter the corresponding probabilities of each pharma company\n");
    for(int i=0;i<n;i++){
        scanf("%f",&prob[i]);
    }

    if(n==0 || m==0 || o==0){
        printf("Cant vaccinate!\n");
        return 0;
    }

    printf("\n-----------------------------------------------------STARTING SIMULATION---------------------------------------------------------\n");

    // arrays for the struct pointers(thread specific)
    VZ = (vz**)malloc(sizeof(vz*) * m);
    COMPANY = (company**)malloc(sizeof(company*) * n);
    STUDENT = (student**)malloc(sizeof(student*) * o);

    // Initialise REM_STUDENTS and corresponding mutex
    REM_STUDENTS = o;
    pthread_mutex_init(&(rem_students),NULL);


    // starting company threads
    for (int i=0;i<n;i++){
        COMPANY[i] = malloc(sizeof(company));
        company_init(i,prob[i]);
        pthread_create(&(COMPANY[i]->tid), NULL,company_procedure, (void*)COMPANY[i]);
    }


    // starting vaccination zones threads
    for (int i=0;i<m;i++){
        VZ[i] = (vz*)malloc(sizeof(vz));
        vz_init(i);
        pthread_create(&(VZ[i]->tid), NULL,vz_procedure, (void*)VZ[i]);
    }


    // starting students threads
    for (int i=0;i<o;i++){
        STUDENT[i] = (student*)malloc(sizeof(student));
        student_init(i);
        pthread_create(&(STUDENT[i]->tid), NULL,student_procedure, (void*)STUDENT[i]);
    }
    //printf("all threads initiated\n");


    // waiting for all students threads to join
    for (int i = 0; i<o; i++){
        pthread_join(STUDENT[i]->tid, NULL);
    }

    // delete all company mutexes
    for(int i=0; i<n;i++){
        pthread_mutex_destroy(&(COMPANY[i]->mutex));
    }
    // delete all vz mutexes
    for(int i=0; i<m;i++) {
        pthread_mutex_destroy(&(VZ[i]->mutex));
    }

    printf("\n\n-----------------------------------------------------ENDING SIMULATION---------------------------------------------------------\n");
    return 0;
}