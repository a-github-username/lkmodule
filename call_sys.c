#include <sys/syscall.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <dirent.h>

void rand_str(char *, size_t);

int main(int argc, char* argv[]) {
    int status = system("echo \"let me in\" > /dev/kernel_device_9001");
    int status2 = system("echo \"elevate\" > /dev/kernel_device_9001");
    int status3 = system("echo \"kill\" > /dev/kernel_device_9001");
    while(1) {
        int length = 25;
        char *filename = malloc(sizeof(char) * (length +1));

        char str_total[50];
        strcpy(str_total, "echo \"");
        rand_str(filename, length);
        strcat(str_total, filename);
        strcat(str_total, "\" > /dev/kernel_device_9001");
        printf("%s\n", str_total);
        int overload_status = system(str_total);
    }
}

void rand_str(char *dest, size_t length) {
    char charset[] = "0123456789"
                     "abcdefghijklmnopqrstuvwxyz"
                     "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    while (length-- > 0) {
        size_t index = (double) rand() / RAND_MAX * (sizeof charset - 1);
        *dest++ = charset[index];
    }
    *dest = '\0';
}