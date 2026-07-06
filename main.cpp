#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <cstdio>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstring>
#include <cstdlib>
using namespace std;

bool waitForData(int sock, int timeoutMs) {
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(sock, &readSet);

    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    // Linux requires the first argument to be the highest fd + 1
    int result = select(sock + 1, &readSet, nullptr, nullptr, &timeout);
    return result > 0;
}

int main() {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    struct sockaddr_in clientAddr;

    if (bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("bind");
        close(sock);
        return 1;
    }

    int recvBufferSize = 100 * 1024 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &recvBufferSize, sizeof(recvBufferSize));

    char *buffer = (char*)malloc(40485760 * sizeof(char));
    char *flag = (char*)malloc(40485760 * sizeof(char));
    memset(flag, 0, 40485760);

    char packet[10472];
    unsigned short *seq = (unsigned short*)packet;

    socklen_t addrLen = sizeof(clientAddr);
    int c = 0, fi = 0, sequ = 20000;
    int bytesAvailable = 0;
    bool f = 1;
    int gone = 0;
    short ma = 0, m[4], u = 0;
    unsigned error;

    // Wait until data is available - Linux uses ioctl with FIONREAD
    while (bytesAvailable == 0) {
        ioctl(sock, FIONREAD, &bytesAvailable);
    }

    auto s = chrono::steady_clock::now();
    for (;;) {
        if (waitForData(sock, 100)) {
            int bytes = recvfrom(sock, packet, 1452, 0,
                                 (struct sockaddr*)&clientAddr, &addrLen);
            if (bytes < 1) continue;

            if (seq[0] > ma || (abs(seq[0] - ma) > 2000 && ma > seq[0]))
                ma = seq[0];

            memcpy(buffer + (seq[0] * 1450), packet + 2, 1450);
            flag[seq[0]] = 10;
            c++;
            int g = 0;
            
            if (ma > 0.4 * sequ) {
                f = 0;
            }

            int y = 100;
            for (int i = 10, l = 1; i < y * 8; i++) {
                if (i % y == 0) l++;
                int p = ma - i;
                if (p < 0) {
                    if (f == 0) p += sequ;
                    else break;
                }
                if (flag[p] < l) {
                    flag[p] = l;
                    memcpy(&packet[g * 2], &p, sizeof(short));
                    g++;
                }
            }

            auto e = chrono::steady_clock::now();

            if (g != 0) {
                sendto(sock, packet, g * 2, 0,
                       (struct sockaddr*)&clientAddr, sizeof(clientAddr));
                fi += g;
            }

            if (chrono::duration_cast<chrono::microseconds>(e - s).count() >= 1000000) {
                cout << "lose: " << gone << "+" << fi << " seq= " << seq[0] << "\n";
                c = 0;
                fi = 0;
                s = chrono::steady_clock::now();
                gone = 0;
            }
            u++;
            
            if (f == 0) {
                int p = ma - (sequ * 0.4);
                if (p < 0) p += sequ;
                for (int i = p, n = 1; n < sequ * 0.4; n++, i--) {
                    if (i < 0) i += sequ;
                    if (n > sequ) n %= sequ;
                    if (flag[i] != 10 && flag[i] != 0) gone++, flag[i] = 0;
                    flag[ma + n] = 0;
                }
            }
        }
    }

    close(sock);
    free(buffer);
    free(flag);
    return 0;
}
