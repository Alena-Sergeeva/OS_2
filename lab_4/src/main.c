#include "library.h"

#define MEMORY_POOL_SIZE 1024

static Allocator *allocator_create_stub(void *const memory, const size_t size)
{
    const char msg[] = "allocator_create: Function not found, using mmap\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);

    void *mapped_memory = mmap(memory, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (mapped_memory == MAP_FAILED)
    {
        const char err_msg[] = "allocator_create: mmap failed\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return NULL;
    }
    return (Allocator *)mapped_memory;
}

static void allocator_destroy_stub(Allocator *const allocator)
{
    const char msg[] = "allocator_destroy: Function not found, using munmap\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);

    if (allocator)
    {
        if (munmap(allocator, MEMORY_POOL_SIZE) == -1)
        {
            const char err_msg[] = "allocator_destroy: munmap failed\n";
            write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        }
    }
}

static void *allocator_alloc_stub(Allocator *const allocator, const size_t size)
{
    const char msg[] = "allocator_alloc: Function not found, using mmap\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);

    void *mapped_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mapped_memory == MAP_FAILED)
    {
        const char err_msg[] = "allocator_alloc: mmap failed\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
        return NULL;
    }

    return mapped_memory;
}

static void allocator_free_stub(Allocator *const allocator, void *const memory)
{
    const char msg[] = "allocator_free: Function not found, using munmap\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);

    if (memory && munmap(memory, sizeof(memory)) == -1)
    {
        const char err_msg[] = "allocator_free: munmap failed\n";
        write(STDERR_FILENO, err_msg, sizeof(err_msg) - 1);
    }
}

static allocator_create_f *allocator_create;
static allocator_destroy_f *allocator_destroy;
static allocator_alloc_f *allocator_alloc;
static allocator_free_f *allocator_free;

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        const char msg[] = "Usage: ./Main <library_path>\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return EXIT_FAILURE;
    }

    void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
    argc++;
    if (argc > 2 && library)
    {
        if (!library)
        {
            const char msg[] = "Failed to load library\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            return EXIT_FAILURE;
        }

        allocator_create = dlsym(library, "allocator_create");
        allocator_destroy = dlsym(library, "allocator_destroy");
        allocator_alloc = dlsym(library, "allocator_alloc");
        allocator_free = dlsym(library, "allocator_free");

        if (!allocator_create)
        {
            allocator_create = allocator_create_stub;
        }
        if (!allocator_destroy)
        {
            allocator_destroy = allocator_destroy_stub;
        }
        if (!allocator_alloc)
        {
            allocator_alloc = allocator_alloc_stub;
        }
        if (!allocator_free)
        {
            allocator_free = allocator_free_stub;
        }
    }
    else
    {
        const char msg[] = "error: failed to open custom library\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        return EXIT_FAILURE;
    }

    // Teсты библиотеки
    size_t size = MEMORY_POOL_SIZE;
    void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (addr == MAP_FAILED)
    {
        dlclose(library);
        char message[] = "mmap failed\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        return EXIT_FAILURE;
    }

    Allocator *allocator = allocator_create(addr, MEMORY_POOL_SIZE);

    if (!allocator)
    {
        const char msg[] = "Failed to initialize allocator\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        munmap(addr, size);
        dlclose(library);

        return EXIT_FAILURE;
    }

    int *int_block = (int *)allocator_alloc(allocator, sizeof(int));
    if (int_block)
    {
        *int_block = 42;
        const char msg[] = "Allocated int_block with value 42\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }
    else
    {
        const char msg[] = "Failed to allocate memory for int_block\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    float *float_block = (float *)allocator_alloc(allocator, sizeof(float));
    if (float_block)
    {
        *float_block = 3.14f;
        const char msg[] = "Allocated float_block with value 3.14\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }
    else
    {
        const char msg[] = "Failed to allocate memory for float_block\n";
        write(STDERR_FILENO, msg, sizeof(msg));
    }

    if (int_block)
    {
        allocator_free(allocator, int_block);
        const char msg[] = "Freed int_block\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    if (float_block)
    {
        allocator_free(allocator, float_block);
        const char msg[] = "Freed float_block\n";
        write(STDOUT_FILENO, msg, sizeof(msg));
    }

    allocator_destroy(allocator);
    const char msg[] = "Allocator destroyed\n";
    write(STDOUT_FILENO, msg, sizeof(msg));

    if (library)
    {
        dlclose(library);
    }
    munmap(addr, size);

    return EXIT_SUCCESS;
}