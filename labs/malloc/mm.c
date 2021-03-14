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
#include <stdint.h>
#include <stdbool.h>

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

typedef uint32_t word;
typedef unsigned long number_of_words;

#define WSIZE (sizeof(word))
#define DSIZE (2 * WSIZE)
#define _CHUNKSIZE (1 << 12)
#define CHUNK_WORDS (_CHUNKSIZE / WSIZE)
#define MIN_MALLOC_WORDS 4

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define _PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(word *)(p))
#define PUT(p, val) (*(word *)(p) = (val))

#define _GET_SIZE(p) (GET(p) & ~0x7)
#define _GET_SIZE_IN_WORD(p) (_GET_SIZE(p) / WSIZE)
#define _GET_ALLOC(p) (GET(p) & 0x1)
#define GET_BLK_SIZE(bp) (_GET_SIZE_IN_WORD(HDRP(bp)))
#define GET_BLK_ALLOC(bp) (_GET_ALLOC(HDRP(bp)))
#define SET_BLK_HDR(bp, words, alloc) (PUT(HDRP(bp), _PACK(words * WSIZE, alloc)))
#define SET_BLK_FTR(bp, words, alloc) (PUT(FTRP(bp), _PACK(words * WSIZE, alloc)))

#define HDRP(bp) ((word *)(bp)-1)
#define FTRP(bp) ((word *)(bp) + GET_BLK_SIZE(bp) - 2)

#define NEXT_BLKP(bp) ((word *)(bp) + GET_BLK_SIZE(bp))
#define PREV_BLKP(bp) ((word *)(bp)-_GET_SIZE_IN_WORD((word *)(bp)-2))

#define REACH_EPILOGUE(bp) (!_GET_SIZE(HDRP(bp)) && GET_BLK_ALLOC(bp))
#define SET_NEXT_BLK_AS_EPILOGUE(bp) (PUT(FTRP(bp) + 1, 1))

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *bp);
extern void *mm_realloc(void *bp, size_t size);

static void *extend_heap(number_of_words words);
static void *coalesce(word *bp);
static void *find_fit(number_of_words size);
static void place(word *bp, number_of_words size);
static void split(word *bp, number_of_words size);
static int mm_check(int line_no);

static word *heap_listp;

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);
    heap_listp += 2;
    SET_BLK_HDR(heap_listp, 2, 1);
    SET_BLK_FTR(heap_listp, 2, 1);
    SET_NEXT_BLK_AS_EPILOGUE(heap_listp);
    if (extend_heap(CHUNK_WORDS) == NULL)
        return -1;
    return 0;
}

void *mm_malloc(size_t size)
{
    number_of_words awords;
    number_of_words extend_words;
    word *bp;

    if (size == 0)
        return NULL;

    if (size <= DSIZE)
        awords = MIN_MALLOC_WORDS;
    else
        awords = (DSIZE / WSIZE) * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);

    if ((bp = find_fit(awords)) != NULL)
    {
        place(bp, awords);
        return bp;
    }
    extend_words = MAX(awords, CHUNK_WORDS);
    if ((bp = extend_heap(extend_words)) == NULL)
        return NULL;
    place(bp, awords);
    return bp;
}

void mm_free(void *bp)
{
    number_of_words words = GET_BLK_SIZE(bp);
    SET_BLK_HDR(bp, words, 0);
    SET_BLK_FTR(bp, words, 0);
    coalesce(bp);
}

void *mm_realloc(void *bp, size_t size)
{
    number_of_words copy_words = GET_BLK_SIZE(bp);
    number_of_words new_words = size / WSIZE;
    if (new_words < copy_words)
        return bp;

    word *new_bp = mm_malloc(size);
    word *old_bp = bp;
    if (new_bp == NULL)
        return NULL;
    memcpy(new_bp, old_bp, copy_words * WSIZE);
    mm_free(old_bp);
    return new_bp;
}

static void *extend_heap(number_of_words words)
{
    word *bp;
    number_of_words awords;

    awords = (words % 2) ? words + 1 : words;
    if ((long)(bp = mem_sbrk(awords * WSIZE)) == -1)
        return NULL;

    SET_BLK_HDR(bp, awords, 0);
    SET_BLK_FTR(bp, awords, 0);
    SET_NEXT_BLK_AS_EPILOGUE(bp);
    return coalesce(bp);
}

static void *coalesce(word *bp)
{
    bool prev_alloc = GET_BLK_ALLOC(PREV_BLKP(bp));
    bool next_alloc = GET_BLK_ALLOC(NEXT_BLKP(bp));
    number_of_words words = GET_BLK_SIZE(bp);

    if (prev_alloc && next_alloc)
        return bp;
    else if (prev_alloc && !next_alloc)
    {
        words += GET_BLK_SIZE(NEXT_BLKP(bp));
        SET_BLK_HDR(bp, words, 0);
        SET_BLK_FTR(bp, words, 0);
    }
    else if (!prev_alloc && next_alloc)
    {
        words += GET_BLK_SIZE(PREV_BLKP(bp));
        SET_BLK_HDR(PREV_BLKP(bp), words, 0);
        SET_BLK_FTR(PREV_BLKP(bp), words, 0);
        bp = PREV_BLKP(bp);
    }
    else
    {
        words += (GET_BLK_SIZE(PREV_BLKP(bp)) + GET_BLK_SIZE(NEXT_BLKP(bp)));
        SET_BLK_HDR(PREV_BLKP(bp), words, 0);
        SET_BLK_FTR(NEXT_BLKP(bp), words, 0);
        bp = PREV_BLKP(bp);
    }

    return bp;
}

static void *find_fit(number_of_words words)
{
    word *bp;
    for (bp = NEXT_BLKP(heap_listp); !REACH_EPILOGUE(bp); bp = NEXT_BLKP(bp))
        if ((GET_BLK_SIZE(bp) >= words) && (!GET_BLK_ALLOC(bp)))
            return bp;
    return NULL;
}

static void place(word *bp, number_of_words words)
{
    if (GET_BLK_SIZE(bp) > words + 2)
        split(bp, words);
    SET_BLK_HDR(bp, words, 1);
    SET_BLK_FTR(bp, words, 1);
}

static void split(word *bp, number_of_words words)
{
    number_of_words sec_half_words = GET_BLK_SIZE(bp) - words;
    SET_BLK_HDR(bp, words, 0);
    SET_BLK_FTR(bp, words, 0);
    SET_BLK_HDR(NEXT_BLKP(bp), sec_half_words, 0);
    SET_BLK_FTR(NEXT_BLKP(bp), sec_half_words, 0);
}

static int mm_check(int line_no)
{
    word *bp = heap_listp;
    printf("\nMemory check called from %d\n", line_no);
    printf("Unused padding word at: %p\n", bp - 2);
    printf("Unused padding word block size: %d words, block allocated: %d\n", GET_BLK_SIZE(bp - 1), GET_BLK_ALLOC(bp - 1));
    for (; !REACH_EPILOGUE(bp); bp = NEXT_BLKP(bp))
    {
        printf("Block header at %p\n", HDRP(bp));
        printf("Header block size: %d words, footer block size %d words\nHeader block allocated: %d, footer block allocated: %d\n", GET_BLK_SIZE(bp), _GET_SIZE_IN_WORD(FTRP(bp)), GET_BLK_ALLOC(bp), _GET_ALLOC(FTRP(bp)));
    }
    printf("Epilogue block at %p\n", HDRP(bp));
    printf("Epilogue block size: %d words, block allocated: %d\n", GET_BLK_SIZE(bp), GET_BLK_ALLOC(bp));
    return 0;
}
