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

int main(int argc, char *argv[])
{
    assert(argc == 2);
    char *host = argv[1];

    struct hostent* hostinfo = gethostbyname(host);
    assert(hostinfo);

    struct servent* servinfo = getservbyname("daytime", "tcp");
    assert(servinfo);
    printf("daytime port is %d\n", ntohs(servinfo->s_port));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = servinfo->s_port;
    address.sin_addr = *(struct in_addr*)*(hostinfo->h_addr_list);

    int sock = socket(AF_INET, SOCK_STREAM,0);
    int result = connect(sock, (struct sockaddr*) &address, sizeof(address));
    assert(result != -1);

    char buffer[128];
    result = read(sock, buffer, sizeof(buffer));
    assert(result > 0);
    buffer[result] = '\0';
    printf("the day time is : %s", buffer);
    close(sock);

    return 0;
}
