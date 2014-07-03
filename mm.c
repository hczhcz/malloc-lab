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
    /* best fit */
    // #define MM_FIT(need, size) (0)
    /* mixed fit */
    // #define MM_FIT(need, size) (((size) << 1) < (need))
    /* first fit */
    #define MM_FIT(need, size) (1)

/* if searching is enabled (not only breaking) */
    /* always */
    // #define MM_SEARCH() (1)
    /* cond 1 */
    // #define MM_SEARCH() (total_alloc - total_free < (total_brk >> 1))
    /* block first */
    #define MM_SEARCH() (total_alloc > need_size)

/* if check param of functions */
    // #define MM_CHECK

/* cast a void pointer to a header pointer */
#define MM(ptr) ((MMHeader *) (ptr))

/* header of a data block */
typedef struct {
    /* size of the block, apply SIGN_MARK if the block is not used */
    size_t size;
} MMHeader;

/* first free block */
void *first_free;

/* historical data */
size_t total_alloc = 0;
size_t total_free = 0;
size_t total_brk = 0;

/*
 * mm_first_free - Get and also update first_free.
 */
inline void *mm_first_free(void)
{
    void *now = first_free;
    size_t now_size = 0;

    // scan the block list
    while ((now - 1) != mem_heap_hi()) {
        now_size = MM(now)->size;

        if (SIGN_CHECK(now_size)) {
            break;
        } else {
            now += now_size;
        }
    }

    // write back
    first_free = now;

    return now;
}

/*
 * mm_get_first_free - Access first_free.
 */
inline void *mm_get_first_free(void)
{
    return first_free;
}

/*
 * mm_set_first_free - Change first_free.
 */
inline void mm_set_first_free(void *ptr)
{
    first_free = ptr;
}

/*
 * mm_put_header - Generate the header of a block and return the data section.
 */
inline void *mm_put_header(void *ptr, size_t size)
{
    MM(ptr)->size = size;
    return ptr + MM_HEADER_SIZE;
}

/*
 * mm_restore_header - Get the header of a block from a data pointer.
 */
inline void *mm_restore_header(void *ptr)
{
    return ptr - MM_HEADER_SIZE;
}

/*
 * mm_print - Print block list for debug purpose.
 */
void mm_print(void)
{
    void *now = mem_heap_lo();
    size_t now_size = 0;

    printf("\n");

    // scan the block list
    while ((now - 1) != mem_heap_hi()) {
        now_size = MM(now)->size;
        printf("pos: %x - head: %x\n", (unsigned) now, now_size);

        // visit the next block
        now += SIGN_CHECK(now_size) ? SIGN_MARK(now_size) : now_size;
    }
}

/*
 * mm_merge - Try to merge with next free block.
 */
inline int mm_merge(void *ptr, size_t *p_size)
{
    void *next = ptr + *p_size;
    size_t next_size = MM(next)->size;

    // if the next block can be merged
    if ((next - 1) != mem_heap_hi() && SIGN_CHECK(next_size)) {
        *p_size += SIGN_MARK(next_size);
        mm_put_header(ptr, SIGN_MARK(*p_size));
        return -1;
    } else {
        return 0;
    }
}

/*
 * mm_split - Split from a block if it is possible, and use part of it.
 */
inline void *mm_split(void *ptr, size_t size, size_t need_size)
{
    size_t more_size = size - need_size;

    // if these is more space for another block
    if (more_size >= USEFUL_SIZE) {
        // create another block
        mm_put_header(ptr + need_size, SIGN_MARK(more_size));

        // generate the block
        return mm_put_header(ptr, need_size);
    } else {
        // wasted size
        total_alloc += more_size;

        // generate the block
        return mm_put_header(ptr, size);
    }
}

/*
 * mm_break - Extend the last block.
 */
inline void *mm_break(void *ptr, size_t size, size_t need_size)
{
    size_t brk_size = need_size - size;

    // historical data
    total_brk += brk_size;

    // extense the heap
    mem_sbrk(brk_size);

    // generate the block
    return mm_put_header(ptr, need_size);
}

/*
 * mm_init - Initialize the malloc package.
 */
int mm_init(void)
{
    mm_set_first_free(mem_heap_lo());

    // put a small block to hold some data
    mem_sbrk(48);
    mm_put_header(mem_heap_lo(), SIGN_MARK(48));

    return 0;
}

/*
 * mm_malloc - Allocate a block.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
#ifdef MM_CHECK
    if (!size) return;
#endif

    void *now = mm_first_free();
    void *best = NULL;
    size_t now_size = 0;
    size_t best_size = SIZE_T_MAX;
    size_t need_size = MM_HEADER_SIZE + ALIGN(size);

    // historical data
    total_alloc += need_size;

    if (MM_SEARCH()) {
        // scan the block list
        // if the best one is not found, keep best == NULL
        // if the last one is used, let now_size == 0
        while ((now - 1) != mem_heap_hi()) {
            now_size = MM(now)->size;

            if (SIGN_CHECK(now_size)) {
                // current block is not used

                now_size = SIGN_MARK(now_size);

                // try to merge useable blocks
                while (now_size < need_size && mm_merge(now, &now_size));

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

        if (best) {
            // there is useable block and splitting is possible
            return mm_split(best, best_size, need_size);
        } else {
            // there is no useable block and sbrk is needed
            return mm_break(now - now_size, now_size, need_size);
        }
    } else {
        // force sbrk
        return mm_break(mem_heap_hi() + 1, 0, need_size);
    }
}

/*
 * mm_free - Free a block.
 */
void mm_free(void *ptr)
{
#ifdef MM_CHECK
    if (!ptr) return;
#endif

    void *now = mm_restore_header(ptr);

    // historical data
    total_free += MM(now)->size;

    // change first_free
    if (now < mm_get_first_free()) {
        mm_set_first_free(now);
    }

    // mark it as an unused block
    mm_put_header(now, SIGN_MARK(MM(now)->size));
}

/*
 * mm_realloc - Change the size of a block.
 */
void *mm_realloc(void *ptr, size_t size)
{
#ifdef MM_CHECK
    if (!ptr) return mm_malloc(size);
    if (!size) return mm_free(ptr);
#endif

    void *now = mm_restore_header(ptr);
    void *dest;
    size_t now_size = MM(now)->size;
    size_t need_size = MM_HEADER_SIZE + ALIGN(size);

    // try to merge useable blocks
    while (now_size < need_size && mm_merge(now, &now_size));

    if (now_size >= need_size) {
        // the block can be locally extensed and splitting is possible
        return mm_split(now, now_size, need_size);
    } else {
        if ((now + now_size - 1) == mem_heap_hi()) {
            // the block is the last block and it can be extensed
            return mm_break(now, now_size, need_size);
        } else {
            // need to allocate a new block and copy data to it
            dest = mm_malloc(size);
            memcpy(dest, ptr, MM(now)->size - MM_HEADER_SIZE);
            mm_free(ptr);
            return dest;
        }
    }
}














