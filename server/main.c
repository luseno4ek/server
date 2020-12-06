#include <stdio.h>

void ReadPort(int* port, int argc, char** argv) {
    if(argc == 1) {
        printf("Please run again with port in arguments\n");
    } else {
        char* temp = argv[1];
        for(int i = 0; temp[i] != ''; i++) {
            
        }
    }
    
}

int main(int argc, char* argv[]) {
    int port;
    ReadPort(&port, argc, argv);
}
