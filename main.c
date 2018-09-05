#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "server.h"
#include "client.h"

int connected = 0;
static Client clients[MAX_CLIENTS];
Client *client;
fd_set rdfs;
SOCKET server_sock;

void server(const char *port)
{
    server_sock = init_connection(port);
    char buffer[BUF_SIZE];
    int max = server_sock;
    while(1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(server_sock, &rdfs);

        if(connected > 0)
        {
            for (int i = 1; i <= connected ; i++) {
                FD_SET(clients[i].sock, &rdfs);
                if (max < clients[i].sock) {
                    max = clients[i].sock;
                }
            }
        }

        render_server_input();

        if(select(max + 1, &rdfs,NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        if(FD_ISSET(STDIN_FILENO, &rdfs))
        {
            fgets(buffer, BUF_SIZE - 1, stdin);
            {
                char *p = NULL;
                p = strstr(buffer, "\n");
                if(p != NULL)
                {
                    *p = 0;
                }
                else
                {
                    buffer[BUF_SIZE - 1] = 0;
                }
            }
            execute_command(buffer);
        }
        else if(FD_ISSET(server_sock, &rdfs))
        {
            SOCKADDR_IN csin = { 0 };
            size_t sinsize = sizeof csin;
            int csock = accept(server_sock, (SOCKADDR *)&csin, (socklen_t *) &sinsize);
            if(csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }
            if(read_client(csock, buffer) == -1)
            {
                continue;
            }

            max = csock > max ? csock : max;
            FD_SET(csock, &rdfs);
            Client c = { csock };
            strncpy(c.name, buffer, BUF_SIZE - 1);
            connected++;
            printf("\r[+] %s Connected\n", c.name);
            clients[connected] = c;

            int onlineStringSize = sizeof("[+]  is online.");
            char str[sizeof(c.name) + onlineStringSize];
            strcpy(str, "[+] ");
            write_clients(strcat(strcat(str, c.name), " is online."), c.sock);
        }
        else {
            if (connected > 0) {
                for (int i = 1; i <= connected; i++) {
                    Client client = clients[i];
                    if (FD_ISSET(client.sock, &rdfs)) {
                        int c = read_client(client.sock, buffer);
                        if (c == 0) {
                            remove_client(i);
                        } else {
                            execute_client_command(buffer, client);
                        }
                        break;
                    }
                }
            }
        }
    }
}

static void remove_client(int index)
{
    Client client = clients[index];
    char name[BUF_SIZE];
    strcpy(name, client.name);
    write_client(client.sock, "[+] Sorry You're Kicked Out by the Server!");
    clear_client(client);
    int onlineStringSize = sizeof("[+]  is disconnected.");
    char str[sizeof(name) + onlineStringSize];
    strcpy(str, "[+] ");
    write_clients(strcat(strcat(str, name), " is disconnected."), -1);
}

static void clear_client(Client client) {
    end_connection(client.sock);
    memmove(&client,&client + 1, 0);
    int index = get_client_index(client);
    for(int i = index; i < MAX_CLIENTS - 1; i++) clients[i] = clients[i + 1];
    connected--;
}

static int get_client_index(Client client) {
    for (int i = 0; i < connected; ++i) {
        if (client.sock == clients[i].sock){
            return i;
        }
    }
}
static void execute_command (const char *buffer) {
    char* kick_command = malloc(6);
    strncpy(kick_command, buffer, 5);

    if (strncmp(buffer, ":help", strlen(":help")) == 0) {
        print_help();
    } else if (strncmp(buffer, ":shutdown", strlen(":shutdown")) == 0) {
        printf("Bye.\n");
        end_connection(server_sock);
        exit(1);
    } else if (strncmp(buffer, ":who", strlen(":who")) == 0) {
        print_connected_users(0, -1);
    } else if (strncmp(kick_command, ":kill", strlen(":kill")) == 0) {
        kill_user(buffer, *kick_command);
    } else {
        printf("[+] %s, command not found! type :help to see available commands.\n", buffer);
    }
}

void execute_client_command(char *command, Client client) {
    if (strncmp(command, ":who", strlen(":who")) == 0) {
        print_connected_users(1, client.sock);
    } else if (strncmp(command, ":quit", strlen(":quit")) == 0) {
        quit_user(client);
    } else {
        char str[sizeof(client.name) + sizeof(command)];
        strcpy(str, client.name);
        write_clients(strcat(strcat(str, "\t> "), command), client.sock);
    }
}

void quit_user(Client client){
    write_client(client.sock, "Bye.");
    char str[BUF_SIZE];
    sprintf(str, "[+] %s is Disconnected.", client.name);
    write_clients(str, client.sock);
    clear_client(client);
}

static void print_connected_users(int is_print_to_client, SOCKET socket) {
    int buff_size = 50;

    for (int i = 1; i <= connected; i++) {
        buff_size += sizeof(clients[i].name);
    }

    char str[buff_size];

    if (connected == 0){
        strcpy(str, "\r[+] There is no connected users.\n");
    } else {
        strcpy(str, "\r[+] Connected Users are:\n");
        for (int i = 1; i <= connected; i++) {
            char tmp[10 + sizeof(clients[i].name)];
            sprintf(tmp, "\t%d) %s\n", i, clients[i].name);
            strcat(str, tmp);
        }
    }

    if (is_print_to_client > 0) {
        write_client(socket, str);
    } else {
        puts(str);
    }
}

void print_startup_info(const char *port){
    printf("[+] Starting the server.....\n");
    printf("[+] The Server has successfully started at %s:%s\n", getHostIpAddress(), port);
    printf("[+] Type :help to list all available commands.\n");
}

static void print_help() {
    printf("\r\n[HELP] TCP Server\n");
    printf("Usage: server <port>...\n");
    printf("Options:\n");
    printf("\t:help\t\tDisplay this information.\n");
    printf("\t:who\t\tPrint a list of connected users.\n");
    printf("\t:kill <username>\t\tKick a user out.\n");
    printf("\t:shutdown\t\tDisconnect all users and shutdown the server.\n\n");
    printf("Author: Osman KHODER.\n");
    printf("Email: osman.khoder@isae.edu.lb\n");
    printf("Version: 1.2.\n\n");
    render_server_input();
}

static void kill_user(const char *buffer, const char kick_command) {
    int name_size = (sizeof(buffer) - sizeof(kick_command) - 1);
    char* username = malloc((size_t) name_size);
    strncpy(username, buffer + 6, (size_t) name_size);

    int exists = 0;

    for (int i = 1; i <= connected ; i++) {
        Client client = clients[i];
        if (strcmp(client.name, username) == 0) {
            remove_client(i);
            exists = 1;
            break;
        }
    }


    if (exists == 1) {
        printf("\r[+] %s successfully disconnected.\n", username);
    } else {
        printf("\r[+] %s not found.\n", username);
    }
}

static void render_server_input() {
    printf("\rSERVER\t> ");
    fflush(stdout);
}

static int init_connection(const char *port)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    if(sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons((uint16_t) *port);
    sin.sin_family = AF_INET;
    if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }
    if(listen(sock,1) == SOCKET_ERROR)
    {
        perror("listen()");
        exit(errno);
    }

    print_startup_info(port);

    return sock;
}

char * getHostIpAddress() {
    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    checkHostEntry(host_entry);

    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr*)
            host_entry->h_addr_list[0]));

    return IPbuffer;
}

// Returns hostname for the local computer
void checkHostName(int hostname)
{
    if (hostname == -1)
    {
        perror("gethostname");
        exit(1);
    }
}

// Returns host information corresponding to host name
void checkHostEntry(struct hostent * hostentry)
{
    if (hostentry == NULL)
    {
        perror("gethostbyname");
        exit(1);
    }
}

static void end_connection(int sock)
{
    closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
    int n = 0;
    if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
    {
        perror("recv()");
        n = 0;
    }
    buffer[n] = 0;
    return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
    if(send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

static void write_clients(const char *buffer, SOCKET sender_sock){
    for (int i = 1; i <= connected; i++) {
        Client client = clients[i];
        if (client.sock == sender_sock)
            continue;
        if (client.sock == 0) continue;
        write_client(client.sock, buffer);
    }
}

int main(int argc, char **argv)
{
    if(argc < 2)
    {
        printf("Usage : %s <port>...\n", argv[0]);
        return EXIT_FAILURE;
    }
    server(argv[1]);
    return EXIT_SUCCESS;
}
