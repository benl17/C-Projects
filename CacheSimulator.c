/*
 * csim.c:  
 * A cache simulator that can replay traces (from Valgrind) and output
 * statistics for the number of hits, misses, and evictions.
 * The replacement policy is LRU.
 */  

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

/******************************************************************************/
/* DO NOT MODIFY THESE VARIABLES **********************************************/

//Globals set by command line args.
int b = 0; //number of block (b) bits
int s = 0; //number of set (s) bits
int E = 0; //number of lines per set

//Globals derived from command line args.
int B; //block size in bytes: B = 2^b
int S; //number of sets: S = 2^s

//Global counters to track cache statistics in access_data().
int hit_cnt = 0;
int miss_cnt = 0;
int evict_cnt = 0;

//global variable that I added to keep track of current maximum
int currMax = 1;

//Global to control trace output
int verbosity = 0; //print trace if set
/******************************************************************************/
  
  
//Type mem_addr_t: Use when dealing with addresses or address masks.
typedef unsigned long long int mem_addr_t;

//Type cache_line_t: Use when dealing with cache lines.
typedef struct cache_line {                    
    char valid;
    mem_addr_t tag;
    int lruCounter; //keeps track of the current position in the least recently used queue
} cache_line_t;

//Type cache_set_t: Use when dealing with cache sets
typedef cache_line_t* cache_set_t;

//Type cache_t: Use when dealing with the cache.
typedef cache_set_t* cache_t;

// Create the cache we're simulating. 
cache_t cache;  

/* 
 * init_cache:
 * Allocates the data structure for a cache with S sets and E lines per set.
 * Initializes all valid bits and tags with 0s.
 */                    
void init_cache() {
    //get both B and S using the pow math function
    B = pow(2, b);
    S = pow(2, s);

    //allocate space for the cache sets
    cache = malloc(sizeof(cache_set_t) * S);
    if(cache == NULL) { //check that if was allocated correctly
        exit(1);
    }
     
    //allocate space the every line in the cache
    for(int i = 0; i < S; i++) {
        cache[i] = malloc(sizeof(cache_line_t) * E);
        if(cache[i] == NULL) { //check that it was allocated correctly
            exit(1);
        }
        //set each value from the struct 
        for(int u = 0; u < E; u++) {
            cache[i][u].valid = '0';
            cache[i][u].tag = 0;
            cache[i][u].lruCounter = 0;
        }
    }      
}
  

/* 
 * free_cache:
 * Frees all heap allocated memory used by the cache.
 */                    
void free_cache() {
    //for loop to iterate through cache and free every line and then free the cache itself
    for(int r = 0; r < S; r++) {
        free(cache[r]);
        cache[r] = NULL;
    }    
    free(cache);
    cache = NULL;         
}


/* 
 * access_data:
 * Simulates data access at given "addr" memory address in the cache.
 *
 * If already in cache, increment hit_cnt
 * If not in cache, cache it (set tag), increment miss_cnt
 * If a line is evicted, increment evict_cnt
 */                    
void access_data(mem_addr_t addr) {
    //local variables to help stop the function from incrementing miss_cnt and evict_cnt when it shouldn't
    int hitTracker = 0;
    int evictTracker = 0;
    
    //masks used to find the s and t bits
    mem_addr_t setMask = ((1 << s) - 1);
    mem_addr_t tMask = ((1 << (64 - s - b)) - 1);

    //find the s and t bits using the masks and &
    int setNum = (addr >> b) & setMask;
    int tNum = (addr >> (b + s)) & tMask;
    
    //look through cache to find if its already in the cache
    for(int t = 0; t < E; t++) {
        if(cache[setNum][t].tag == tNum && cache[setNum][t].valid == '1') {
            hit_cnt += 1; //it is in the cache so increment the global variable
            cache[setNum][t].lruCounter = currMax + 1;
            currMax += 1;
            hitTracker = 1; //used to stop the function from incrementing miss_cnt
            break;
        }
    }

    if(!hitTracker) {
        //was not in cache so increment miss_cnt by 1
        miss_cnt += 1;

        //iterate through entire set looking for a valid bit of 0
        for(int tag = 0; tag < E; tag++) {
            if(cache[setNum][tag].valid == '0') { 
                cache[setNum][tag].valid = '1';//set the valid bit to 1
                cache[setNum][tag].tag = tNum; //set the line's tag to tNum
                cache[setNum][tag].lruCounter = currMax + 1; //set this line to the most recently used 
                currMax += 1;
                evictTracker = 1; //to stop the function from incrementing evict_cnt
                break;
            }
        }

        if(!evictTracker) {
            //no free space in cache, have to evict so update evict_cnt by 1
            evict_cnt += 1;

            //local variables used to keep track of the least recently used line
            int leastUsed = 900000;
            int currIndexLRU = 0; 
            int lruIndex = 0;

            //iterate through the entire set looking for the least recently used line that will be evicted
            for(int y = 0; y < E; y++) {
                currIndexLRU = cache[setNum][y].lruCounter;

                //current line is the current least recently used line
                if(currIndexLRU < leastUsed) {
                    leastUsed = currIndexLRU;
                    lruIndex = y;
                }   
            }

            //set the least recently used line to the current tag and set its counter to the most recently used
            cache[setNum][lruIndex].valid = '1';
            cache[setNum][lruIndex].tag = tNum;
            cache[setNum][lruIndex].lruCounter = currMax + 1;
            currMax += 1;
        }
    }
}
  
  
/* 
 * replay_trace:
 * Replays the given trace file against the cache.
 *
 * Reads the input trace file line by line.
 * Extracts the type of each memory access : L/S/M
 */                    
void replay_trace(char* trace_fn) {           
    char buf[1000];  
    mem_addr_t addr = 0;
    unsigned int len = 0;
    FILE* trace_fp = fopen(trace_fn, "r"); 

    if (!trace_fp) { 
        fprintf(stderr, "%s: %s\n", trace_fn, strerror(errno));
        exit(1);   
    }

    while (fgets(buf, 1000, trace_fp) != NULL) {
        if (buf[1] == 'S' || buf[1] == 'L' || buf[1] == 'M') {
            sscanf(buf+3, "%llx,%u", &addr, &len);
      
            if (verbosity)
                printf("%c %llx,%u ", buf[1], addr, len);

            if(buf[1] == 'S' || buf[1] == 'L') {
                access_data(addr);
            } 
            
            if(buf[1] == 'M') {
                access_data(addr);
                access_data(addr);
            }

            if (verbosity)
                printf("\n");
        }
    }
    fclose(trace_fp);
}  
  
  
/*
 * print_usage:
 * Print information on how to use csim to standard output.
 */                    
void print_usage(char* argv[]) {                 
    printf("Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n", argv[0]);
    printf("Options:\n");
    printf("  -h         Print this help message.\n");
    printf("  -v         Optional verbose flag.\n");
    printf("  -s <num>   Number of s bits for set index.\n");
    printf("  -E <num>   Number of lines per set.\n");
    printf("  -b <num>   Number of b bits for block offsets.\n");
    printf("  -t <file>  Trace file.\n");
    printf("\nExamples:\n");
    printf("  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n", argv[0]);
    printf("  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n", argv[0]);
    exit(0);
}  
  
  
/*
 * print_summary:
 * Prints a summary of the cache simulation statistics to a file.
 */                    
void print_summary(int hits, int misses, int evictions) {                
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}  
  
  
/*
 * main:
 * Main parses command line args, makes the cache, replays the memory accesses
 * free the cache and print the summary statistics.  
 */                    
int main(int argc, char* argv[]) {                      
    char* trace_file = NULL;
    char c;
    
    // Parse the command line arguments: -h, -v, -s, -E, -b, -t 
    while ((c = getopt(argc, argv, "s:E:b:t:vh")) != -1) {
        switch (c) {
            case 'b':
                b = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'h':
                print_usage(argv);
                exit(0);
            case 's':
                s = atoi(optarg);
                break;
            case 't':
                trace_file = optarg;
                break;
            case 'v':
                verbosity = 1;
                break;
            default:
                print_usage(argv);
                exit(1);
        }
    }

    //Make sure that all required command line args were specified.
    if (s == 0 || E == 0 || b == 0 || trace_file == NULL) {
        printf("%s: Missing required command line argument\n", argv[0]);
        print_usage(argv);
        exit(1);
    }

    //Initialize cache.
    init_cache();

    //Replay the memory access trace.
    replay_trace(trace_file);

    //Free memory allocated for cache.
    free_cache();

    //Print the statistics to a file.
    //DO NOT REMOVE: This function must be called for test_csim to work.
    print_summary(hit_cnt, miss_cnt, evict_cnt);
    return 0;   
} 
