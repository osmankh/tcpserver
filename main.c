#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "server.h"
#include "client.h"

int connected = 0;
static Client client;
void server(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    int max = sock;
    fd_set rdfs;
    while(1)
    {
        FD_ZERO(&rdfs);
        FD_SET(STDIN_FILENO, &rdfs);
        FD_SET(sock, &rdfs);
        if(connected)
        {
            FD_SET(client.sock, &rdfs);
        }
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
            if(connected && send(client.sock, buffer, strlen(buffer), 0) < 0)
            {
                perror("send()");
                exit(errno);
            }
        }
        else if(FD_ISSET(sock, &rdfs))
        {
            SOCKADDR_IN csin = { 0 };
            size_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, (socklen_t *) &sinsize);
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
            client=c;
            connected=1;
        }
        else
        {
            if(connected)
            {
                if(FD_ISSET(client.sock, &rdfs))
                {
                    int c = read_client(client.sock, buffer);
                    if(c == 0)
                    {
                        closesocket(client.sock);
                        remove_client(&client);
                        puts("Client déconnecté");
                    }
                    else
                    {
                        printf("%s : ",client.name);
                        puts(buffer);
                    }
                }
            }
        }
    }

    clear_client(&client);
    end_connection(sock);
}

static void clear_client(Client *client)
{
    if(connected)
    {
        closesocket(client->sock);
    }
}

static void remove_client(Client *client)
{
    memmove(&client,&client + 1, 0);
    connected--;
}

static int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = { 0 };
    if(sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
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
    return sock;
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

int main(int argc, char **argv)
{
    server();
    return EXIT_SUCCESS;
}
