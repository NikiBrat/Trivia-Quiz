/*
    Name: Uros Nikolic
    Pledge: I pledge my honor that I have abided by the Stevens Honor System
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#define MAX_CONN 3
#define BUFFER_SIZE 2048

void usage(const char* cmd) 
{
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n\n   -i IP_address          Default to \"127.0.0.1\";\n   -p port_number         Default to 25555;\n   -h                     Display this help info.\n", cmd);
    exit(EXIT_FAILURE);
}

int preparefileDesc(fd_set* active_fds, int server_fd) 
{
    FD_ZERO(active_fds);
    FD_SET(server_fd, active_fds);
    FD_SET(STDIN_FILENO, active_fds);
    
    int max_fd = -1;
    if(server_fd > STDIN_FILENO) max_fd = server_fd;
    else max_fd = STDIN_FILENO;

    return max_fd;
}

void Connection(int argc, const char** argv, int* s_fd) 
{
    char * ip_addr = "127.0.0.1";
    int port = 25555;
    int iflg, pflg;

    int c;
    while((c = getopt(argc, (char* const*) argv, "f:i:p:h")) != -1) 
    {
        switch(c) 
        {
            case 'i':
                if(iflg) usage(argv[0]);
                iflg = 1;
                ip_addr = optarg;
                break;
            case 'p':
                if(pflg) usage(argv[0]);
                pflg = 1;
                port = atoi(optarg);
                usage(argv[0]);
                break;
            case 'h':
                usage(argv[0]);
                break;
            case '?':
                fprintf(stderr, "Error: Unknown option '-%c' received.\n", optopt);
                usage(argv[0]);
        }
    }

    struct sockaddr_in s_addr;
    socklen_t addr_size = sizeof(s_addr);
    *s_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&s_addr, 0, sizeof(s_addr));

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    if((s_addr.sin_addr.s_addr = inet_addr(ip_addr)) == (in_addr_t)(-1)) 
    {
        fprintf(stderr, "Error: Unable to connect to IP '%s'.\n", ip_addr);
        exit(EXIT_FAILURE);
    }

    if(connect(*s_fd, (struct sockaddr *) &s_addr, addr_size) < 0) 
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char const *argv[]) 
{
    int s_fd;
    fd_set fds;

    Connection(argc, argv, &s_fd);

    while(1) 
    {
        int max_fd = preparefileDesc(&fds, s_fd);

        select(max_fd+1, &fds, NULL, NULL, NULL);

        if(FD_ISSET(s_fd, &fds)) 
        {
            char buffer[BUFFER_SIZE] = "";
            if(read(s_fd, buffer, BUFFER_SIZE) <= 0) break;
            else 
            {
                printf("%s", buffer);
                fflush(stdout);
            }
        }

        if(FD_ISSET(STDIN_FILENO, &fds)) 
        {
            char buffer[BUFFER_SIZE] = "";
            fgets(buffer, BUFFER_SIZE, stdin);
            write(s_fd, buffer, strlen(buffer)-1);
        }
    }
    return 0;
}
