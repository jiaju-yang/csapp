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
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *bp);
extern void *mm_realloc(void *bp, size_t size);

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t size);
static void place(void *bp, size_t size);
static void split(void *bp, size_t size);
static int mm_check(int lineNo);

#define WSIZE 4
#define DSIZE (2 * WSIZE)
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp)-WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp)-GET_SIZE((char *)(bp)-DSIZE))

#define GET_EPILOGUE(bp) (!GET_SIZE(HDRP(bp)) && GET_ALLOC(HDRP(bp)))

static char *heap_listp;

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));
    heap_listp += (2 * WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *bp, size_t size)
{
    size_t copySize = GET_SIZE(HDRP(bp));
    if (size < copySize)
        return bp;

    void *newBp = mm_malloc(size);
    void *oldBp = bp;
    if (newBp == NULL)
        return NULL;
    memcpy(newBp, oldBp, copySize);
    mm_free(oldBp);
    return newBp;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
        return bp;
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void *find_fit(size_t size)
{
    char *bp;
    for (bp = NEXT_BLKP(heap_listp); !GET_EPILOGUE(bp); bp = NEXT_BLKP(bp))
        if ((GET_SIZE(HDRP(bp)) >= size) && (!GET_ALLOC(HDRP(bp))))
        {
            return bp;
        }
    return NULL;
}

static void place(void *bp, size_t size)
{
    if (GET_SIZE(HDRP(bp)) > size + DSIZE)
    {
        split(bp, size);
    }
    PUT(HDRP(bp), PACK(size, 1));
    PUT(FTRP(bp), PACK(size, 1));
}

static void split(void *bp, size_t size)
{
    size_t sec_size = GET_SIZE(HDRP(bp)) - size;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(sec_size, 0));
    PUT(FTRP(NEXT_BLKP(bp)), PACK(sec_size, 0));
}

static int mm_check(int lineno)
{
    char *bp = heap_listp;
    printf("\nMemory check called from %d\n", lineno);
    printf("Unused padding word at: %p\n", bp - DSIZE);
    printf("Unused padding word block size: %d, block allocated: %d\n", GET_SIZE(bp - DSIZE), GET_ALLOC(bp - DSIZE));
    for (; !GET_EPILOGUE(bp); bp = NEXT_BLKP(bp))
    {
        printf("Block header at %p\n", bp - WSIZE);
        printf("Header block size: %d, footer block size %d\nHeader block allocated: %d, footer block allocated: %d\n", GET_SIZE(HDRP(bp)), GET_SIZE(FTRP(bp)), GET_ALLOC(HDRP(bp)), GET_ALLOC(FTRP(bp)));
    }
    printf("Epilogue block at %p\n", FTRP(bp) + WSIZE);
    printf("Epilogue block size: %d, block allocated: %d\n", GET_SIZE(FTRP(bp) + WSIZE), GET_ALLOC(FTRP(bp) + WSIZE));
    return 0;
}
