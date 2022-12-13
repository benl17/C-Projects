#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "p3Heap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           

    int size_status;

    /*
     * Size of the block is always a multiple of 8.
     * Size is stored in all block headers and in free block footers.
     *
     * Status is stored only in headers using the two least significant bits.
     *   Bit0 => least significant bit, last bit
     *   Bit0 == 0 => free block
     *   Bit0 == 1 => allocated block
     *
     *   Bit1 => second last bit 
     *   Bit1 == 0 => previous block is free
     *   Bit1 == 1 => previous block is allocated
     * 
     * End Mark: 
     *  The end of the available memory is indicated using a size_status of 1.
     * 
     * Examples:
     * 
     * 1. Allocated block of size 24 bytes:
     *    Allocated Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 25
     *      If the previous block is allocated p-bit=1 size_status would be 27
     * 
     * 2. Free block of size 24 bytes:
     *    Free Block Header:
     *      If the previous block is free      p-bit=0 size_status would be 24
     *      If the previous block is allocated p-bit=1 size_status would be 26
     *    Free Block Footer:
     *      size_status should be 24
     */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heap_start = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int alloc_size;

/*
 * Additional global variables may be added as needed below
 */

 /*
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block (payload) on success.
 * Returns NULL on failure.
 */
void* balloc(int size) {   
    //check if size is less than 1 or if size is greater than heap size
    if(size < 1 || size + sizeof(blockHeader) > alloc_size) {
    	return NULL;
    }

    //const variables for header and padding size so they can't be changed and then add them all into blockSize for true size of block that will be allocated
    int const hdrSize = sizeof(blockHeader);
    int blockPayload = size;
    int const blockPadding = (8 - ((hdrSize + blockPayload) % 8));
    int blockSizeNeed = hdrSize + blockPayload + blockPadding;

    blockHeader *currBlock = heap_start; //start of the heap
    blockHeader *currBestFit = NULL; //used when there isn't a perfect fit and need to keep track of current best fit
    blockHeader *splitBlock = NULL; //used in case of a non perfect fit
    blockHeader *nextBlock = NULL; //used to get the next block

    //variables used in the while loop below
    int currBlockSize;
    int currBestFitSize = 2000000000; //used to track size of current best fit 
    int splitFit = 0; //used to keep track if there has been a non perfect fit found
    int currShifted;
    int prevPBit = 0; //used to keep track of p bit for block in case of non perfect block found

    //while loop to loop through the entire heap looking for a block
    while (currBlock->size_status != 1) {
        currBlockSize = currBlock->size_status; //size of current block including the p and a bit
        currShifted = (currBlockSize >> 2) << 2; //get rid of the p and a bit so it doesn't affect address arithmetic 

        if(currBlockSize & 1) { //block is allocated
            currBlock = (blockHeader*)((char*)currBlock + currShifted);
            continue; //continue going through while loop 
        } else { //block is free
            if(currShifted == blockSizeNeed) { //perfect fit, exit while loop
                currBestFit = currBlock;
                currBestFit->size_status = currShifted;
                currBestFit->size_status += 1; //a bit
                if(currBlockSize & 2) { //checks for p bit
                    currBestFit->size_status += 2; //add p bit if there is 
                }
                nextBlock = (blockHeader*)((char*)currBlock + currShifted); 
                nextBlock->size_status += 2; //set p bit of the next block
                return ((void*)currBestFit) + sizeof(blockHeader); //return the address of the payload
            } else if(currShifted > blockSizeNeed) {//not a perfect fit
                if(currShifted < currBestFitSize) { //checks if currBlockSize is less than previous best fit size
                    if(currBlockSize & 2){ //checks for the p bit that will be used after while loop
                        prevPBit = 1;
                    } else {
                        prevPBit = 0;
                    }
                    currBestFit = currBlock;
                    currBestFitSize = currShifted;
                    if(!splitFit) { //checks if there has been a block found yet
                        splitFit = 1;
                    }
                }
                currBlock = (blockHeader*)((char*)currBlock + currShifted); //continue to loop through the while loop
                continue;
            }
            currBlock = (blockHeader*)((char*)currBlock + currShifted); //to stop infinite loops, if the currBlockSize does not meet any of the if statements
            continue;
        }
    }

    if(splitFit) { //block found is not a perfect fit
        currBestFit->size_status = blockSizeNeed;
        currBestFit->size_status += 1; //allocate

        if(prevPBit && !(currBestFit->size_status & 2)) { //checks prevPBit variable and if the currBestFit already has a pbit
            currBestFit->size_status += 2;
        } 

        splitBlock = (blockHeader*)((char*)currBestFit + blockSizeNeed); //split the block
        splitBlock->size_status = currBestFitSize - blockSizeNeed;

        if(splitBlock->size_status != 1) {
            splitBlock->size_status += 2; //set p bit of the split block
        }
        
        return ((void*)currBestFit) + sizeof(blockHeader); //return the address of the payload
    } else { 
        return NULL; //no block found, null is returned
    }
} 

/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - Update header(s) and footer as needed.
 */                    
int bfree(void *ptr) {   
    if(ptr == NULL) {  //checks if the pointer is null
        return -1;
    } 

    blockHeader *ptrBlock = (blockHeader*)((char*)ptr - sizeof(blockHeader)); //adjust for the header
    if((unsigned int)ptr % 8 != 0) { //checks if the pointer is a multiple of 8
        return -1;
    } 
    if(!(ptrBlock->size_status & 1)) { //block is already freed
        return -1;
    }
    if(ptrBlock < heap_start || ptrBlock > (heap_start + alloc_size - 8)) { //checks if the ptr is inside the heap space
        return -1;
    }

    //actualy free the block
    int ptrBlockSizeShifted = (ptrBlock->size_status >> 2) << 2;
    ptrBlock->size_status -= 1; //set the a bit to 0

    blockHeader *nextBlock = (blockHeader*)((char*)ptrBlock + ptrBlockSizeShifted); //get the next block
    
    if(nextBlock->size_status != 1 && nextBlock->size_status & 2) { //check if the next block is the end and if the p bit is set
        nextBlock->size_status -= 2; //set the p bit of the next block to 0
    }   
    
    return 0;
} 

/*
 * Function for traversing heap block list and coalescing all adjacent 
 * free blocks.
 *
 * This function is used for delayed coalescing.
 * Updated header size_status and footer size_status as needed.
 */
int coalesce() {
    blockHeader *currCoalBlock = heap_start; //start of the heap 
    blockHeader *nextCoalBlock;

    int counter = 0; //used to keep track of the number of coalesced blocks 
    int currCoalBlockSize;
    int currCoalShifted; //size of block without a and p bit
    int nextCoalShifted; //size of block without a and p bit
    int nextCoalBlockSize;

    while(currCoalBlock->size_status != 1) {
        currCoalBlockSize = currCoalBlock->size_status; //size of the current block including a and p bit
        currCoalShifted = (currCoalBlockSize >> 2) << 2; //shift the bits to get rid of a and p bit

        if(((blockHeader*)((char*)currCoalBlock + currCoalShifted))->size_status == 1) { //if the next block is the end of heap, break loop
            break;
        }

        nextCoalBlock = (blockHeader*)((char*)currCoalBlock + currCoalShifted); //next block 
        nextCoalBlockSize = nextCoalBlock->size_status;
        nextCoalShifted = (nextCoalBlockSize >> 2) << 2;

        //current block is allocated
        if(currCoalBlockSize & 1){
            currCoalBlock = nextCoalBlock; //set the current block to the next block
            continue;
        }

        //next block is allocated
        if(nextCoalBlockSize & 1){
            currCoalBlock = (blockHeader*)((char*)nextCoalBlock + nextCoalShifted); //get the block after the next of current block
            continue;
        }

        //if both currBlock and nextBlock are free, then coalesce
        if( (!(currCoalBlockSize & 1)) && (!(nextCoalBlockSize & 1)) ) {
            currCoalBlock->size_status += nextCoalShifted; //current block is now current + next block, coalesced 
            currCoalShifted = ((currCoalBlock->size_status) >> 2) << 2;
            nextCoalBlock->size_status = 0; //set the size of the next block to 0

            currCoalBlock = (blockHeader*)((char*)currCoalBlock + currCoalShifted);
            counter++; //blocks have been coalesced, we can now return 1 when while loop exited
            continue;
        }
    }

    if(counter < 1){ //if no blocks were coalesced 
        return 0;
    } else { //else blocks were coalesced
        return 1;
    }
}

 
/* 
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int init_heap(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;   // page size
    int padsize;    // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* end_mark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }

    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    alloc_size = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    alloc_size -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heap_start = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    end_mark = (blockHeader*)((void*)heap_start + alloc_size);
    end_mark->size_status = 1;

    // Set size in header
    heap_start->size_status = alloc_size;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heap_start->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heap_start + alloc_size - 4);
    footer->size_status = alloc_size;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void disp_heap() {     
 
    int counter;
    char status[6];
    char p_status[6];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heap_start;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, 
	"*********************************** Block List **********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "alloc");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "FREE ");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "alloc");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "FREE ");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%4i\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, 
	"---------------------------------------------------------------------------------\n");
    fprintf(stdout, 
	"*********************************************************************************\n");
    fprintf(stdout, "Total used size = %4d\n", used_size);
    fprintf(stdout, "Total free size = %4d\n", free_size);
    fprintf(stdout, "Total size      = %4d\n", used_size + free_size);
    fprintf(stdout, 
	"*********************************************************************************\n");
    fflush(stdout);

    return;  
} 