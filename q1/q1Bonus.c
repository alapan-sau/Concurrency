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

typedef struct s{
	int left;
	int right;
	int* arr;
}s;

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
}

void *mergeSort(void * temp)
{
	s* incoming = (s*)temp;
	int l = incoming->left ;
	int r = incoming->right;
	int *arr = incoming->arr;

	if (l < r){
		int m = l + (r - l) / 2;

		pthread_t tidleft;
		s* outgoingleft = (s*)malloc(sizeof(s));
		outgoingleft->left = l;
		outgoingleft->right = m;
		outgoingleft->arr = arr;
		pthread_create(&tidleft, NULL, mergeSort, (void*)(outgoingleft));


		pthread_t tidright;
		s* outgoingright = (s*)malloc(sizeof(s));
		outgoingright->left = m+1;
		outgoingright->right = r;
		outgoingright->arr = arr;
		pthread_create(&tidright, NULL, mergeSort, (void*)(outgoingright));


		pthread_join(tidleft, NULL);
		pthread_join(tidright, NULL);

        merge(arr, l, m, r);
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
    scanf("%d",&n);

	int arr[n];
    for(int i=0;i<n;i++){
        scanf("%d",&arr[i]);
    }

	int arr_size = n;

	printf("Given array is \n");
	printArray(arr, arr_size);

	pthread_t tid;
	s* outgoing = (s*)malloc(sizeof(s));
	outgoing->left = 0;
	outgoing->right = arr_size-1;
	outgoing->arr = arr;

	pthread_create(&tid, NULL, mergeSort, (void*)(outgoing));

	pthread_join(tid, NULL);
	printf("\nSorted array is \n");
	printArray(outgoing->arr, arr_size);

	return 0;
}
