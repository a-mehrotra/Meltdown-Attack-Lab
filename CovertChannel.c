#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/mman.h>
#include <sys/stat.h>

#define SIZE 4096
// Used threshold found in Lab 5
#define THRESHOLD 166
#define NUM_TRACES 100000

char *addr;
size_t offset;
void *target;
int s;

uint8_t data;
uint8_t *userArray;

void clflush(volatile void* Tx) {
    asm volatile("lfence;clflush (%0) \n" :: "c" (Tx));
}

static __inline__ uint64_t timer_start (void) {
    unsigned cycles_low, cycles_high;
    asm volatile ("CPUID\n\t"
                  "RDTSC\n\t"
                  "mov %%edx, %0\n\t"
                  "mov %%eax, %1\n\t": "=r" (cycles_high), "=r" (cycles_low)::
            "%rax", "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ uint64_t timer_stop (void) {
    unsigned cycles_low, cycles_high;
    asm volatile("RDTSCP\n\t"
                 "mov %%edx, %0\n\t"
                 "mov %%eax, %1\n\t"
                 "CPUID\n\t": "=r" (cycles_high), "=r" (cycles_low):: "%rax",
    "%rbx", "%rcx", "%rdx");
    return ((uint64_t)cycles_high << 32) | cycles_low;
}

static __inline__ void maccess(void *p) {
    asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
}

uint32_t reload(void *target) {
    volatile uint32_t time;
    uint64_t t1, t2;
    t1 = timer_start();
    maccess(target);
    t2 = timer_stop();
    return t2 - t1;
}

void *map_offset(const char *file, size_t offset) {
    int fd = open(file, O_RDONLY);
    if (fd < 0)
        return NULL;

    char *mapaddress = (char*) mmap(0, sysconf(_SC_PAGE_SIZE), PROT_READ, MAP_PRIVATE, fd, offset & ~(sysconf(_SC_PAGE_SIZE) -1));
    close(fd);
    if (mapaddress == MAP_FAILED)
        return NULL;
    return (void *)(mapaddress+(offset & (sysconf(_SC_PAGE_SIZE) -1)));
}

void unmap_offset(void *address) {
    munmap((char *)(((uintptr_t)address) & ~(sysconf(_SC_PAGE_SIZE) -1)),
           sysconf(_SC_PAGE_SIZE));
}

int retrieveData(int x) {

    for(int i = 0; i < 256; i++) {
        clflush(userArray + (i * SIZE));
    }

    maccess(userArray  + (x * SIZE));

    uint32_t timing = 0;
    uint8_t recievedData = 0;

    for(int i = 0; i < 256; i++){
        timing = reload(userArray + (i * SIZE));
        if(timing < THRESHOLD){
            recievedData = i;
            break;
        }
    }

    return recievedData;
}

int getCovertChannel() {
    userArray = (uint8_t *) malloc(sizeof(uint8_t) * SIZE * 256) ; // 256 memory pages

    data = 4;
    int retrievedData;

    int i;

    memset(userArray, 1, sizeof(uint8_t) * SIZE * 256);


    int correct = 0;
    for (i = 0; i < NUM_TRACES; i++){
        data = rand() % 256;
        retrievedData = retrieveData(data);
        if(retrievedData == data){
            correct++;
        }
    }

    return correct;
}

int main() {
    srand(time(NULL));
    float ratio;
    int percentCorrect = getCovertChannel();
    ratio = (float) percentCorrect / (float) NUM_TRACES;
    printf("Hits: %d\n", percentCorrect);
    printf("Num traces: %d\n", NUM_TRACES);
    printf("The correctness ratio is %f\n", ratio);
    return 0;
}