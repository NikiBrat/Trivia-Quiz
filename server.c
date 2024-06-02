/*
    Name: Uros Nikolic
    Pledge: I pledge my honor that I have abided by the Stevens Honor System
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#define MAX_CONN 3
#define BUFFER_SIZE 2048


struct Entry 
{
    char prompt[1024];
    char options[3][50];
    int answer_idx;
};


struct Player 
{
    int fd;
    int score;
    char name[128];
};

void usage(const char* cmd) 
{
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n   -f question_file       Default to \"question.txt\";\n   -i IP_address          Default to \"127.0.0.1\";\n   -p port_number         Default to 25555;\n   -h                     Display this help info.\n", cmd);
    exit(EXIT_FAILURE);
}

//Prepare file descriptor set for accepting all active connections
int preparefileDesc(fd_set* active_fileDesc, int server_fd, int* client_fileDesc) 
{
    FD_ZERO(active_fileDesc);
    FD_SET(server_fd, active_fileDesc);
    
    int max_fd = server_fd;

    for(int i = 0; i < MAX_CONN; i++) 
    {
        if(client_fileDesc[i] > -1) FD_SET(client_fileDesc[i], active_fileDesc);
        if(client_fileDesc[i] > max_fd) max_fd = client_fileDesc[i];
    }

    return max_fd;
}

    //Read questions from file
int read_questions(struct Entry* arr, const char* filename) 
{
    FILE *file;
    if((file = fopen(filename, "r")) < 0) 
    {
        perror("open");
        exit(EXIT_FAILURE);
    }

    int done = 0;
    char buffer[BUFFER_SIZE];
    for(int i = 0; i < 50; i++) 
    {
        if(fgets(buffer, BUFFER_SIZE, file) == NULL) 
        {
            fclose(file);
            return 0;
        }

        strncpy((arr[i].prompt), buffer, strlen(buffer)-1);
        buffer[0] = 0;
        char* token;
        char* search = " ";
        char* buffer_ptr = buffer;
        if(fgets(buffer, BUFFER_SIZE, file) == NULL) 
        {
            fclose(file);
            return 0;
        }

        for(int j = 0; j < 3; j++) 
        {
            token = strtok(buffer_ptr, search);
            strcpy(arr[i].options[j], token);
            buffer_ptr += strlen(token)+1;
        }

        arr[i].options[2][strlen(token)-1] = 0;
        buffer[0] = 0;


        if(fgets(buffer, BUFFER_SIZE, file) == NULL) 
        {
            fclose(file);
            return 0;
        }

        buffer[strlen(buffer)-1] = 0;
        for(int j = 0; j < 3; j++) 
        {
            int cmp = strcmp(buffer, arr[i].options[j]);
            if(cmp == 0 || cmp == 13 || cmp == -13) arr[i].answer_idx = j;
        }

        fgets(buffer, BUFFER_SIZE, file); 
        if(feof(file)) 
        {
            fclose(file);
            return ++i; 
        }
    }
}

int main(int argc, char const *argv[]) 
{
    char * filename = "questions.txt";
    char * ip_addr = "127.0.0.1";
    int port = 25555;
    int fflag, iflag, pflag;
    int server_fd, client_fd;


    int flag;
    while((flag = getopt(argc, (char* const*) argv, ":f:i:p:h")) != -1) 
    {
        switch(flag) 
        {
            case 'f': 
                if(fflag) usage(argv[0]);
                fflag = 1;
                filename = optarg;
                break;
            case 'i': 
                if(iflag) usage(argv[0]);
                iflag = 1;
                ip_addr = optarg;
                break;
            case 'p': 
                if(pflag) usage(argv[0]);
                pflag = 1;
                port = atoi(optarg);
                
                if(port < 1024 || port > 49151) 
                {
                    fprintf(stderr, "Error: Invalid port '%s'.\n", optarg);
                    usage(argv[0]);
                }
                break;
            case 'h': 
                usage(argv[0]);
                break;
            case '?': 
                fprintf(stderr, "Error: Unknown option '-%d' received.\n", optopt);
                usage(argv[0]);
        }
    }

    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    {
        fprintf(stderr, "Error: Unable to open socket. %s.\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    struct sockaddr_in in_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    memset(&s_addr, 0, addr_size);

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port);
    if((s_addr.sin_addr.s_addr = inet_addr(ip_addr)) == (in_addr_t)(-1)) 
    {
        fprintf(stderr, "Error: Unable to connect to IP '%s'.\n", ip_addr);
        exit(EXIT_FAILURE);
    }

    bind(server_fd, (struct sockaddr *) &s_addr, addr_size);

    fd_set fileDesc;
    int clientFileDesc[MAX_CONN];
    struct Player players[MAX_CONN];
    int n_conn;

    for(int i = 0; i < MAX_CONN; i++) 
    {
        clientFileDesc[i] = -1;
        players[i].fd = -1;
    }

if(listen(server_fd, MAX_CONN) < 0) 
{
    perror("listen");
    exit(EXIT_FAILURE);
}

printf("Welcome to 392 Trivia!\n");
printf("Lobby size: 0/3 players\n");

int numberQ;
struct Entry questions[50];
if((numberQ = read_questions(questions, filename)) == 0) 
{
    fprintf(stderr, "Error: Failed to parse '%s'.\n", filename);
    exit(EXIT_FAILURE);
}

int done = 0;
while(!done) 
{
    int max_fd = preparefileDesc(&fileDesc, server_fd, clientFileDesc);

    select(max_fd+1, &fileDesc, NULL, NULL, NULL);

    if(FD_ISSET(server_fd, &fileDesc)) 
    {
        client_fd = accept(server_fd, (struct sockaddr*) &in_addr, &addr_size);
        if(n_conn == MAX_CONN) 
        {
            close(client_fd);
            fprintf(stderr, "Max connection reached!\n");
        } 
        else 
        {
            struct Player p;
            for(int i = 0; i < MAX_CONN; i++) 
            {
                if(clientFileDesc[i] == -1) 
                {
                    clientFileDesc[i] = client_fd;

                    p = players[i];
                    p.fd = client_fd;
                    p.score = 0;
                    strcpy(p.name, "");

                    break;
                }
            }
            printf("New connection detected!\n");
            write(p.fd, "Please type your name: ", 24);
            n_conn++;
        }
    }

    for(int i = 0; i < MAX_CONN; i++) 
    {
        if(clientFileDesc[i] > -1 && FD_ISSET(clientFileDesc[i], &fileDesc)) 
        {
            char buffer[BUFFER_SIZE] = "";
            if(read(clientFileDesc[i], buffer, BUFFER_SIZE) == 0) 
            { 
                close(clientFileDesc[i]);
                FD_CLR(clientFileDesc[i], &fileDesc);

                clientFileDesc[i] = -1;
                players[i].fd = -1;
                players[i].score = 0;
                players[i].name[0] = 0;

                printf("Lost connection!\n");
                n_conn--;
            }
            else 
            { 
                strcpy(players[i].name, buffer);
                sprintf(buffer, "Hi %s!\n", players[i].name);
                printf("Lobby size: %d/3 players!\n", n_conn);
                write(clientFileDesc[i], buffer, strlen(buffer));
                printf("%s", buffer);
                fflush(stdout);
            }
        }

        for(int j = 0; j < MAX_CONN; j++) 
        {
            if(strlen(players[j].name) == 0) break;
            if(j == MAX_CONN-1) done = 1;
        }
    }
}

printf("The game starts now!\n");

int questionNumber = 0;
int waiting = 0;

while(1) 
{
    int max_fd = preparefileDesc(&fileDesc, server_fd, clientFileDesc);

    if(!waiting) 
    {
        if(questionNumber == numberQ) break;

        printf("Question %d: %s\n1: %s\n2: %s\n3: %s\n", questionNumber+1, questions[questionNumber].prompt, questions[questionNumber].options[0], questions[questionNumber].options[1], questions[questionNumber].options[2]);

        for(int i = 0; i < MAX_CONN; i++) 
        {
            char buffer[BUFFER_SIZE] = "";
            sprintf(buffer, "Question %d: %s\nPress 1: %s\nPress 2: %s\nPress 3: %s\n", questionNumber+1, questions[questionNumber].prompt, questions[questionNumber].options[0], questions[questionNumber].options[1], questions[questionNumber].options[2]);
            write(clientFileDesc[i], buffer, strlen(buffer));
        }

        waiting = 1;
    }

    select(max_fd+1, &fileDesc, NULL, NULL, NULL);

    for(int i = 0; i < MAX_CONN; i++) 
    {
        if(clientFileDesc[i] > -1 && FD_ISSET(clientFileDesc[i], &fileDesc)) 
        {
            char buffer[BUFFER_SIZE] = "";
            if(read(clientFileDesc[i], buffer, BUFFER_SIZE) == 0) 
            {
                close(clientFileDesc[i]);
                printf("Lost connection!\n");

                for(int j = 0; j < i; j++) 
                {
                    if(j != i) close(clientFileDesc[i]);
                }

                exit(EXIT_FAILURE);
            } 
            else 
            {
                if(atoi(buffer) == questions[questionNumber].answer_idx+1) 
                {
                    (players[i].score)++;
                }
                else 
                {
                    (players[i].score)--;
                }

                char buffer2[BUFFER_SIZE*2] = "";
                
                sprintf(buffer2, "Answer from %s: %s\tCorrect answer: %d\n", players[i].name, buffer, questions[questionNumber].answer_idx+1);
                
                for(int j = 0; j < MAX_CONN; j++) 
                {
                    write(clientFileDesc[j], buffer2, strlen(buffer2));
                }
                printf("%s", buffer2);
                fflush(stdout);

                questionNumber++;
                waiting = 0;
            }
        }
    }
}

int top_score = INT_MIN;
char* winner = "";
for(int i = 0; i < MAX_CONN; i++) 
{
    if(players[i].score > top_score) 
    {
        top_score = players[i].score;
        winner = players[i].name;
    }
}


    printf("Congrats, %s!\n", winner);
    return 0;
}
