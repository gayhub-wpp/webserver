#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char const *argv[])
{
    if(argc <= 2){
        printf("usage: %s ip_address port_number filename\n", basename(argv[0]));   //basename(argv[0]：文件名  
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &server_address.sin_addr);
    server_address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(sockfd>=0);

    int ret = bind(sockfd, (struct sockaddr*)&server_address, sizeof(server_address));
    assert(ret != -1);

    ret = listen(sockfd, 5);
    assert(ret != 5);

    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    int connfd = accept(sockfd, (struct sockaddr*)&client, &client_addrlength);
    if (connfd < 0)
    {
        printf("errno is : %d\n", errno);
    }
    else{
        printf("1\n");
        int pipefd[2];
        assert(ret != -1);
        ret = pipe(pipefd);
        ret = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);
        ret = splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        assert(ret != -1);

        close(connfd);
    }

    close(sockfd);
    return 0;
}