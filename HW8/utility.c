#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

char digits[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void reverse(char s[]) {
    int i, j;
    char c;

    for (i = 0, j = strlen(s) - 1; i < j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

void safe_write(char *str) {
    size_t slen = strlen(str);
    ssize_t t = 0;
    ssize_t cur = 0;
    while ((cur = write(STDERR_FILENO, &(str[t]), slen - t)) > 0) {
        t += cur;
    }
    if (cur == -1) {
        exit(errno);
    }
}

void write_num10(long long int n) {
    char s[32];
    memset(s, 0, 32);
    long long sign;
    int i;
    if ((sign = n) < 0)
        n = -n;
    i = 0;
    do {
        s[i++] = digits[n % 10];
    } while ((n /= 10) > 0);
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);
    safe_write(s);
}

void write_num16(long long int n) {
    char s[17];
    memset(s, 0, 17);
    for (size_t i = 0; i < 16; ++i) {
        s[15 - i] = digits[(n & (((unsigned long long) 15) << (i * 4))) >> (i * 4)];
    }
    safe_write("0x");
    int i;
    for (i = 0; i < 15 && s[i] == '0'; ++i);
    safe_write(&(s[i]));
}

void write_num16char(char n) {
    char s[3];
    memset(s, 0, 3);
    for (size_t i = 0; i < 2; ++i) {
        s[1 - i] = digits[(n & (((unsigned long long) 1) << (i * 4))) >> (i * 4)];
    }
    safe_write("0x");
    int i;
//    for (i = 0; i < 1 && s[i] == '0'; ++i);
    safe_write(&(s[i]));
}

void hprintf(char *str, long long int num) {
    safe_write(str);
    safe_write(": ");
    write_num10(num);
    safe_write(" [");
    write_num16(num);
    safe_write("]\n");
}
