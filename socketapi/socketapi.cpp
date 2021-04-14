#include <sys/types.h>
#include <sys/socket.h>

// creat socket
int socket( int domain, int type, int protocol);

// name socket
int bind(int sockfd, const struct sockaddr* my_addr, socklen_t addrlen);

//listen socket
int listen(int sockfd, int backlog);

 //accept socket
 int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

 //connect socket
 int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);

 //close socket
 int close(int fd);

 //shutdown socket
 int shutdown(int sockfd, int howto);