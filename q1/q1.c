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

typedef struct s{
	int left;
	int right;
	int* arr;
}s;

int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}

void merge(int *arr, int l, int m, int r)
{
	int i, j, k;
	int n1 = m - l + 1;
	int n2 = r - m;


	int L[n1], R[n2];


	for (i = 0; i < n1; i++)
		L[i] = arr[l + i];
	for (j = 0; j < n2; j++)
		R[j] = arr[m + 1 + j];


	i = 0;
	j = 0;
	k = l;
	while (i < n1 && j < n2) {
		if (L[i] <= R[j]) {
			arr[k] = L[i];
			i++;
		}
		else {
			arr[k] = R[j];
			j++;
		}
		k++;
	}

	while (i < n1) {
		arr[k] = L[i];
		i++;
		k++;
	}

	while (j < n2) {
		arr[k] = R[j];
		j++;
		k++;
	}
	return;
}
void selectionSort(int *arr,int l, int r){
    int i, j, min_idx;
    for(i = l; i <=r; i++){
        // Find the minimum element
        min_idx = i;
        for (j=i+1; j<=r; j++)
        	if (arr[j] < arr[min_idx])
            	min_idx = j;
		// swap
		int temp = arr[min_idx];
		arr[min_idx] = arr[i];
		arr[i] = temp;
    }
	return;
}
void mergeSort(int *arr, int l, int r)
{
	if (r-l > 5){
		int m = l + (r - l) / 2;
		mergeSort(arr, l, m);
		mergeSort(arr, m + 1, r);

		merge(arr, l, m, r);
	}
	else{
		selectionSort(arr,l,r);
	}
	return;
}

void mergeSortProcess(int *arr, int l, int r)
{
	if (r-l>5){

		int m = l + (r - l) / 2;

        pid_t leftChild = fork();
        if(leftChild < 0) perror("fork: ");
		// Sort left half
        if(leftChild == 0){
		    mergeSortProcess(arr, l, m);
            exit(0);
        }

        pid_t rightChild = fork();
        if(rightChild < 0) perror("fork: ");
        // Sort right half
        if(rightChild == 0){
		    mergeSortProcess(arr, m + 1, r);
            exit(0);
        }

        // merge them in parent process!!
        if((leftChild > 0) && (rightChild > 0)){
            waitpid(leftChild,NULL,WUNTRACED);
            waitpid(rightChild,NULL,WUNTRACED);
            merge(arr, l, m, r);
        }
	}
    else{
        selectionSort(arr,l,r);
		return;
    }
}

void *mergeSortThread(void * temp)
{
	s* incoming = (s*)temp;
	int l = incoming->left ;
	int r = incoming->right;
	int *arr = incoming->arr;

	if (r-l > 5){
		int m = l + (r - l) / 2;

		pthread_t tidleft;
		s* outgoingleft = (s*)malloc(sizeof(s));
		outgoingleft->left = l;
		outgoingleft->right = m;
		outgoingleft->arr = arr;
		pthread_create(&tidleft, NULL, mergeSortThread, (void*)(outgoingleft));


		pthread_t tidright;
		s* outgoingright = (s*)malloc(sizeof(s));
		outgoingright->left = m+1;
		outgoingright->right = r;
		outgoingright->arr = arr;
		pthread_create(&tidright, NULL, mergeSortThread, (void*)(outgoingright));


		pthread_join(tidleft, NULL);
		pthread_join(tidright, NULL);

        merge(arr, l, m, r);
	}
	else{
		selectionSort(arr, l, r);
	}
	return NULL;
}


void printArray(int A[], int size)
{
	int i;
	for (i = 0; i < size; i++)
		printf("%d ", A[i]);
	printf("\n");
}

int main()
{
    int n;
	printf("Enter the number of elements :\n");
    scanf("%d",&n);

    int *process_arr = shareMem(sizeof(int)*(n+1));
	printf("Enter the array elements :\n");
    for(int i=0;i<n;i++){
        scanf("%d",&process_arr[i]);
    }
	int arr_size = n;

	int normal_arr[n+1];
	for(int i =0;i<n;i++){
		normal_arr[i] = process_arr[i];
	}

	int *thread_arr = (int*)malloc((n+1)* sizeof(int));
	for(int i=0;i<n;i++){
		thread_arr[i] = process_arr[i];
	}

//////////////////////////////////////////////////
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
    long double st = ts.tv_nsec/(1e9)+ts.tv_sec;

	mergeSort(normal_arr, 0, arr_size - 1);

	clock_gettime(CLOCK_MONOTONIC, &ts);
    long double en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Normal Merge Sort\n\n");
	long double t1 = en-st;
	printArray(normal_arr, arr_size);
	printf("\n");
	printf("Time taken for Normal Merge Sort= %Lf\n\n", en - st);

//////////////////////////////////////////////////

	clock_gettime(CLOCK_MONOTONIC, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

	mergeSortProcess(process_arr, 0, arr_size - 1);

	clock_gettime(CLOCK_MONOTONIC, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Multi-process Merge Sort\n\n");
	long double t2 = en-st;
	printArray(process_arr, arr_size);
	printf("\n");
	printf("Time taken for multi-process Merge Sort = %Lf\n\n", en - st);
//////////////////////////////////////////////////////

	clock_gettime(CLOCK_MONOTONIC, &ts);
    st = ts.tv_nsec/(1e9)+ts.tv_sec;

	pthread_t tid;
	s* outgoing = (s*)malloc(sizeof(s));
	outgoing->left = 0;
	outgoing->right = arr_size-1;
	outgoing->arr = thread_arr;

	pthread_create(&tid, NULL, mergeSortThread, (void*)(outgoing));

	pthread_join(tid, NULL);

	clock_gettime(CLOCK_MONOTONIC, &ts);
    en = ts.tv_nsec/(1e9)+ts.tv_sec;
    printf("Multi-threaded Merge Sort\n\n");
	long double t3 = en-st;
	printArray(thread_arr, arr_size);
	printf("\n");
    printf("Time taken for multi-threaded Merge Sort = %Lf\n\n", en - st);
//////////////////////////////////////////////////////

	printf("For %d elements:\n",n);
    printf("Normal merge sort                      : %Lf\n",t1);
    printf("Merge sort by creating child processes : %Lf\n",t2);
    printf("Threaded merge sort                    : %Lf\n",t3);
    printf("\n");
    printf("Normal merge sort ran:\n");;
    printf("\t \t \t [%Lf] times faster than threaded merge sort\n",t3/t1);
    printf("\t \t \t [%Lf] times faster than child process merge sort\n",t2/t1);

	return 0;
}
