#include "header.h"


int min(int a, int b){              // Utility function to calculate min
    if(a<b)return a;
    return b;
}

int selectCOMPANY(){                // vc chooses a company to get alloted
    while(1){
        for(int i=0;i<n;i++){
            if(COMPANY[i]->state==STATE_AVAILABLE && COMPANY[i]->batches > 0){
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
        printf("Pharmaceutical Company %d has delivered %d vaccines to Vaccination zone %d, resuming vaccinations now\n",alloted_company->id,alloted_company->perbatch,self->id);

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
            printf("Vaccination Zone %d is ready to vaccinate with %d slots.\n",self->id,k);

            pthread_mutex_unlock(&(self->mutex));

            // locked untill every slot is alloted;
            while(self->slots > 0);

            pthread_mutex_lock(&(self->mutex));

            printf("Vaccination Zone %d entering Vaccination Phase\n",self->id);
            self->state = STATE_VACCINATING;

            // wait for all alloted students are vaccinated
            for(int i=0;i<o;i++){
                while(self->alloted_students[i]==1);
            }
            // waiting stopped => all are vaccinated
            printf("Vaccination Zone %d exiting Vaccination Phase\n",self->id);

            pthread_mutex_unlock(&(self->mutex));
            if(self->rem_vacc == 0){
                printf("Vaccination Zone %d has run out of vaccines\n",self->id);
                pthread_mutex_lock(&(alloted_company->mutex));
                alloted_company->rem_vz--;
                pthread_mutex_unlock(&(alloted_company->mutex));
                break;
            }
        }
    }
}

void delay_random(){
    int t = rand() % 3  + 2;
    sleep(t);
}

void * company_procedure(void * temp){      // the company thread
    company* self = (company*)temp;
    // delay_random();
    while(1){
        pthread_mutex_lock(&(self->mutex));
        self->batches = rand() % 5 + 1;     // preparing batches
        self->perbatch = rand() % 10 + 10;  // preparing batch vaccines
        printf("Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %f.Waiting for all the vaccines to be used to resume production.\n",self->id,self->batches,self->curr_prob);
        self->state = STATE_AVAILABLE;      // allows the vz's to take one batch each
        pthread_mutex_unlock(&(self->mutex));


        // wait for all the batches to be taken and all vzs to be done using them!
        while(self->rem_vz > 0 || self->batches > 0);

        // all vaccines used, change state to unavailable
        self->state = STATE_UNAVAILABLE;
    }
}



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
            printf("Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n",self->id,alloted_vz->id);
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

        printf("Student %d has arrived for his %dth round of Vaccination\n",self->id,self->round);
        printf("Student %d is waiting to be allocated a slot on a Vaccination Zone\n",self->id);

        vz* alloted_vz  = allotVZ(self);    // allots the vz
        waitVZ(alloted_vz);                 // waits for all allotments of alloted vz

        printf("Student %d on Vaccination Zone %d has been vaccinated which has success probability %f.\n",self->id,alloted_vz->id,alloted_vz->curr_prob);
        alloted_vz->alloted_students[self->id]=0;  // loop of vz cooresponding to this student stopped!

        // go to test!
        int result = test(alloted_vz,self);
        if(result){
            printf("Student %d has tested ‘positive’ for antibodies.\n",self->id);
            return NULL;
        }
        else{
            printf("Student %d has tested ‘negative’ for antibodies.\n",self->id);
            if(self->round == 4)
            {
                printf("Student %d went home for another online semester\n",self->id);
                return NULL;
            }

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

int main(){
    scanf("%d %d %d",&n,&m,&o);
    float prob[n];

    for(int i=0;i<n;i++){
        scanf("%f",&prob[i]);
    }
    VZ = (vz**)malloc(sizeof(vz*) * m);
    COMPANY = (company**)malloc(sizeof(company*) * n);
    STUDENT = (student**)malloc(sizeof(student*) * o);

    REM_STUDENTS = o;
    pthread_mutex_init(&(rem_students),NULL);

    for (int i=0;i<n;i++){
        COMPANY[i] = malloc(sizeof(company));
        company_init(i,prob[i]);
        pthread_create(&(COMPANY[i]->tid), NULL,company_procedure, (void*)COMPANY[i]);
    }

    for (int i=0;i<m;i++){
        VZ[i] = (vz*)malloc(sizeof(vz));
        vz_init(i);
        pthread_create(&(VZ[i]->tid), NULL,vz_procedure, (void*)VZ[i]);
    }

    for (int i=0;i<o;i++){
        STUDENT[i] = (student*)malloc(sizeof(student));
        student_init(i);
        pthread_create(&(STUDENT[i]->tid), NULL,student_procedure, (void*)STUDENT[i]);
    }
    //printf("threads initiated\n");
    for (int i = 0; i<o; i++){
        pthread_join(STUDENT[i]->tid, NULL);
    }

    printf("Simulation Done.\n");

    for(int i=0; i<n;i++){
        pthread_mutex_destroy(&(COMPANY[i]->mutex));
    }
    for(int i=0; i<m;i++) {
        pthread_mutex_destroy(&(VZ[i]->mutex));
    }
}