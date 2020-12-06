#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

void ReadPort(int* port, int argc, char* argv[]) {
    if(argc == 1) {
        printf("Please run again with port in arguments.\n");
        _exit(1);
    } else {
        for(int i = 0; i < strlen(argv[1]); i++) {
            *port = *port * 10 + argv[1][i] - 48;
        }
    }
    
}

int main(int argc, char* argv[]) {
    int port = 0;
    ReadPort(&port, argc, argv);
    printf("port = %d\n", port);
}
