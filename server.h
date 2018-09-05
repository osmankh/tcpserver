#ifndef SERVER_H
#define SERVER_H
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#define CRLF		"\r\n"
#define BUF_SIZE	1024
#define MAX_CLIENTS 20
#include "client.h"
static void server(const char *port);
static int init_connection(const char *port);
static void end_connection(int sock);
static int read_client(SOCKET sock, char *buffer);
static void write_clients(const char *buffer, SOCKET sender_sock);
static void write_client(SOCKET sock, const char *buffer);
static void execute_command(const char *buffer);
static void render_server_input();
static void remove_client(int index);
static void print_startup_info(const char *port);
static void print_help();
static void kill_user(const char *buffer, const char kick_command);
static void checkHostName(int hostname);
static void checkHostEntry(struct hostent * hostentry);
static char * getHostIpAddress();
static void print_connected_users(int is_print_to_client, SOCKET sock);
static void execute_client_command(char *strcat, Client client);
static void quit_user(Client client);
static void clear_client(Client client);
static int get_client_index(Client client);
#endif /* guard */
