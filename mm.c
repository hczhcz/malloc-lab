/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "your_student_id",
    /* First member's full name */
    "your_name",
    /* First member's email address */
    "your_name@sjtu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

/* check the sign mark in a size_t value */
#define SIGN_CHECK(value) (((long) (value)) < 0)

/* change the sign mark in a size_t value */
#define SIGN_MARK(value) (~(value))

/* maximum value of size_t */
#define SIZE_T_MAX ((size_t) -1)

/* aligned size of size_t */
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* aligned size of blocks' header */
#define MM_HEADER_SIZE (ALIGN(sizeof(MMHeader)))

/* the least size of an useful block */
#define USEFUL_SIZE (MM_HEADER_SIZE + ALIGNMENT)

/*
 * options of memory allocator
 */
/* if the block can fit */
//#define MM_FIT(need, size) (((size) << 3) < (need))
#define MM_FIT(need, size) (1)

/* header of a data block */
typedef struct {
    /* size of the block, apply SIGN_MARK if the block is not used */
    size_t size;
} MMHeader;

/* first free block */
void *first_free;

/* 
 * mm_put_header - Generate the header of a block and return the data section.
 */
void *mm_put_header(void *ptr, size_t size)
{
    ((MMHeader *) ptr)->size = size;
    return ptr + MM_HEADER_SIZE;
}

/* 
 * mm_restore_header - Get the header of a block from a data pointer.
 */
MMHeader *mm_restore_header(void *ptr)
{
    return (MMHeader *) (ptr - MM_HEADER_SIZE);
}

/* 
 * mm_init - Initialize the malloc package.
 */
int mm_init(void)
{
    first_free = mem_heap_lo();

    return 0;
}

/*
 * mm_print - Print block list for debug purpose.
 */
void mm_print()
{
    void *now = mem_heap_lo();
    size_t now_size = 0;

    // scan the block list
    while ((now - 1) != mem_heap_hi()) {
        now_size = ((MMHeader *) now)->size;
        printf("pos: %x - head: %x\n", (unsigned) now, now_size);

        // visit the next block
        now += SIGN_CHECK(now_size) ? SIGN_MARK(now_size) : now_size;
    }
}

/* 
 * mm_malloc - Allocate a block.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    void *now = first_free;
    void *best = NULL;
    size_t now_size = 0;
    size_t best_size = SIZE_T_MAX;
    size_t need_size = MM_HEADER_SIZE + ALIGN(size);
    size_t more_size;

    // param checking
    // if (!size) return;

    // scan the block list
    // if the best one is not found, keep best == NULL
    // if the last one is used, let now_size == 0
    while ((now - 1) != mem_heap_hi()) {
        now_size = ((MMHeader *) now)->size;
        // if (now_size) printf("%d\n", now_size);
        // printf("%x\n", first_free);

        if (SIGN_CHECK(now_size)) {
            // current block is not used

            now_size = SIGN_MARK(now_size);

            // check if this block is useable and useful
            if (now_size >= need_size && now_size < best_size) {
                best = now;
                best_size = now_size;

                if (MM_FIT(need_size, now_size)) break;
            }

            // visit the next block
            now += now_size;
        } else {
            // current block is used

            // visit the next block
            now += now_size;

            // reset now_size
            now_size = 0;
        }
    }

    // printf("%d, %d\n", now_size, need_size);

    // if there is no useable block
    if (!best) {
        // seek to the tail block
        mem_sbrk(need_size - now_size);
        best = now - now_size;
        best_size = need_size;
    } else {
        more_size = best_size - need_size;

        // if these is more space than necessary
        if (more_size >= USEFUL_SIZE) {
            mm_put_header(best + need_size, SIGN_MARK(more_size));
            best_size = need_size;
        }
        //printf("%d\n", more_size);
    }

    // change first_free
    if (best == first_free) first_free += best_size;

    return mm_put_header(best, best_size);
}

/*
 * mm_free - Free a block.
 */
void mm_free(void *ptr)
{
    MMHeader *now = mm_restore_header(ptr);

    // change first_free
    if (ptr < first_free) first_free = now;

    // mark it as an unused block
    mm_put_header(now, SIGN_MARK(now->size));
}

/*
 * mm_realloc - Change the size of a block.
 */
void *mm_realloc(void *ptr, size_t size)
{
    /*void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;*/
    return 0;
}














