#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

//___________________Run_Server:___________________

struct User {
    int socket;
    char* name;
    int name_size;
};

struct UsersFD {
    struct User* user;
    int size;
    int capacity;
};

//___________________Chat_State:___________________

void SendChatState(struct UsersFD* users) {
    
}

//___________________Small_Functions:___________________

int ResetSet(struct UsersFD* users, fd_set* set, int socket_serv) {
    int max_d = socket_serv;
    FD_ZERO(set);
    FD_SET(socket_serv, set);
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        FD_SET(current_socket, set);
        if(current_socket > max_d) {
            max_d = current_socket;
        }
    }
    return max_d;
}

void SelectFromSet(struct UsersFD* users, fd_set* set, int max_d) {
    struct timeval time;
    time.tv_sec = 7;
    time.tv_usec = 0;
    int select_res = select(max_d + 1, set, NULL, NULL, &time);
    if(select_res == 0) {
        SendChatState(users);
    }
}

//___________________UsersFD:___________________

void InitUsersFD(struct UsersFD* users) {
    users->size = 0;
    users->capacity = 2;
    users->user = (struct User*) malloc(sizeof(struct User) * users->capacity);
}

void AppendUsersFD(struct UsersFD* users, int socket_serv) {
    users->size++;
    if(users->size == users->capacity) {
        users->capacity *= 2;
        users->user = (struct User*) realloc(users->user, users->capacity * sizeof(struct User));
    }
    int res = accept(socket_serv, NULL, NULL);
    if(res == -1) {
        printf("ERROR: fail to accept user\n");
        _exit(3);
    } else {
        users->user[users->size - 1].socket = res;
    }
}

//___________________New_User:___________________

void AcceptUser(struct UsersFD* users, fd_set* set, int socket_serv) {
    if(FD_ISSET(socket_serv, set)) {
        AppendUsersFD(users, socket_serv);
    }
}

//___________________Run_Server:___________________

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
    printf("# chat is running at port %d\n", port);
}

void RunServer(int socket_serv) {
    struct UsersFD users;
    InitUsersFD(&users);
    while(true) {
        fd_set set;
        int max_d = ResetSet(&users, &set, socket_serv);
        SelectFromSet(&users, &set, max_d);
        AcceptUser(&users, &set, socket_serv);
    }
}

int main(int argc, char* argv[]) {
    int port = 0;
    ReadPort(&port, argc, argv);
    int socket_serv;
    InitServer(&socket_serv, port);
    RunServer(socket_serv);
}
