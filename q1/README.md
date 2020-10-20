# Merge Sort

## Introduction

Merge Sort is a Divide and Conquer Sorting Algorithm. The Merge Sort works by dividing the task of sorting an array into two subtasks of sorting halves of the array, and merge them. A pivot is chosen at the median positioned element of the array, all elements to the left and right of it are served to the algorithm to be sorted recursively and merged back. An attempt to parallelize the algorithm and introduce concurrency, using multiple processing cores by creating multiple processes and threads is made.<br>

## Implementation

### Selection Sort

We introduce a Selection Sort snippet to all of the above mentioned parallelised, multi-threaded and single-threaded classical Merge Sort algorithms when `n<5`. This modification is to achieve the a diminished overhead for creation of too many child process, and threads. The following code snippet is used to implement the Selection Sort.<br>

```c
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
```
The pointer to the array along with its left and right indices are used to refer to the section of array to be sorted using the Selection Sort.<br>

### The Merge function

After sorting each half of the array recursively, it merges both the left and right sorted halves into a single sorted array.<br>

```c
void merge(int *arr, int l, int m, int r)
{
	int i, j, k;
	int n1 = m - l + 1;
	int n2 = r - m;

	int L[n1], R[n2];

	for (i = 0; i < n1; i++) L[i] = arr[l + i];
	for (j = 0; j < n2; j++) R[j] = arr[m + 1 + j];

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
```
This Merge Function is used in all the three approaches.<br>

### Normal Recursive Merge Sort

This function recursively divides the array into left and right subarrays based on a median positioned pivot and calls the `merge()` function. Generally the base of this process is a single valued array, which is trivially sorted, however, here we take a Selection Sort approach as soon as `n<5`<br>

```c
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
```

### Multi-process Merge Sort

This function creates a child process for each of the two subarrays to be sorted. The parent, then calls the `merge()` function to merge them into a single sorted array. So the recursive sortings are parallelised on multiple processing cores.
```c
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
```
The processes do not share any memory. To overcome this problem, a shared memory has been created as follows.<br>

```c
int * shareMem(size_t size){
     key_t mem_key = IPC_PRIVATE;
     int shm_id = shmget(mem_key, size, IPC_CREAT | 0666);
     return (int*)shmat(shm_id, NULL, 0);
}
```

### Multi-threaded Merge Sort

Similarly, this process parallelizes, and introduce concurrency by sorting each subarray in a new thread instead of a process. The threads of a program share the memory, so we just simply use the array as a global declaration.

```c
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
```

## Observations

The three approches of merge sort were run on arrays of various `array lengths` leading to following results <br>

```
For 10 elements:
Normal merge sort                      : 0.000003
Merge sort by creating child processes : 0.001523
Threaded merge sort                    : 0.000225

Normal merge sort ran:
	 	 	 [75.000155] times faster than threaded merge sort
	 	 	 [507.667731] times faster than child process merge sort
```

```
For 100 elements:
Normal merge sort                      : 0.000014
Merge sort by creating child processes : 0.008176
Threaded merge sort                    : 0.001233

Normal merge sort ran:
	 	 	 [88.071306] times faster than threaded merge sort
	 	 	 [583.999195] times faster than child process merge sort
```

```
For 1000 elements:
Normal merge sort                      : 0.000074
Merge sort by creating child processes : 0.057276
Threaded merge sort                    : 0.010184

Normal merge sort ran:
	 	 	 [137.621656] times faster than threaded merge sort
	 	 	 [774.000195] times faster than child process merge sort
```

```
For 100000 elements:
Normal merge sort                      : 0.016208
Merge sort by creating child processes : 3.316962
Threaded merge sort                    : 0.620638

Normal merge sort ran:
	 	 	 [38.291725] times faster than threaded merge sort
	 	 	 [204.647922] times faster than child process merge sort

```

Now, we vary the number of elements sorted using the `Selection Sort` along the three approaches of merge sort which leads to the following results<br>

```
SELECTION SORT APPLIED TO n < 5
For 1000 elements:
Normal merge sort                      : 0.000074
Merge sort by creating child processes : 0.057349
Threaded merge sort                    : 0.009298

Normal merge sort ran:
	 	 	 [125.648631] times faster than threaded merge sort
	 	 	 [774.986377] times faster than child process merge sort
```

```
SELECTION SORT APPLIED TO n < 100
For 1000 elements:
Normal merge sort                      : 0.000171
Merge sort by creating child processes : 0.007918
Threaded merge sort                    : 0.000802

Normal merge sort ran:
	 	 	 [4.690058] times faster than threaded merge sort
	 	 	 [46.304088] times faster than child process merge sort
```

```
SELECTION SORT APPLIED TO n < 5000
For 100000 elements:
Normal merge sort                      : 0.496192
Merge sort by creating child processes : 0.154205
Threaded merge sort                    : 0.143037

Normal merge sort ran:
	 	 	 [0.288270] times faster than threaded merge sort
	 	 	 [0.310777] times faster than child process merge sort

```

From the above observations,<br>
* For smaller values of n, the overhead of thread/process creation surpasses, the benefits of parallelization and concurrency at a large extent resulting in slower(longer) runtime.

* Even though, for the larger values of n, the benefits of parallelisation starts to become visible, it still isn't able to outweight the overhead.

* The thread creation has a remarkable lesser overhead than process because of sharable memory among threads, unlike processes.

* By increasing the extent of Selection Sort, we decrease the the overhead for threads/process creation remarkably, even resulting to a faster runtime than the normal merge sort at very high values.