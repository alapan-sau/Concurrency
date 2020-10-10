#include "header.h"


int min(int a, int b){
    if(a<b)return a;
    return b;
}

int selectCOMPANY(){
    while(1){
        for(int i=0;i<n;i++){
            if(COMPANY[i]->state==STATE_AVAILABLE && COMPANY[i]->batches > 0){
                return i;
            }
        }
    }
}

company* allotCOMPANY(){
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



void * vz_procedure(void * temp){
    vz * self = (vz *)temp;
    company* alloted_company;
    while(1){
        // taking batch from company
        alloted_company = allotCOMPANY();
        self->curr_prob = alloted_company->curr_prob;
        self->rem_vacc = alloted_company->perbatch;
        printf("Pharmaceutical Company %d has delivered vaccines to Vaccination zone %d, resuming vaccinations now\n",alloted_company->id,self->id);

        // giving students vaccine
        while(1){
            while(REM_STUDENTS==0);
            pthread_mutex_lock(&(self->mutex));
            self->state = STATE_ALLOTING;
            pthread_mutex_lock(&rem_students);
            int k = min(REM_STUDENTS, self->rem_vacc);
            k = min(k,8);
            REM_STUDENTS-=k;
            pthread_mutex_unlock(&rem_students);
            if(k==0)
            {
                continue;
            }
            self->slots = k; // this k is minimum of rem stud, rem vac, maximum_lim
            printf("Vaccination Zone %d is ready to vaccinate with %d slots.\n",self->id,k);
            pthread_mutex_unlock(&(self->mutex));

            while(self->slots > 0){
            } // locked untill all are alloted;

            pthread_mutex_lock(&(self->mutex));
            self->state = STATE_VACCINATING;
            printf("Vaccination Zone %d entering Vaccination Phase\n",self->id);

            // wait for everyone to be vaccinated
            for(int i=0;i<o;i++){
                while(self->alloted_students[i]==1);
            }

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
    sleep(2);
}

void * company_procedure(void * temp){
    company* self = (company*)temp;
    delay_random();
    while(1){
        pthread_mutex_lock(&(self->mutex));
        self->batches = rand() % 5 + 1;
        self->perbatch = rand() % 10 + 10;
        pthread_mutex_unlock(&(self->mutex));
        self->state = STATE_AVAILABLE;
        printf("Pharmaceutical Company %d has prepared %d batches of vaccines which have success probability %f.Waiting for all the vaccines to be used to resume production.\n",self->id,self->batches,self->curr_prob);

        while(self->rem_vz > 0 || self->batches > 0);    // wait for all the vzs to be done
        pthread_mutex_lock(&(self->mutex));
        self->state = STATE_UNAVAILABLE;
        pthread_mutex_unlock(&(self->mutex));
    }
}


// arrives -> selects VZ -> VZ alloted -> wait -> VZ reached+vaccinated -> test -> go back / goes out;

int selectVZ(){
    while(1){
        for(int i=0;i<m;i++){
            if(VZ[i]->state==STATE_ALLOTING && VZ[i]->slots > 0){
                return i;
            }
        }
    }
}

vz *allotVZ(student* self){
    while(1){
        vz* selected_vz = VZ[selectVZ()];
        pthread_mutex_lock(&(selected_vz->mutex));
        if(selected_vz->state == STATE_ALLOTING && selected_vz->slots > 0){
            vz * alloted_vz = selected_vz;
            alloted_vz->slots--;
            alloted_vz->alloted_students[self->id]=1;
            pthread_mutex_unlock(&(selected_vz->mutex));
            return alloted_vz;
        }
        else{
            pthread_mutex_unlock(&(selected_vz->mutex));
        }
    }
}

void waitVZ(vz *alloted_vz){
    while(alloted_vz->state!=STATE_VACCINATING);
    return;
}

int test(vz *alloted_vz,student *self){
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

    student* self = (student *)temp;

    printf("Student %d has arrived for his %dth round of Vaccination\n",self->id,self->round);
    printf("Student %d is waiting to be allocated a slot on a Vaccination Zone\n",self->id);

    vz* alloted_vz  = allotVZ(self);
    printf("Student %d assigned a slot on the Vaccination Zone %d and waiting to be vaccinated\n",self->id,alloted_vz->id);
    waitVZ(alloted_vz);

    printf("Student %d on Vaccination Zone %d has been vaccinated which has success probability %f.\n",self->id,alloted_vz->id,alloted_vz->curr_prob);
    alloted_vz->alloted_students[self->id]=0;    // loop of vz cooresponding to this student stopped

    int result = test(alloted_vz,self);
    if(result){
        printf("Student %d has tested ‘positive’ for antibodies.\n",self->id);
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
        student_procedure(self);
        return NULL;
    }
}

int company_init(int i,float prob){
    COMPANY[i]->batches=0;
    COMPANY[i]->curr_prob = prob;
    COMPANY[i]->id=i;
    pthread_mutex_init(&(COMPANY[i]->mutex),NULL);
    COMPANY[i]->state=-1;
    COMPANY[i]->rem_vz=0;
    COMPANY[i]->perbatch=0;
}

int student_init(int i){
    STUDENT[i]->id = i;
    STUDENT[i]->round=1;
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
}

int main(){
    scanf("%d %d %d",&n,&m,&o);
    float prob[n];
    //printf("%d %d %d here\n",m,n,o);
    for(int i=0;i<n;i++){
        scanf("%f",&prob[i]);
    }
    // printf("there\n");
    //printf("inputs taken\n");
    VZ = (vz**)malloc(sizeof(vz*) * m);
    COMPANY = (company**)malloc(sizeof(company*) * n);
    STUDENT = (student**)malloc(sizeof(student*) * o);

    REM_STUDENTS = o;

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
        pthread_mutex_lock(&rem_students);
        pthread_mutex_unlock(&rem_students);
    }

    printf("Simulation Done.\n");

    for(int i=0; i<n;i++){
        pthread_mutex_destroy(&(COMPANY[i]->mutex));
    }
    for(int i=0; i<m;i++) {
        pthread_mutex_destroy(&(VZ[i]->mutex));
    }
}