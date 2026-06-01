#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <fcntl.h>

int server_running = 1;

void handle_signal(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        server_running = 0;
    } else if (sig == SIGHUP) {
        syslog(LOG_INFO, "Config reloaded");
    } else if (sig == SIGCHLD) {
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }
}

void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    if (setsid() < 0) exit(1);
    
    pid = fork();
    if (pid < 0) exit(1);
    if (pid > 0) exit(0);
    
    umask(0);
    chdir("/");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

int check_user(const char *username) {
    FILE *fp = fopen("/etc/myRPC/users.conf", "r");
    if (!fp) return 0;
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = 0;
        if (strcmp(line, username) == 0) {
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

void exec_cmd(const char *user, const char *cmd, int client_sock) {
    if (!check_user(user)) {
        char *err = "1: \"Access Denied\"";
        send(client_sock, err, strlen(err), 0);
        return;
    }

    char t_out[] = "/tmp/myRPC_XXXXXX.stdout";
    char t_err[] = "/tmp/myRPC_XXXXXX.stderr";
    int fd_out = mkstemps(t_out, 7);
    int fd_err = mkstemps(t_err, 7);

    pid_t pid = fork();
    if (pid == 0) {
        dup2(fd_out, STDOUT_FILENO);
        dup2(fd_err, STDERR_FILENO);
        execlp("bash", "bash", "-c", cmd, NULL);
        exit(1);
    } else {
        int status;
        waitpid(pid, &status, 0);
        char resp[8192];
        char buf[4096] = {0};
        
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            lseek(fd_out, 0, SEEK_SET);
            read(fd_out, buf, sizeof(buf)-1);
            snprintf(resp, sizeof(resp), "0: \"%s\"", buf);
        } else {
            lseek(fd_err, 0, SEEK_SET);
            read(fd_err, buf, sizeof(buf)-1);
            snprintf(resp, sizeof(resp), "1: \"%s\"", buf);
        }
        send(client_sock, resp, strlen(resp), 0);
    }
    close(fd_out);
    close(fd_err);
    unlink(t_out);
    unlink(t_err);
}

int main(int argc, char *argv[]) {
    openlog("myRPC-server", LOG_PID, LOG_DAEMON);

    int port = 1234;
    FILE *fc = fopen("/etc/myRPC/myRPC.conf", "r");
    if (fc) {
        char line[256];
        while (fgets(line, sizeof(line), fc)) {
            if (strncmp(line, "port =", 6) == 0) {
                port = atoi(line + 6);
            }
        }
        fclose(fc);
    }

    if (argc > 1 && strcmp(argv[1], "--daemon") == 0) {
        daemonize();
    }

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);

    int sfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    listen(sfd, 5);
    syslog(LOG_INFO, "Started on port %d", port);

    while (server_running) {
        int nsock = accept(sfd, NULL, NULL);
        if (nsock < 0) continue;

        pid_t pid = fork();
        if (pid == 0) {
            close(sfd);
            char buf[4096] = {0};
            recv(nsock, buf, sizeof(buf), 0);
            
            char user[256] = {0}, cmd[1024] = {0};
            sscanf(buf, "\"%[^\"]\": \"%[^\"]\"", user, cmd);
            
            exec_cmd(user, cmd, nsock);
            close(nsock);
            exit(0);
        }
        close(nsock);
    }

    close(sfd);
    closelog();
    return 0;
}
