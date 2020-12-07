#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

//___________________Types:___________________

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

//___________________Helpful_Functions:___________________

bool IsStrEq(char* s1, char* s2) {
    while(*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

char* GetString(char* buf, int* size) {
    char* buf2 = (char*) malloc(*size * sizeof(char));
    int i;
    for(i = 0; buf[i] != '\r'; i++) {
        buf2[i] = buf[i];
    }
    buf2[i] = '\0';
    *size = i;
    return buf2;
}

//___________________Chat_State:___________________

void SendChatState(struct UsersFD* users) {
    printf("# chat server is running (%d users are connected...)\n", users->size);
    char* size = (char*) malloc(2 * sizeof(char));
    size[0] = users->size + 48;
    size[1] = '\0';
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        write(current_socket, "--[Server]: chat server is running (", 36);
        write(current_socket, size, 2);
        write(current_socket, " users are connected)\n", 22);
    }
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

char* CreateName(struct UsersFD* users) {
    char* leaving_name = (char*) malloc(7 * sizeof(char));
    leaving_name[0] = 'u';
    leaving_name[1] = 's';
    leaving_name[2] = 'e';
    leaving_name[3] = 'r';
    leaving_name[4] = '_';
    leaving_name[5] = users->size + 48;
    leaving_name[6] = '\0';
    return leaving_name;
}

void SayBye(struct UsersFD* users, int leaving_user) {
    char* leaving_name = users->user[leaving_user].name;
    int name_size = users->user[leaving_user].name_size;
    if(leaving_name == NULL) {
        leaving_name = CreateName(users);
        name_size = 7;
    }
    printf("# %s left the chat/n", leaving_name);
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        write(current_socket, "--[Server]: ", 12);
        write(current_socket, leaving_name, name_size);
        write(current_socket, " left the chat.\n", 16);
    }
}

void Greetings(struct UsersFD* users) {
    char* new_name = users->user[users->size - 1].name;
    int name_size = users->user[users->size - 1].name_size;
    printf("# user_%d changed name to '%s'\n", users->size, new_name);
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        write(current_socket, "--[Server]: Welcome", 19);
        if(i == users->size - 1) {
            write(current_socket, ",", 1);
        }
        write(current_socket, " ", 1);
        write(current_socket, new_name, name_size);
        write(current_socket, "!\n", 2);
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
        printf("# user_%d joined the chat\n", users->size);
    }
}

void DeleteUser(struct UsersFD* users, int leaving_user) {
    int leaving_socket = users->user[leaving_user].socket;
    shutdown(leaving_socket, 2);
    close(leaving_socket);
    for(int i = leaving_user - 1; i < users->size; i++) {
        users->user[i] = users->user[i + 1];
    }
    users->size--;
}

//___________________New_User:___________________

bool IsNameCorrect(char* buf, int size) {
    if(size == 17) {
        return false;
    }
    for(int i = 0; i < size; i++) {
        if(((buf[i] > 64) && (buf[i] < 91)) || (buf[i] == 95) || ((buf[i] > 96) && (buf[i] < 123))) {
        } else {
            return false;
        }
    }
    return true;
}

bool IsNameMatched(struct UsersFD* users, char* name) {
    for(int i = 0; i < users->size; i++) {
        if(IsStrEq(users->user[i].name, name)) {
            return true;
        }
    }
    return false;
}

void SelectName(struct UsersFD* users) {
    int user_socket = users->user[users->size - 1].socket;
    bool CORRECTNAME = false;
    while(!CORRECTNAME) {
        write(user_socket, "--[Server]: Choose your name: ", 30);
        char* buf = (char*) malloc(17 * sizeof(char));
        size_t read_res = read(user_socket, buf, 17);
        if(read_res == 0) {
            DeleteUser(users, users->size - 1);
            SayBye(users, users->size - 1);
        } else {
            int size;
            buf = GetString(buf, &size);
            if(IsNameCorrect(buf, size)) {
                if(!IsNameMatched(users, buf)) {
                    users->user[users->size - 1].name = buf;
                    users->user[users->size - 1].name_size = size;
                } else {
                    write(user_socket, "--[Server]: This name is already taken\n", 39);
                    free(buf);
                    continue;
                }
            } else {
                write(user_socket, "--[Server]: Invalid name\n", 25);
                free(buf);
            }
        }
    }
    Greetings(users);
}

void AcceptUser(struct UsersFD* users, fd_set* set, int socket_serv) {
    if(FD_ISSET(socket_serv, set)) {
        AppendUsersFD(users, socket_serv);
        SelectName(users);
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
