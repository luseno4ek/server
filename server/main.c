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
    int user_number;
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
    char buf[128];
    const ssize_t size = sprintf(buf,  "--[Server]: chat server is running (%d users are connected...)\n", users->size);
    for(int i = 0; i < users->size; i++) {
        if(users->user[i].name != NULL) {
            write(users->user[i].socket, buf, size);
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
    for(int i = 0; i < add_s->size; i++) {
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

int GetUniqueNumber(struct UsersFD* users) {
    bool unique = false;
    int i = 1;
    while(!unique) {
        unique = true;
        for(int j = 0; j < users->size; j++) {
            if(i == users->user[j].user_number) {
                unique = false;
                i++;
                break;
            }
        }
    }
    return i;
}

void AppendUsersFD(struct UsersFD* users, int socket_serv) {
    if(users->size == users->capacity) {
        users->capacity *= 2;
        users->user = (struct User*) realloc(users->user, users->capacity * sizeof(struct User));
    }
    int res = accept(socket_serv, NULL, NULL);
    if(res == -1) {
        printf("ERROR: fail to accept user\n");
        _exit(3);
    }
    users->user[users->size].socket = res;
    users->user[users->size].user_number = GetUniqueNumber(users);
    users->user[users->size].name = NULL;
    InitString(&(users->user[users->size].message));
    printf("# user_%d joined the chat\n", users->user[users->size].user_number);
    users->size++;
}

void DeleteUser(struct UsersFD* users, int leaving_user) {
    int leaving_socket = users->user[leaving_user].socket;
    shutdown(leaving_socket, 2);
    close(leaving_socket);
    free(users->user[leaving_user].name);
    FreeString(&(users->user[leaving_user].message));
    users->user[leaving_user] = users->user[users->size - 1];
    users->size--;
}

//___________________Greeting_and_FareWell:___________________

char* CreateName(int number) {
    char* leaving_name = (char*) malloc(12 * sizeof(char));
    sprintf(leaving_name, "user_%d", number);
    return leaving_name;
}

void SayBye(struct UsersFD* users, int leaving_user) {
    char* leaving_name = users->user[leaving_user].name;
    if(leaving_name == NULL) {
        leaving_name = CreateName(users->user[leaving_user].user_number);
    }
    printf("# %s left the chat\n", leaving_name);
    char buf[128];
    const int size = sprintf(buf, "--[Server]: %s left the chat.\n", leaving_name);
    for(int i = 0; i < users->size; i++) {
        if((users->user[i].name != NULL) && (i != leaving_user)) {
            const int current_socket = users->user[i].socket;
            write(current_socket, buf, size);
        }
    }
}

void Greetings(struct UsersFD* users, int index) {
    char* new_name = users->user[index].name;
    printf("# user_%d changed name to '%s'\n", users->user[index].user_number, new_name);
    char buf[128];
    const int size = sprintf(buf,  "--[Server]: Welcome %s!\n", new_name);
    char buf1[128];
    const int size1 = sprintf(buf1, "--[Server]: %s, welcome to our chat!\n", new_name);
    for(int i = 0; i < users->size; i++) {
        if(users->user[i].name != NULL) {
            const int current_socket = users->user[i].socket;
            if(users->user[index].user_number == users->user[i].user_number) {
                write(current_socket, buf1, size1);
            } else {
                write(current_socket, buf, size);
            }
        }
    }
}

//___________________Name_Changing:___________________

bool IsSymbolOK(char c) {
    return (c >= 'a' && c <= 'z') || c == '_' || (c >= 'A' && c <= 'Z');
}

bool IsNameCorrect(char* buf, int size) {
    if((size > 16) || (size < 3)) {
        return false;
    }
    for(int i = 0; i < size; i++) {
        if(!IsSymbolOK(buf[i])) {
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
    const int user_socket = users->user[index].socket;
    const int size = GetStringSize(buf);
    if(IsNameCorrect(buf, size)) {
        if(!IsNameMatched(users, buf)) {
            users->user[index].name = buf;
            users->user[index].name_size = size;
            Greetings(users, index);
            return;
        } else {
            static const char msg[] = "--[Server]: This name is already taken\n";
            write(user_socket, msg, strlen(msg));
        }
    } else {
        static const char msg[] =  "--[Server]: Invalid name\n";
        write(user_socket, msg, strlen(msg));
    }
    static const char msg[] = "--[Server]: Choose your name: ";
    write(user_socket, msg, strlen(msg));
}

//___________________New_User:___________________

bool IsServerFull(struct UsersFD* users) {
    const int MAX_CAPACITY = 3;
    if(users->size > MAX_CAPACITY) {
        static const char msg [] = "--[Server]: Sorry the chat is full\n";
        write(users->user[users->size - 1].socket, msg, strlen(msg));
        printf("# user_%d is removed, because the chat is full\n", users->size);
        DeleteUser(users, users->size - 1);
        return true;
    }
    return false;
}

void AcceptUser(struct UsersFD* users, fd_set* set, int socket_serv) {
    if(FD_ISSET(socket_serv, set)) {
        AppendUsersFD(users, socket_serv);
        const int user_socket = users->user[users->size - 1].socket;
        if(!IsServerFull(users)) {
            static const char msg[] = "--[Server]: Choose your name: ";
            write(user_socket, msg, strlen(msg));
        }
    }
}

//___________________Init_Server:___________________

int ReadPort(int argc, char* argv[]) {
    int port = 0;
    if(argc == 1) {
        return -1;
    }
    for(int i = 0; i < strlen(argv[1]); i++) {
        if(argv[1][i] < '0' || argv[1][i] > '9') {
            return -1;
        }
        port = port * 10 + argv[1][i] - '0';
    }
    return port;
}

int InitServer(int port) {
    const int socket_serv = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_serv == -1) {
        return -1;
    }
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    const int bind_res = bind(socket_serv, (struct sockaddr*) &addr, sizeof(addr));
    if(bind_res == -1) {
        printf("ERROR: Fail to bind socket, try again\n");
        return -2;
    }
    const int ls = listen(socket_serv, 5);
    if(ls == -1) {
        printf("ERROR: Fail to listen socket, try again\n");
        return -3;
    }
    printf("# chat is running at port %d\n", port);
    return socket_serv;
}

bool IsPortCorrect(int port) {
    if(port < 1 || port > 65535) {
        printf("Run again with port value in arguments.\n");
        return false;
    }
    return true;
}

bool IsServerInitCorrectly(int socket_serv) {
    if(socket_serv == -1) {
        printf("ERROR: Fail to create a socket, try again\n");
        return false;
    }
    if(socket_serv == -2) {
        printf("ERROR: Fail to bind socket, try again\n");
        return false;
    }
    if(socket_serv == -3) {
        printf("ERROR: Fail to listen socket, try again\n");
        return false;
    }
    return true;
}

//___________________Sending_Message:___________________

bool UserWannaLeave(char* buf) {
    if(IsStrEq("bye!", buf)) {
        return true;
    }
    return false;
}

void CleanMessage(struct UsersFD* users, int sender, int index) {
    const char* buf = users->user[sender].message.data;
    const ssize_t size = users->user[sender].message.size;
    for(int i = 0; i < size - index; i++) {
        users->user[sender].message.data[i] = buf[i + index];
    }
    users->user[sender].message.capacity = (int) size - index;
    users->user[sender].message.data = (char*) realloc(users->user[sender].message.data, users->user[sender].message.capacity * sizeof(char));
    users->user[sender].message.size = (int) size - index;
}

void SendMessage(struct UsersFD* users, int sender, char* message) {
    const int message_size = GetStringSize(message);
    if(users->user[sender].name == NULL) {
        SelectName(users, sender, message);
        return;
    }
    char* sender_name = users->user[sender].name;
    int name_size = users->user[sender].name_size;
    printf("[%s]: %s\n", sender_name, message);
    char* buf = malloc((5 + name_size + message_size) * sizeof(char));
    const int full_size = sprintf(buf, "[%s]: %s\n", sender_name, message);
    for(int i = 0; i < users->size; i++) {
        const int current_socket = users->user[i].socket;
        if((i != sender) && (users->user[i].name != NULL)) {
            write(current_socket, buf, full_size);
        }
    }
    free(buf);
}

void ParceMessage(struct UsersFD* users, int sender) {
    struct String one_message;
    struct String full_message = users->user[sender].message;
    one_message.data = malloc(sizeof(char) * full_message.size);
    int index = 0;
    for(int i = 0; i < full_message.size; i++) {
        one_message.data[i - index] = full_message.data[i];
        if(one_message.data[i - index] == '\n') {
            one_message.size = i - index;
            char* message = GetString(&one_message);
            if(UserWannaLeave(message)) {
                SayBye(users, sender);
                DeleteUser(users, sender);
                FreeString(&one_message);
                return;
            }
            SendMessage(users, sender, message);
            index = i + 1;
            one_message.data = malloc(sizeof(char) * full_message.size - index);
        }
    }
    FreeString(&one_message);
    CleanMessage(users, sender, index);
}

//___________________Reading_Message:___________________

bool IsEndOfMessage(struct String* s) {
    if(s->data[s->size - 1] == '\r') {
        return true;
    }
    return false;
}

bool ReadString(struct UsersFD* users, int index) {
    int user_socket = users->user[index].socket;
    struct String s;
    InitString(&s);
    ssize_t read_res = read(user_socket, s.data, s.capacity);
    s.size = read_res;
    if(read_res == 0) {
        SayBye(users, index);
        DeleteUser(users, index);
        FreeString(&s);
        return false;
    } else {
        AppendString(&(users->user[index].message), &s);
        return true;
    }
}

void ReadFromUsers(struct UsersFD* users, fd_set* set) {
    for(int i = 0; i < users->size; i++) {
        int current_socket = users->user[i].socket;
        if(FD_ISSET(current_socket, set)) {
            bool UserSendAMessage = ReadString(users, i);
            if(UserSendAMessage) {
                ParceMessage(users, i);
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
    int port = ReadPort(argc, argv);
    if(!IsPortCorrect(port)) {
        return 1;
    }
    int socket_serv = InitServer(port);
    if(!IsServerInitCorrectly(socket_serv)) {
        return 1;
    }
    RunServer(socket_serv);
}
