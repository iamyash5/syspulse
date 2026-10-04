// syspulsectl.cpp
// SysPulse CLI: connects to the daemon's UNIX socket and sends a command.
//
// Build: g++ -std=c++17 -o syspulsectl syspulsectl.cpp
// Usage: ./syspulsectl status
//        ./syspulsectl set-threshold 70

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

constexpr const char* SOCKET_PATH = "/tmp/syspulse.sock";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n"
                  << "  syspulsectl status\n"
                  << "  syspulsectl set-threshold <value>\n";
        return 1;
    }

    std::string command;
    std::string arg1 = argv[1];

    if (arg1 == "status") {
        command = "STATUS";
    } else if (arg1 == "set-threshold" && argc >= 3) {
        command = std::string("SET_THRESHOLD ") + argv[2];
    } else {
        std::cout << "Unknown command.\n";
        return 1;
    }

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "socket() failed\n";
        return 1;
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Could not connect to daemon. Is syspulsed running?\n";
        close(sock);
        return 1;
    }

    write(sock, command.c_str(), command.size());

    char buf[256] = {0};
    ssize_t n = read(sock, buf, sizeof(buf) - 1);
    if (n > 0) {
        std::cout << buf;
    }

    close(sock);
    return 0;
}