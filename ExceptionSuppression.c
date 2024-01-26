#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <signal.h>
#define NUM_SAMPLES 100000

static __inline__ void maccess(void *p) {
    asm volatile("movq (%0), %%rax\n" : : "c"(p) : "rax");
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

int main(int argc, char** argv) {
    uint32_t *test_illegal_address;
    if (signal(SIGSEGV, segfault_handler) == SIG_ERR) {
        printf("Failed to setup signal handler\n");
        return -1;
    }
    int nFault = 0;
    for(int i = 0; i < NUM_SAMPLES; i++){
        test_illegal_address += 4096;

        if (!setjmp(buffer)){
            // perform potentially invalid memory access
            maccess(test_illegal_address);
            continue;
        }
        nFault++;
    }
    printf("exception suppressed: %i\n", nFault);
}