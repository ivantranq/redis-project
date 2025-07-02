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

static void Msg(char * msg) { 
    fprintf(stderr, "%s\n", msg);
}

static void DoSomething(int connFd) {
    // Read
    char readBuffer [64] = {};
    ssize_t n = read(connFd, &readBuffer, sizeof(readBuffer) - 1);
    if (n < 0) { 
        Msg("read() error");
        return;
    }
    printf("Client says: %s\n", readBuffer);

    // Write
    char writeBuffer[] = "world";
    write(connFd, writeBuffer, strlen(writeBuffer));
}

int main() {
    // Obtain a socket handle
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Set socket options
    int val = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

    // Create a socket address
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind to socket address
    int failed = bind(fd, (const sockaddr *) &addr, sizeof(addr));
    if (failed) { Die("bind()"); }

    // Listen to socket address
    failed = listen(fd, SOMAXCONN);
    if (failed) { Die("listen()"); }

    while (true) {
        // Accept 
        struct sockaddr_in clientAddr = {};
        socklen_t addrLen = sizeof(clientAddr);
        int connFd = accept(fd, (sockaddr * ) &clientAddr, &addrLen);
        if (connFd < 0) { continue; }

        //Do Something
        DoSomething(connFd);

        // Close
        close(connFd);
    }    

    return 0;
}

