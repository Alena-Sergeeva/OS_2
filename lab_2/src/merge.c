#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

typedef struct
{
    int *array;
    int left;
    int right;
    int cnt;
} MergeSortArgs;

pthread_mutex_t mutex; // Мьютекс для синхронизации
int thread_count = 0;  // Количество активных потоков

void merge(int *array, int left, int mid, int right)
{
    int i, j, k;
    int n1 = mid - left + 1;
    int n2 = right - mid;

    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    for (i = 0; i < n1; i++)
        L[i] = array[left + i];
    for (j = 0; j < n2; j++)
        R[j] = array[mid + 1 + j];

    i = 0;
    j = 0;
    k = left;

    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
            array[k++] = L[i++];
        else
            array[k++] = R[j++];
    }

    while (i < n1)
        array[k++] = L[i++];
    while (j < n2)
        array[k++] = R[j++];

    free(L);
    free(R);
}

void *merge_sort(void *args)
{
    MergeSortArgs *ms_args = (MergeSortArgs *)args;
    int left = ms_args->left;
    int right = ms_args->right;
    int mid;
    int cnt = ms_args->cnt;

    pthread_mutex_lock(&mutex);
    thread_count += 2; // Увеличение счетчика активных потоков
    pthread_mutex_unlock(&mutex);

    if (left < right)
    {
        mid = left + (right - left) / 2;

        MergeSortArgs left_args = {ms_args->array, left, mid, cnt};
        MergeSortArgs right_args = {ms_args->array, mid + 1, right, cnt};

        pthread_t thread1, thread2;

        if (thread_count < cnt)
        {
            pthread_create(&thread1, NULL, merge_sort, &left_args);
            pthread_create(&thread2, NULL, merge_sort, &right_args);

            pthread_join(thread1, NULL);
            pthread_join(thread2, NULL);
        }

        merge(ms_args->array, left, mid, right);
    }

    pthread_mutex_lock(&mutex);
    --thread_count; // Уменьшение счетчика активных потоков
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <array_size> <num_threads>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);

    int *arr = (int *)malloc(n * sizeof(int));
    srand(100);
    for (int i = 0; i < n; i++)
    {
        arr[i] = rand() % 100;
    }

    pthread_mutex_init(&mutex, NULL);
    MergeSortArgs args = {arr, 0, n - 1, num_threads};

    clock_t start_time = clock();

    merge_sort(&args);

    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;
    ++thread_count;
    printf("The value of the requested threads: %d\n", num_threads);
    printf("The total number of threads created: %d\n", thread_count);
    printf("Elapsed time: %f seconds\n", time_spent);

    free(arr);

    pthread_mutex_destroy(&mutex);

    return 0;
}
