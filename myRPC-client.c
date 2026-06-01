#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <pwd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main(int argc, char *argv[]) {
    int opt;
    char *command = NULL;
    char *host = "127.0.0.1";
    int port = 1234;
    int is_stream = 1;

    struct option long_options[] = {
        {"command", required_argument, 0, 'c'},
        {"host",    required_argument, 0, 'h'},
        {"port",    required_argument, 0, 'p'},
        {"stream",  no_argument,       0, 's'},
        {"dgram",   no_argument,       0, 'd'},
        {"help",    no_argument,       0, 0},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "c:h:p:sd", long_options, NULL)) != -1) {
        switch (opt) {
            case 'c': command = optarg; break;
            case 'h': host = optarg; break;
            case 'p': port = atoi(optarg); break;
            case 's': is_stream = 1; break;
            case 'd': is_stream = 0; break;
            case 0:
                printf("Usage: %s -c \"cmd\" [-h ip] [-p port] [-s|-d]\n", argv[0]);
                exit(0);
            default: exit(1);
        }
    }

    if (!command) exit(1);

    struct passwd *pw = getpwuid(getuid());
    if (!pw) exit(1);

    char request[4096];
    snprintf(request, sizeof(request), "\"%s\": \"%s\"", pw->pw_name, command);

    int sock;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);

    char response[8192] = {0};

    if (is_stream) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) exit(1);
        send(sock, request, strlen(request), 0);
        recv(sock, response, sizeof(response) - 1, 0);
        close(sock);
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        sendto(sock, request, strlen(request), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
        socklen_t len = sizeof(server_addr);
        recvfrom(sock, response, sizeof(response) - 1, 0, (struct sockaddr *)&server_addr, &len);
        close(sock);
    }

    printf("%s\n", response);
    return 0;
}
