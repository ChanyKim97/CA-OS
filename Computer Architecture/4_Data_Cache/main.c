#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for getopt()

#define BYTES_PER_WORD 4

// #define DEBUG

/*
 * Cache structures
 */
int time = 0;

typedef struct
{
    int age;
    int valid;
    int modified;
    uint32_t tag;
} cline;

typedef struct
{
    cline *lines;
} cset;

typedef struct
{
    int s; // index bits
    int E; // way
    int b; // block offset bits
    cset *sets;
} cache;

int index_bit(int n){
    int cnt = 0;

    while(n) {
        cnt++;
        n = n >> 1;
    }

    return cnt-1;
}

/***************************************************************/
/*                                                             */
/* Procedure : build_cache                                     */
/*                                                             */
/* Purpose   : Initialize cache structure                      */
/*                                                             */
/* Parameters:                                                 */
/*     int S: The set of cache                                 */
/*     int E: The associativity way of cache                   */
/*     int b: The blocksize of cache                           */
/*                                                             */
/***************************************************************/
cache build_cache(int S, int E, int b)
{
	/* Implement this function */
    cache r_cache;
    r_cache.sets = malloc(sizeof(cset) * S);

    r_cache.b = index_bit(b);
    r_cache.s = index_bit(S);
    r_cache.E = E;

    for (int i = 0; i < S; i++) { //초기화

        r_cache.sets[i].lines = malloc(sizeof(cline) * E);

        for (int j = 0; j < E; j++) {

            r_cache.sets[i].lines[j].age = 0;
            r_cache.sets[i].lines[j].modified = 0;
            r_cache.sets[i].lines[j].valid = 0;
            r_cache.sets[i].lines[j].tag = 0;
        }
    }

    return r_cache;
	// return result_cache;
}

/***************************************************************/
/*                                                             */
/* Procedure : access_cache                                    */
/*                                                             */
/* Purpose   : Update cache stat and content                   */
/*                                                             */
/* Parameters:                                                 */
/*     cache *L: An accessed cache                             */
/*     int op: Read/Write operation                            */
/*     uint32_t addr: The address of memory access             */
/*     int *hit: The number of cache hit                       */
/*     int *miss: The number of cache miss                     */
/*     int *wb: The number of write-back                       */
/*                                                             */
/***************************************************************/
void access_cache(cache *L, int op, uint32_t addr, int *hit, int *miss, int *wb)
{
	/* Implement this function */
    int last = 0;

    int indexbit = L->s;
    int blockbit = L->b;

    uint32_t tag = addr >> (indexbit + blockbit);

    int index = (addr >> blockbit) % (1 << indexbit);

    if (op == 0) { //read
        for (int i = 0; i < L->E; i++) {
            if (L->sets[index].lines[i].age < L->sets[index].lines[last].age) {
                last = i;
            }

            if (L->sets[index].lines[i].valid == 1) {//있는자리 매칭                                      //read hit 부족
                if (L->sets[index].lines[i].tag == tag) {                                                 //write도 마찬가지
                    *hit += 1;
                    L->sets[index].lines[i].age = time;//0
                    return;
                }
            }

            else if (L->sets[index].lines[i].valid == 0) { //빈자리
                *miss += 1;                                                                              //read miss 개많음
                L->sets[index].lines[i].valid = 1;                                                         //wirte 동일
                L->sets[index].lines[i].age = time;
                L->sets[index].lines[i].tag = tag;
                return;
            }
        }
        if (L->sets[index].lines[last].modified == 1) { //write back
            *wb += 1;
            //printf("\n\n\n\n\n\n\n&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&check&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n\n\n\n");
        }

        *miss += 1;
        L->sets[index].lines[last].age = time;
        L->sets[index].lines[last].tag = tag;
        L->sets[index].lines[last].modified = 0;
        L->sets[index].lines[last].valid= 1;
    }

    else if (op == 1) { //write
        for (int i = 0; i < L->E; i++) {
            if (L->sets[index].lines[i].age < L->sets[index].lines[last].age) {
                last = i;
            }

            if (L->sets[index].lines[i].valid == 1) {//
                if (L->sets[index].lines[i].tag == tag) {
                    *hit += 1;
                    L->sets[index].lines[i].age = time;
                    L->sets[index].lines[i].modified = 1;
                    return;
                }
            }

            else if (L->sets[index].lines[i].valid == 0) {
                *miss += 1;
                L->sets[index].lines[i].valid = 1;
                L->sets[index].lines[i].age = time;
                L->sets[index].lines[i].tag = tag;
                L->sets[index].lines[i].modified = 1;
                return;
            }
        }
        if (L->sets[index].lines[last].modified == 1) {
            *wb += 1;
        }

        *miss += 1;
        L->sets[index].lines[last].age = time;
        L->sets[index].lines[last].tag = tag;
        L->sets[index].lines[last].modified = 1;
        L->sets[index].lines[last].valid = 1;
    }
}

/***************************************************************/
/*                                                             */
/* Procedure : cdump                                           */
/*                                                             */
/* Purpose   : Dump cache configuration                        */
/*                                                             */
/***************************************************************/
void cdump(int capacity, int assoc, int blocksize)
{

    printf("Cache Configuration:\n");
    printf("-------------------------------------\n");
    printf("Capacity: %dB\n", capacity);
    printf("Associativity: %dway\n", assoc);
    printf("Block Size: %dB\n", blocksize);
    printf("\n");
}

/***************************************************************/
/*                                                             */
/* Procedure : sdump                                           */
/*                                                             */
/* Purpose   : Dump cache stat                                 */
/*                                                             */
/***************************************************************/
void sdump(int total_reads, int total_writes, int write_backs,
           int reads_hits, int write_hits, int reads_misses, int write_misses)
{
    printf("Cache Stat:\n");
    printf("-------------------------------------\n");
    printf("Total reads: %d\n", total_reads);
    printf("Total writes: %d\n", total_writes);
    printf("Write-backs: %d\n", write_backs);
    printf("Read hits: %d\n", reads_hits);
    printf("Write hits: %d\n", write_hits);
    printf("Read misses: %d\n", reads_misses);
    printf("Write misses: %d\n", write_misses);
    printf("\n");
}


/***************************************************************/
/*                                                             */
/* Procedure : xdump                                           */
/*                                                             */
/* Purpose   : Dump current cache state                        */
/*                                                             */
/* Cache Design                                                */
/*                                                             */
/*      cache[set][assoc][word per block]                      */
/*                                                             */
/*                                                             */
/*       ----------------------------------------              */
/*       I        I  way0  I  way1  I  way2  I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set0  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*       I        I  word0 I  word0 I  word0 I                 */
/*       I  set1  I  word1 I  word1 I  work1 I                 */
/*       I        I  word2 I  word2 I  word2 I                 */
/*       I        I  word3 I  word3 I  word3 I                 */
/*       ----------------------------------------              */
/*                                                             */
/*                                                             */
/***************************************************************/
void xdump(cache* L)
{
    int i, j, k = 0;
    int b = L->b, s = L->s;
    int way = L->E, set = 1 << s;

    uint32_t line;

    printf("Cache Content:\n");
    printf("-------------------------------------\n");

    for(i = 0; i < way; i++) {
        if(i == 0) {
            printf("    ");
        }
        printf("      WAY[%d]", i);
    }
    printf("\n");

    for(i = 0; i < set; i++) {
        printf("SET[%d]:   ", i);

        for(j = 0; j < way; j++) {
            if(k != 0 && j == 0) {
                printf("          ");
            }
            if(L->sets[i].lines[j].valid) {
                line = L->sets[i].lines[j].tag << (s + b);
                line = line | (i << b);
            }
            else {
                line = 0;
            }
            printf("0x%08x  ", line);
        }
        printf("\n");
    }
    printf("\n");
}


int main(int argc, char *argv[])
{
    int capacity=1024;
    int way=8;
    int blocksize=8;
    int set;

    // Cache
    cache simCache;

    // Counts
    int read=0, write=0, writeback=0;
    int readhit=0, writehit=0;
    int readmiss=0, writemiss = 0;

    // Input option
    int opt = 0;
    char* token;
    int xflag = 0;

    // Parse file
    char *trace_name = (char*)malloc(32);
    FILE *fp;
    char line[16];
    char *op;
    uint32_t addr;

    /* You can define any variables that you want */

    trace_name = argv[argc-1];
    if (argc < 3) {
        printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
        exit(1);
    }

    while((opt = getopt(argc, argv, "c:x")) != -1) {
        switch(opt) {
            case 'c':
                token = strtok(optarg, ":");
                capacity = atoi(token);
                token = strtok(NULL, ":");
                way = atoi(token);
                token = strtok(NULL, ":");
                blocksize  = atoi(token);
                break;

            case 'x':
                xflag = 1;
                break;

            default:
                printf("Usage: %s -c cap:assoc:block_size [-x] input_trace \n", argv[0]);
                exit(1);

        }
    }

    // Allocate
    set = capacity / way / blocksize;

    /* TODO: Define a cache based on the struct declaration */
    simCache = build_cache(set, way, blocksize);

    // Simulate
    fp = fopen(trace_name, "r"); // read trace file
    if(fp == NULL) {
        printf("\nInvalid trace file: %s\n", trace_name);
        return 1;
    }

    cdump(capacity, way, blocksize);

    /* TODO: Build an access function to load and store data from the file */
    while (fgets(line, sizeof(line), fp) != NULL) {
        op = strtok(line, " ");
        addr = strtoull(strtok(NULL, ","), NULL, 16);

#ifdef DEBUG
        // You can use #define DEBUG above for seeing traces of the file.
        fprintf(stderr, "op: %s\n", op);
        fprintf(stderr, "addr: %x\n", addr);
#endif
        // ...
        if (!strcmp(op, "R"))
        {
            access_cache(&simCache, 0, addr, &readhit, &readmiss, &writeback);
            read++;
        }

        else if (!strcmp(op, "W"))
        {
            access_cache(&simCache, 1, addr, &writehit, &writemiss, &writeback);
            write++;
        }
        // read & wirte 나눠서
        time++;
        // ...
    }

    // test example
    sdump(read, write, writeback, readhit, writehit, readmiss, writemiss);
    if (xflag) {
        xdump(&simCache);
    }

    return 0;
}
