#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

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

void InitServer(int* socket_serv, int port) {
    *socket_serv = -1;
    while(*socket_serv == -1) {
        *socket_serv = socket(AF_INET, SOCK_STREAM, 0);
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    int bind_res = bind(*socket_serv, (struct sockaddr*) &addr, sizeof(addr));
    if(bind_res == -1) {
        printf("ERROR: Fail to bind socket, try again\n");
        _exit(2);
    }
    int ls = listen(*socket_serv, 5);
    if(ls == -1) {
        printf("ERROR: Fail to listen socket, try again\n");
        _exit(2);
    }
}

int main(int argc, char* argv[]) {
    int port = 0;
    ReadPort(&port, argc, argv);
    int socket_serv;
    InitServer(&socket_serv, port);
}
