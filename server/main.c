#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

//___________________Types:___________________

struct String {
    char* data;
    int capacity;
    ssize_t size;
};

struct User {
    int socket;
    char* name;
    int name_size;
    struct String message;
};

struct UsersFD {
    struct User* user;
    int size;
    int capacity;
};

//___________________Helpful_Functions:___________________

bool IsStrEq(char* s1, char* s2) {
    if((s1 == NULL) || (s2 == NULL)) {
        return false;
    }
    while(*s1 != '\0' && *s2 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 == *s2;
}

int GetStringSize(char* buf) {
    int i = 0;
    for(i = 0; buf[i] != '\0'; i++) {}
    return i;
}

//___________________Chat_State:___________________

void SendChatState(struct UsersFD* users) {
    printf("# chat server is running (%d users are connected...)\n", users->size);
    char* size = (char*) malloc(2 * sizeof(char));
    size[0] = users->size + 48;
    size[1] = '\0';
    for(int i = 0; i < users->size; i++) {
        if(users->user[i].name != NULL) {
            int current_socket = users->user[i].socket;
            write(current_socket, "--[Server]: chat server is running (", 36);
            write(current_socket, size, 2);
            write(current_socket, " users are connected)\n", 22);
        }
    }
}

//___________________Set_Usage:___________________

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

//_________________String_Usage:_________________//

void InitString(struct String* s) {
    s->size = 0;
    s->capacity = 32;
    s->data = (char*) malloc(s->capacity * sizeof(char));
}

void FreeString(struct String* s) {
    free(s->data);
}

void AppendString(struct String* s, struct String* add_s) {
    s->capacity = (int) add_s->size + (int) s->size;
    s->data = (char*) realloc(s->data, s->capacity * sizeof(char));
    for(int i = 0; i <= add_s->size; i++) {
        s->data[s->size + i] = add_s->data[i];
    }
    s->size += add_s->size;
    FreeString(add_s);
}

char* GetString(struct String* s) {
    (s->data)[s->size - 1] = '\0';
    return s->data;
}

//___________________UsersFD_Usage:___________________

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
        users->user[users->size - 1].name = NULL;
        InitString(&(users->user[users->size - 1].message));
        printf("# user_%d joined the chat\n", users->size);
    }
}

void DeleteUser(struct UsersFD* users, int leaving_user) {
    int leaving_socket = users->user[leaving_user].socket;
    shutdown(leaving_socket, 2);
    close(leaving_socket);
    free(users->user[leaving_user].name);
    FreeString(&(users->user[leaving_user].message));
    for(int i = leaving_user; i < users->size; i++) {
        users->user[i] = users->user[i + 1];
    }
    users->size--;
}

//___________________Greeting_and_FareWell:___________________

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
    printf("# %s left the chat\n", leaving_name);
    for(int i = 0; i < users->size; i++) {
        if((users->user[i].name != NULL) && (i != leaving_user)) {
            int current_socket = users->user[i].socket;
            write(current_socket, "--[Server]: ", 12);
            write(current_socket, leaving_name, name_size);
            write(current_socket, " left the chat.\n", 16);
        }
    }
}

void Greetings(struct UsersFD* users, int index) {
    char* new_name = users->user[index].name;
    int name_size = users->user[index].name_size;
    printf("# user_%d changed name to '%s'\n", index + 1, new_name);
    for(int i = 0; i < users->size; i++) {
        if(users->user[i].name != NULL) {
            int current_socket = users->user[i].socket;
            write(current_socket, "--[Server]: Welcome", 19);
            if(i == index) {
                write(current_socket, ",", 1);
            }
            write(current_socket, " ", 1);
            write(current_socket, new_name, name_size);
            write(current_socket, "!\n", 2);
        }
    }
}

//___________________Name_Changing:___________________

bool IsNameCorrect(char* buf, int size) {
    if((size > 16) || (size < 3)) {
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

void SelectName(struct UsersFD* users, int index, char* buf) {
    int user_socket = users->user[index].socket;
    int size = GetStringSize(buf);
    if(IsNameCorrect(buf, size)) {
        if(!IsNameMatched(users, buf)) {
            users->user[index].name = buf;
            users->user[index].name_size = size;
            Greetings(users, index);
            return;
        } else {
            write(user_socket, "--[Server]: This name is already taken\n", 39);
        }
    } else {
        write(user_socket, "--[Server]: Invalid name\n", 25);
    }
    write(user_socket, "--[Server]: Choose your name: ", 30);
}

//___________________New_User:___________________

bool IsServerFull(struct UsersFD* users) {
    const int MAX_CAPACITY = 3;
    if(users->size > MAX_CAPACITY) {
        write(users->user[users->size - 1].socket, "--[Server]: Sorry the chat is full\n", 36);
        printf("# user_%d is removed, because the chat is full\n", users->size);
        DeleteUser(users, users->size - 1);
        return true;
    }
    return false;
}

void AcceptUser(struct UsersFD* users, fd_set* set, int socket_serv) {
    if(FD_ISSET(socket_serv, set)) {
        AppendUsersFD(users, socket_serv);
        int user_socket = users->user[users->size - 1].socket;
        if(!IsServerFull(users)) {
            write(user_socket, "--[Server]: Choose your name: ", 30);
        }
    }
}

//___________________Init_Server:___________________

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

//___________________Sending_Message:___________________

bool UserWannaLeave(char* buf) {
    if(IsStrEq("bye!", buf)) {
        return true;
    }
    return false;
}

void SendMessage(struct UsersFD* users, int sender, char* message) {
    int size = GetStringSize(message);
    if(users->user[sender].name == NULL) {
        SelectName(users, sender, message);
        return;
    }
    if(UserWannaLeave(message)) {
        SayBye(users, sender);
        DeleteUser(users, sender);
        return;
    }
    char* sender_name = users->user[sender].name;
    int name_size = users->user[sender].name_size;
    printf("[%s]: %s\n", sender_name, message);
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        if((i != sender) && (users->user[i].name != NULL)) {
            write(current_socket, "[", 1);
            write(current_socket, sender_name, name_size);
            write(current_socket, "]: ", 3);
            write(current_socket, message, size);
            write(current_socket, "\n", 1);
        }
    }
}

//___________________Reading_Message:___________________

bool IsEndOfMessage(struct String* s) {
    if(s->data[s->size - 1] == '\r') {
        return true;
    }
    return false;
}

char* ReadString(struct UsersFD* users, int index) {
    int user_socket = users->user[index].socket;
    struct String s;
    InitString(&s);
    ssize_t read_res = read(user_socket, s.data, s.capacity);
    s.size = read_res - 1;
    if(read_res == 0) {
        SayBye(users, index);
        DeleteUser(users, index);
        FreeString(&s);
        return NULL;
    } else {
        AppendString(&(users->user[index].message), &s);
        if(IsEndOfMessage(&s)) {
            char* message = GetString(&(users->user[index].message));
            return message;
        } else {
            return NULL;
        }
    }
}

void ReadFromUsers(struct UsersFD* users, fd_set* set) {
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        if(FD_ISSET(current_socket, set)) {
            char* buf = ReadString(users, i);
            if(buf != NULL) {
                SendMessage(users, i, buf);
                InitString(&(users->user[i].message));
            }
        }
    }
}

//___________________Run_Server:___________________

void RunServer(int socket_serv) {
    struct UsersFD users;
    InitUsersFD(&users);
    while(true) {
        fd_set set;
        int max_d = ResetSet(&users, &set, socket_serv);
        SelectFromSet(&users, &set, max_d);
        AcceptUser(&users, &set, socket_serv);
        ReadFromUsers(&users, &set);
    }
}

int main(int argc, char* argv[]) {
    int port = 0;
    ReadPort(&port, argc, argv);
    int socket_serv;
    InitServer(&socket_serv, port);
    RunServer(socket_serv);
}
