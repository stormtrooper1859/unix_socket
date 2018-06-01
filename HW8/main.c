#define _GNU_SOURCE

#include <features.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/ucontext.h>
#include <bits/types/stack_t.h>
#include <bits/types/sigset_t.h>
#include <bits/types/siginfo_t.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "utility.h"

#define WTF (-610)
#define num_of_reg 18
#define DELTA 40

char *a;

volatile sig_atomic_t first_time = 1;
volatile sig_atomic_t cur_pos = 0;

char *reg[num_of_reg] = {"rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp", "r8", "r9", "r10", "r11",
                         "r12", "r13", "r14", "r15", "rip", "eflags"};
int regnum[num_of_reg] = {REG_RAX, REG_RBX, REG_RCX, REG_RDX, REG_RSI, REG_RDI, REG_RBP, REG_RSP, REG_R8,
                          REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15, REG_RIP, REG_EFL};

static void handler(int signum, siginfo_t *info, void *ptr) {
    if (first_time) {

        ucontext_t *uc = ptr;

        hprintf("Access violation at address", (long long int) info->si_addr);

        safe_write("\nValue of general purpose registers:\n");

        for (int i = 0; i < num_of_reg; ++i) {
            hprintf(reg[i], uc->uc_mcontext.gregs[regnum[i]]);
        }

        hprintf("ss", uc->uc_mcontext.gregs[REG_CSGSFS] >> 48);
        hprintf("fs", uc->uc_mcontext.gregs[REG_CSGSFS] << 16 >> 48);
        hprintf("gs", uc->uc_mcontext.gregs[REG_CSGSFS] << 32 >> 48);
        hprintf("cs", uc->uc_mcontext.gregs[REG_CSGSFS] << 48 >> 48);

        hprintf("fctrl", uc->uc_mcontext.fpregs->cwd);

        safe_write("\nValue of XMM registers:\n");

        for (int i = 0; i < 16; ++i) {
            safe_write("xmm");
            write_num10(i);
            safe_write(": ");

            for (int j = 0; j < 4; ++j) {
                write_num10(uc->uc_mcontext.fpregs->_xmm[i].element[j]);
                safe_write(" ");
            }

            safe_write("[");
            for (int j = 0; j < 4; ++j) {
                if (j != 0) safe_write(" ");
                write_num16(uc->uc_mcontext.fpregs->_xmm[i].element[j]);
            }
            safe_write("]\n");
        }

        safe_write("\nValue of float point registers:\n");

        for (int i = 0; i < 8; ++i) {
            safe_write("st");
            write_num10(i);
            safe_write(": [raw ");

            write_num16(uc->uc_mcontext.fpregs->_st->exponent);

            for (int j = 0; j < 4; ++j) {
                safe_write(" ");
                write_num16(uc->uc_mcontext.fpregs->_st->significand[j]);
            }
            safe_write("]\n");
        }

        safe_write("\nMemory dump:");
        cur_pos = 0;
    }

    //print memory dump
    char *failaddress;
    if (first_time) {
        failaddress = info->si_addr - DELTA;
        first_time = 0;
    } else {
        failaddress = info->si_addr;
        safe_write("___ ");
    }

    if (cur_pos >= 2 * DELTA) {
        safe_write("\n");
        exit(EXIT_FAILURE);
    }

    do {
        if ((cur_pos) % 8 == 0) safe_write("\n");
        cur_pos += 1;
        failaddress += 1;
        write_num16(*failaddress);
        safe_write(" ");
    } while (cur_pos < 2 * DELTA);

    safe_write("\n");

    exit(EXIT_FAILURE);
}

void make_fall() {
//    a = 121231350;
//    a[0] = 1;
    int t = 0;
    a = malloc(1);
    for (int i = 0; i < 4096 * 16; ++i) {
        t += a[-i];
    }
}

int main() {
    struct sigaction sa;

    sa.sa_sigaction = handler;
    sa.sa_flags = SA_SIGINFO | SA_NODEFER;

    sigaction(SIGSEGV, &sa, NULL);

    make_fall();

    return 0;
}

