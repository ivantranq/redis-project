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
const size_t HEADER_BYTES_LEN = 4;

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

static int32_t Query(int connFd, const char* text) {
    uint32_t len = (uint32_t) strlen(text);
    if (len > k_max_msg) {
        return -1;
    }

    // Send request
    char writeBuf[4 + k_max_msg];
    memcpy(writeBuf, &len, HEADER_BYTES_LEN); // assumes little endian
    memcpy(&writeBuf[HEADER_BYTES_LEN], text, len);

    int32_t err = WriteAll(connFd, writeBuf, HEADER_BYTES_LEN + len);
    if (err) return err;

    // 4 bytes header
    char readBuf[HEADER_BYTES_LEN + k_max_msg + 1];
    errno = 0;
    err = ReadFull(connFd, readBuf, HEADER_BYTES_LEN);
    if (err) {
        Msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    memcpy(&len, readBuf, HEADER_BYTES_LEN);
    if (len > k_max_msg) {
        Msg("Too Long");
        return -1;
    }

    // reply body
    err = ReadFull(connFd, &readBuf[HEADER_BYTES_LEN], len);
    if (err) {
        Msg("read() error");
        return err;
    }
    // Do something with the response (print for now)
    printf("server says: %.*s\n", len, &readBuf[HEADER_BYTES_LEN]);
    return 0;
}

int main() {
    int connFd = socket(AF_INET, SOCK_STREAM, 0);
    if (connFd < 0) Die("socket()");

    // Create a socket address
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // Connect to socket
    int failed = connect(connFd, (const sockaddr *) &addr, sizeof(addr));
    if (failed) Die("connect()");

    // Send multiple requests
    auto err = Query(connFd, "hello1");
    if (err) goto L_DONE;

    err = Query(connFd, "hello2");
    if (err) goto L_DONE;

    L_DONE:

    close(connFd);
    return 0;

}