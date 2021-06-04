#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define LB_ADDRESS "127.0.0.1"
#define LB_PORT 80
#define SERVERS_PORT 80
#define SERVERS_COUNT 3

typedef struct ServerConnection {
    char server_name[10];
    char server_address[15];
    int lb_server_socket;
    int load;
    int delta;
    int new_load;
    
} *ServerConnection;

void printServerConnections(ServerConnection servers_connections[]) {
    for(int i = 0; i < SERVERS_COUNT; i++) {
        printf("server_name: %s\n", servers_connections[i]->server_name);
        printf("server_address: %s\n", servers_connections[i]->server_address);
        printf("lb_server_socket: %d\n", servers_connections[i]->lb_server_socket);
        printf("load: %d\n", servers_connections[i]->load);
        printf("delta: %d\n", servers_connections[i]->delta);
        printf("new_load: %d\n", servers_connections[i]->new_load);
        printf("\n");
    }
}

int chooseServer(ServerConnection servers_connections[], char buffer[]);
void initServerConnections(ServerConnection servers_connections[]);

int main() {
    printf("before Connect To Servers\n");
    // ------------------------------- Connect To Servers -------------------------------
    ServerConnection servers_connections[SERVERS_COUNT];
    initServerConnections(servers_connections);
    printServerConnections(servers_connections);
    printf("before Listen To Clients\n");
    // ------------------------------- Listen To Clients -------------------------------
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    /* Create server socket */
    if (master_socket == -1) {
        fprintf(stderr, "Error creating socket --> %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /*creating server address*/
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));/* Zeroing server address struct */
    server_addr.sin_family = AF_INET;//filling it
    inet_pton(AF_INET, LB_ADDRESS, &(server_addr.sin_addr)); // or : server_addr.sin_addr.s_addr = INADDR_ANY
    server_addr.sin_port = htons(LB_PORT);

    /* Bind */
    if ((bind(master_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1){
        fprintf(stderr, "Error on bind --> %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("before Listening to incoming connections\n");
    /* Listening to incoming connections allowing 5 connections */
    if ((listen(master_socket, 5)) == -1) {
        fprintf(stderr, "Error on listen --> %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    printf("before Accept Clients\n");
    // ------------------------------- Accept Clients -------------------------------
    socklen_t sock_len = sizeof(struct sockaddr_in);
    struct sockaddr_in client_addr;
    char buffer[256];
    while(1) {
        fprintf(stdout, "stdout Before Accept\n");
        fprintf(stdout, "stderr Before Accept\n");
        int client_socket = accept(master_socket, (struct sockaddr *)&client_addr, &sock_len);
        fprintf(stderr, "stderr After Accept\n");

        if (client_socket == -1) {
            fprintf(stderr, "Error on accept --> %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        fprintf(stdout, "Accept peer --> %s\n", inet_ntoa(client_addr.sin_addr));

        if(fork() == 0) {
            int data_len = recv(client_socket, buffer, sizeof(buffer), 0);
            if(data_len < 0) {
                fprintf(stderr, "Error on receiving command --> %s", strerror(errno));
                exit(EXIT_FAILURE);
            }

            ServerConnection server_conn = servers_connections[chooseServer(servers_connections,buffer)];
            send(server_conn->lb_server_socket, buffer, sizeof(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            recv(server_conn->lb_server_socket, buffer, sizeof(buffer), 0);

            send(client_socket, buffer, sizeof(buffer), 0);
            close(master_socket);
        } else {
            close(client_socket);
        }
    }
//    close(master_socket);
//    return 0;
}

int chooseServer(ServerConnection servers_connections[], char buffer[]) {
    return 0;
    // int delta = 0;
    // if (buffer[0] == 'M') {
    //     servers_connections[0]->delta = 2 * (buffer[1] - '0');
    //     servers_connections[1]->delta = 2 * (buffer[1] - '0');
    //     servers_connections[2]->delta = 1 * (buffer[1] - '0');
    // }
    // if (buffer[0] == 'V') {
    //     servers_connections[0]->delta = 1 * (buffer[1] - '0');
    //     servers_connections[1]->delta = 1 * (buffer[1] - '0');
    //     servers_connections[2]->delta = 3 * (buffer[1] - '0');
    // }
    // if (buffer[0] == 'P') {
    //     servers_connections[0]->delta = 1 * (buffer[1] - '0');
    //     servers_connections[1]->delta = 1 * (buffer[1] - '0');
    //     servers_connections[2]->delta = 2 * (buffer[1] - '0');
    // }

    // int server_index = 0;
    // int min_load = INT_MAX;
    // int min_delta = INT_MAX;
    // for (int i = 0; i < SERVERS_COUNT; ++i) {
    //     servers_connections[i]->new_load = servers_connections[i]->load + servers_connections[i]->delta;
    //     if (servers_connections[i]->new_load < min_load) {
    //         min_load = servers_connections[i]->new_load;
    //     } else if ((servers_connections[i]->new_load == min_load) && (servers_connections[i]->delta < min_delta)) {
    //         min_delta = servers_connections[i]->delta;
    //     }
    //     server_index = i;
    // }

    // servers_connections[server_index]->load += servers_connections[server_index]->delta;
    // return server_index;
}












int createLBServerSocket(const char* server_address) {
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, server_address, &(server_addr.sin_addr));
    server_addr.sin_port = htons(SERVERS_PORT);

    /* Create client socket */
    int lb_server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (lb_server_socket == -1) {
        fprintf(stderr, "Error creating socket --> %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    /* Connect to the server */
    if (connect(lb_server_socket, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
        fprintf(stderr, "Error on connect --> %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return lb_server_socket;
}

void initServerConnections(ServerConnection servers_connections[]) {
    for(int i = 0; i < SERVERS_COUNT; i++) {
        servers_connections[i] = (ServerConnection)malloc(sizeof(struct ServerConnection));
        char servNumber = (char)i + '1';
        char server_name[] = "serv$";
        server_name[4] = servNumber;
        char server_address[] = "192.168.0.10$";
        server_address[12] = servNumber;
        printf("before strcpy\n");
        strcpy(servers_connections[i]->server_name, server_name);
        strcpy(servers_connections[i]->server_address, server_address);
        printf("after strcpy\n");
        servers_connections[i]->load = 0;
        servers_connections[i]->delta = 0;
        servers_connections[i]->new_load = 0;
        servers_connections[i]->lb_server_socket = createLBServerSocket(server_address);
    }
}