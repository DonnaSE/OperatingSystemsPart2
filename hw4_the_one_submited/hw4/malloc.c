/*
Name: Mohammed Al Aadhami
ID: 1001417231

Name: Raghad Safauldeen
ID: 1001417235
*/

#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <limits.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)      ((b) + 1)
#define BLOCK_HEADER(ptr)   ((struct block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void printStatistics( void )
{
    printf("\nheap management statistics\n");
    printf("mallocs:\t%d\n", num_mallocs );
    printf("frees:\t\t%d\n", num_frees );
    printf("reuses:\t\t%d\n", num_reuses );
    printf("grows:\t\t%d\n", num_grows );
    printf("splits:\t\t%d\n", num_splits );
    printf("coalesces:\t%d\n", num_coalesces );
    printf("blocks:\t\t%d\n", num_blocks );
    printf("requested:\t%d\n", num_requested );
    printf("max heap:\t%d\n", max_heap );
}

struct block
{
    size_t       size;  /* Size of the allocated block of memory in bytes */
    struct block *next; /* Pointer to the next block of allcated memory   */
    bool         free;  /* Is this block free?                     */
};

struct block *FreeList = NULL; /* Free list to track the blocks available */

struct block * LastBlock = NULL;

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free blocks
 * \param size size of the block needed in bytes
 *
 * \return a block that fits the request or NULL if no free block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct block *findFreeBlock(struct block **last, size_t size)
{
    struct block *curr = FreeList;

#if defined FIT && FIT == 0
    /* First fit */
    while (curr && !(curr->free && curr->size >= size))
    {
        *last = curr;
        curr  = curr->next;
    }
#endif

#if defined WORST && WORST == 0
    int biggest=INT_MIN;
    struct block *ptr=NULL;
    while (curr )
    {
        // try to find biggest current size avalable
        if((int)(curr->size - size)>biggest && (curr->free ))
        {
            ptr = curr;
            biggest = curr->size - size;
        }
        curr=curr->next;
    }
    curr = ptr;

#endif

#if defined BEST && BEST == 0
    //printf("TODO: Implement best fit here\n");
    int smallest=INT_MAX;
    struct block *ptr = NULL;
    while (curr )
    {
        // try to find biggest current size avalable
        if((int)(curr->size - size)< smallest && (curr->free ))
        {
            ptr = curr;
            smallest = curr->size - size;
        }
        curr = curr->next;
    }
    curr = ptr;
#endif

#if defined NEXT && NEXT == 0
    //  printf("TODO: Implement next fit here\n");
    //curr = LastBlock;
    //printf("LAST BLOCK: %p\n", LastBlock);
    if (LastBlock == NULL) LastBlock = FreeList;
    while(curr != NULL && curr->next != LastBlock)
    {
        //printf("JUMP: %p\n", curr);
        if (curr->free && curr->size >= size)
        {
            break;
        }

        if(curr->next==NULL)
        {
            *last = curr;
            curr = FreeList;
            if (curr == LastBlock)
            {
                return NULL;
            }
        }
        else
        {
            *last = curr;
            curr = curr -> next;
        }
    }

//printf("Starting at %p\n", curr );
#endif
    if( curr != NULL )
    {
        LastBlock = curr;
    }
    return curr;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated block of NULL if failed
 */
struct block *growHeap(struct block *last, size_t size)
{
//printf("Growing the heap by %ld\n", size);
    /* Request more space from OS */
    struct block *curr = (struct block *)sbrk(0);
    struct block *prev = (struct block *)sbrk(sizeof(struct block) + size);

    assert(curr == prev);

    /* OS allocation failed */
    if (curr == (struct block *)-1)
    {
        return NULL;
    }
    /* Update FreeList if not set */
    if (FreeList == NULL)
    {
        FreeList = curr;
        LastBlock = FreeList;
    }

    /* Attach new block to prev block */
    if (last)
    {
        last->next = curr;
    }

    /* Update block metadata */
    curr->size = size;
    curr->next = NULL;
    curr->free = false;
    num_blocks++;
    num_grows++;
    max_heap = max_heap + size;
    return curr;
}

/*
 * \brief malloc
 *
 * finds a free block of heap memory for the calling process.
 * if there is no free block that satisfies the request then grows the
 * heap and returns a new block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{

    if( atexit_registered == 0 )
    {
        atexit_registered = 1;
        atexit( printStatistics );
    }

    /* Align to multiple of 4 */
    size = ALIGN4(size);

    /* Handle 0 size */
    if (size == 0)
    {
        return NULL;
    }

    /* Look for free block */
    struct block *last = FreeList;
    struct block *next = findFreeBlock(&last, size);

    /* TODO: Split free block if possible */
    // implement block splitting
    if(next!=NULL)
    {
        if(next->size > size)
        {
            size_t orig_size=next->size;
            struct block *orig_next=next->next;

            next->size=size;
            next->next=(void*)next + sizeof(struct block) + size;

            next->next->size=orig_size - size - sizeof(struct block);
            next->next->next=orig_next;
            num_blocks++;
            num_splits++;
        }
    }
    /* Could not find free block, so grow heap */
    if (next == NULL)
    {
        next = growHeap(last, size);
        LastBlock = next;
    }
    else
    {
        num_reuses++;
    }
    /* Could not find free block or grow heap, so just return NULL */
    if (next == NULL)
    {
        return NULL;
    }

    /* Mark block as in use */
    next->free = false;
    num_mallocs++;
    num_requested++;
    /* Return data address associated with block */
//printf("Retuning %p\n", next );
    return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory block pointed to by pointer. if the block is adjacent
 * to another block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr)
{
    if (ptr == NULL)
    {
        return;
    }

    /* Make block as free */
    struct block *curr = BLOCK_HEADER(ptr);
    assert(curr->free == 0);
    curr->free = true;
    num_frees++;
    // TODO: Coalesce free blocks if needed
    if(curr->next!=NULL)
    {
        if(curr->next->free == 1)
        {
            if(curr->next == LastBlock)
            {
                LastBlock= curr;
            }
            curr->size = curr->size + curr->next->size;
            curr->next = curr->next->next;
            num_coalesces++;
            num_blocks--;
        }
    }
}

/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/
