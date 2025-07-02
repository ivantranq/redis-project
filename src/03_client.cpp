#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in and s_addr
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void Die(char * msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    // Obtain a socket handle
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    // Create a socket address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Connect to socket
    int failed = connect(fd, (const sockaddr *) &addr, sizeof(addr));
    if (failed) { Die("connect"); }

    //Do Something
    char msg[] = "hello";
    write(fd, msg, strlen(msg));

    char readBuffer[64] = {};
    ssize_t n = read(fd, readBuffer, sizeof(readBuffer) - 1);
    if (n < 0) { Die("read"); }
    printf("Server says: %s \n", readBuffer);

    // Close
    close(fd);

    return 0;
}

