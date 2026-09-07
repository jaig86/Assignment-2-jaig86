#include "memory.h"
#include <sys/mman.h>
#include <stddef.h>
#include <stdio.h>

typedef struct header {
    size_t size;
    struct header * prev;
    struct header * next;
    int in_use;
} m_header;

m_header* freelist = NULL;

void print_freelist() {
    m_header* curr = freelist;
    while(curr != NULL) {
        printf("[%p: size:%lu prev:%p next:%p use:%d]\n", curr, curr->size, curr->prev, curr->next, curr->in_use);
        curr = curr->next;
    }
    printf("\n --- \n");
}

void * new_malloc(size_t size) {

    size = (size + 15) & ~((size_t)15);

    if (freelist == NULL) {

        printf("MMAP\n");

        freelist = mmap(
            NULL,
            2048,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        if (freelist == MAP_FAILED) {
            printf("map failed\n");
            return NULL;
        }

        freelist->size = 2048 - sizeof(m_header);
        freelist->prev = NULL;
        freelist->next = NULL;
        freelist->in_use = 0;
    }

    m_header *curr = freelist;

    while (curr != NULL) {

        if (!curr->in_use && curr->size >= size) {
            break;
        }

        curr = curr->next;
    }

    if (curr == NULL) {
        return NULL;
    }

    if (curr->size >= size + sizeof(m_header) + 1) {

        m_header *new_block =
            (m_header *)((char *)(curr + 1) + size);

        new_block->size =
            curr->size - size - sizeof(m_header);

        new_block->in_use = 0;
        new_block->prev = curr;
        new_block->next = curr->next;

        if (curr->next != NULL) {
            curr->next->prev = new_block;
        }

        curr->next = new_block;
        curr->size = size;
    }

    curr->in_use = 1;

    return (void *)(curr + 1);
}

void new_free(void * ptr) {

    if (ptr == NULL) {
        return;
    }

    m_header *curr =
        ((m_header *)ptr) - 1;


    curr->in_use = 0;

    if (curr->next != NULL &&
        curr->next->in_use == 0) {

        m_header *next = curr->next;

        curr->size =
            curr->size +
            sizeof(m_header) +
            next->size;

        curr->next = next->next;

        if (curr->next != NULL) {
            curr->next->prev = curr;
        }
    }

    if (curr->prev != NULL &&
        curr->prev->in_use == 0) {

        m_header *prev = curr->prev;

        prev->size =
            prev->size +
            sizeof(m_header) +
            curr->size;

        prev->next = curr->next;

        if (curr->next != NULL) {
            curr->next->prev = prev;
        }
    }
}