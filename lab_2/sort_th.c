// Отсортировать массив целых чисел при помощи TimSort
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

int thread_count = 0;

pthread_mutex_t count_mutex;

typedef struct
{
    int *arr;
    int low;
    int count;
    int dir;
    int depth;
    int max_depth;
} thread_data_t;

int GetMinrun(int n)
{
    int r = 0; /* станет 1 если среди сдвинутых битов будет хотя бы 1 ненулевой */
    while (n >= 64)
    {
        r |= n & 1;
        n >>= 1;
    }
    return n + r;
}

void compare_and_swap(int *arr, int i, int j, int dir)
{
    if ((dir == 1 && arr[i] > arr[j]) || (dir == 0 && arr[i] < arr[j]))
    {
        int temp = arr[i];
        arr[i] = arr[j];
        arr[j] = temp;
    }
}

void bitonic_merge(int *arr, int low, int count, int dir)
{
    if (count > 1)
    {
        int k = count / 2;
        for (int i = low; i < low + k; i++)
        {
            compare_and_swap(arr, i, i + k, dir);
        }
        bitonic_merge(arr, low, k, dir);
        bitonic_merge(arr, low + k, k, dir);
    }
}

void *bitonic_sort(void *arg)
{
    thread_data_t *data = (thread_data_t *)arg;
    int *arr = data->arr;
    int low = data->low;
    int count = data->count;
    int dir = data->dir;
    int depth = data->depth;
    int max_depth = data->max_depth;

    if (count > 1)
    {
        int k = count / 2;

        if (depth < max_depth)
        {
            pthread_t thread1, thread2;

            thread_data_t *left_data = malloc(sizeof(thread_data_t));
            *left_data = (thread_data_t){arr, low, k, 1, depth + 1, max_depth};

            thread_data_t *right_data = malloc(sizeof(thread_data_t));
            *right_data = (thread_data_t){arr, low + k, k, 0, depth + 1, max_depth};

            pthread_mutex_lock(&count_mutex);
            thread_count += 2;
            pthread_mutex_unlock(&count_mutex);

            pthread_create(&thread1, NULL, bitonic_sort, left_data);
            pthread_create(&thread2, NULL, bitonic_sort, right_data);

            pthread_join(thread1, NULL);
            pthread_join(thread2, NULL);

            free(left_data);
            free(right_data);
        }
        else
        {
            thread_data_t left_data = {arr, low, k, 1, depth + 1, max_depth};
            thread_data_t right_data = {arr, low + k, k, 0, depth + 1, max_depth};

            bitonic_sort(&left_data);
            bitonic_sort(&right_data);
        }

        bitonic_merge(arr, low, count, dir);
    }

    return NULL;
}

int compute_max_depth(int num_threads)
{
    int max_depth = 0;
    int total_threads = 0;
    while (1)
    {
        total_threads = (1 << (max_depth + 1)) - 1;
        if (total_threads > num_threads)
        {
            return max_depth - 1;
        }
        max_depth++;
    }
}

void sort(int *arr, int n, int num_threads)
{
    pthread_t thread;

    int max_depth = compute_max_depth(num_threads);

    pthread_mutex_lock(&count_mutex);
    thread_count++;
    pthread_mutex_unlock(&count_mutex);

    thread_data_t *data = malloc(sizeof(thread_data_t));
    *data = (thread_data_t){arr, 0, n, 1, 0, max_depth};

    pthread_create(&thread, NULL, bitonic_sort, data);
    pthread_join(thread, NULL);

    free(data);
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

    if (n & (n - 1))
    {
        printf("The array size is set incorrectly, it must be a power of 2\n");
        return 1;
    }

    int *arr = (int *)malloc(n * sizeof(int));
    int seed = 1000;
    for (int i = 0; i < n; i++)
    {
        arr[i] = rand_r(&seed) % 100;
    }

    pthread_mutex_init(&count_mutex, NULL);

    clock_t start_time = clock();

    sort(arr, n, num_threads);

    clock_t end_time = clock();
    double time_spent = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("The value of the requested threads: %d\n", num_threads);
    printf("The total number of threads created: %d\n", thread_count);
    printf("Elapsed time: %.15f seconds\n", time_spent);

    free(arr);

    pthread_mutex_destroy(&count_mutex);

    return 0;
}