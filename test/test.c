#include <stdio.h>

int main(int argc, char *argv[]) {
    printf("Hello, world!\n");
    for (int i = 0; i < argc; i++) {
        printf("%s ", argv[i]);
    }
    printf("\n");
    return 0;
}
