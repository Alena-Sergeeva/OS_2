#include "library.h"

#define MIN_BLOCK_SIZE 32

typedef struct Block
{
    size_t size;
    struct Block *next;
    bool is_free;
} Block;

typedef struct Allocator
{
    Block *free_list;
    void *memory_start;
    size_t total_size;
} Allocator;

EXPORT Allocator *allocator_create(void *memory, size_t size)
{
    if (!memory || size < sizeof(Allocator))
    {
        return NULL;
    }

    Allocator *allocator = (Allocator *)memory;
    allocator->memory_start = (char *)memory + sizeof(Allocator);
    allocator->total_size = size - sizeof(Allocator);
    allocator->free_list = (Block *)allocator->memory_start;

    allocator->free_list->size = allocator->total_size - sizeof(Block);
    allocator->free_list->next = NULL;
    allocator->free_list->is_free = true;

    return allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator)
{
    if (allocator)
    {
        memset(allocator, 0, allocator->total_size);
    }
}

EXPORT void *allocator_alloc(Allocator *allocator, size_t size)
{
    if (!allocator || size == 0)
    {
        return NULL;
    }

    size = (size + MIN_BLOCK_SIZE - 1) / MIN_BLOCK_SIZE * MIN_BLOCK_SIZE;

    Block *best = NULL;
    Block *prev_best = NULL;
    Block *current = allocator->free_list;
    Block *prev = NULL;

    while (current)
    {
        if (current->is_free && current->size >= size)
        {
            if (best == NULL || current->size < best->size)
            {
                best = current;
                prev_best = prev;
            }
        }
        prev = current;
        current = current->next;
    }

    if (best)
    {
        size_t remain_size = best->size - size;
        if (remain_size >= sizeof(Block) + MIN_BLOCK_SIZE)
        {
            Block *new_block = (Block *)((char *)best + sizeof(Block) + size);
            new_block->size = remain_size - sizeof(Block);
            new_block->is_free = true;
            new_block->next = best->next;

            best->next = new_block;
            best->size = size;
        }

        best->is_free = false;
        if (prev_best == NULL)
        {
            allocator->free_list = best->next;
        }
        else
        {
            prev_best->next = best->next;
        }

        return (void *)((char *)best + sizeof(Block));
    }

    return NULL;
}

EXPORT void allocator_free(Allocator *allocator, void *ptr_to_memory)
{
    if (!allocator || !ptr_to_memory)
    {
        return;
    }

    Block *head = (Block *)((char *)ptr_to_memory - sizeof(Block));
    if (!head)
        return;

    head->next = allocator->free_list;
    head->is_free = true;
    allocator->free_list = head;

    Block *current = allocator->free_list;
    while (current && current->next)
    {
        if (((char *)current + sizeof(Block) + current->size) == (char *)current->next)
        {
            current->size += current->next->size + sizeof(Block);
            current->next = current->next->next;
        }
        else
        {
            current = current->next;
        }
    }
}