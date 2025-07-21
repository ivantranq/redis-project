#include <sys/socket.h>
#include <arpa/inet.h> // sockaddr_in and s_addr
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <cassert>

//--------------------------------------------------
/// Global Variables
//--------------------------------------------------
const size_t k_max_msg = 4096;

//--------------------------------------------------
/// HELPER FUNCTIONS
//--------------------------------------------------
static void Die(const char * msg){
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}
static void Msg(const char * msg) { 
    fprintf(stderr, "%s\n", msg);
}

//-----------------------------------------------------------------
/// @brief This function grabs the connection, reads everything from 
/// that connection until it sees an EOF.
/// @param connFD the connection file descriptor
/// @param buf the buffer with the payload
/// @param n the size of the payload
/// @return write/read result
//-----------------------------------------------------------------
static int32_t ReadFull(int connFd, char* buf, size_t bufLen){
    while (bufLen > 0) {
        ssize_t numBytesRead = read(connFd, buf, bufLen);
        if (numBytesRead <= 0){
            return -1; // Error occured or EOF.
        }
        assert((size_t)numBytesRead <= bufLen);
        bufLen -= (size_t)numBytesRead;
        buf += numBytesRead;
    }
    return 0;
}


static int32_t WriteAll(int connFd, char* buf, size_t bufLen){
    while (bufLen > 0) {
        ssize_t numBytesWritten = write(connFd, buf, bufLen);
        if (numBytesWritten <= 0){
            return -1; // Error occured or EOF.
        }
        assert((size_t)numBytesWritten <= bufLen);
        bufLen -= (size_t)numBytesWritten;
        buf += numBytesWritten;
    }
    return 0;
}

//-----------------------------------------------------------------
/// @brief This function grabs the connection, reads everything from 
/// that connection until it sees an EOF, then writes back.
/// @param connFD the connection file descriptor
/// @return write/read result
//-----------------------------------------------------------------
static int32_t OneRequest(int connFd){
    char readBuf[4 + k_max_msg];
    errno = 0;

    int32_t err = ReadFull(connFd, readBuf, 4);
    if (err) {
        Msg(errno == 0 ? "EOF" : "ReadFull() error");
        return err;
    }

    uint32_t len = 0;
    memcpy(&len, readBuf, 4); //assumes little endian
    if (len > k_max_msg) {
        Msg("Message is too long");
        return -1;
    }

    // Request body
    err = ReadFull(connFd, &readBuf[4], len);
    if (err) {
        Msg("read() error");
        return err;
    }

    // Do something (just print for now)
    printf("client says : %.*s\n", len, &readBuf[4]);

    // Reply using same protocol
    const char reply[] = "world";
    char writeBuf[4 + sizeof(reply)];
    len = (uint32_t) strlen(reply);
    memcpy(writeBuf, &len, 4);
    memcpy(&writeBuf[4], reply, len);
    
    return WriteAll(connFd, writeBuf, 4 + len);
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

    // Main server loop
    while (true) {
        //accept
        struct sockaddr_in clientAddr = {};
        socklen_t addrLen = sizeof(clientAddr);
        int connFd = accept(fd, (sockaddr *)&clientAddr, &addrLen);
        
        if (connFd < 0) { continue; }

        while (true){
            int32_t err = OneRequest(connFd);
            if (err) { break; }
        }
        close(connFd);
    }

    return 0;
}