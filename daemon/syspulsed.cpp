// syspulsed.cpp
// SysPulse daemon: reads CPU and memory usage from procfs,
// applies a simple threshold policy, and serves status/control
// commands to CLI clients over a UNIX domain socket.
//
// Build: g++ -std=c++17 -o syspulsed syspulsed.cpp -lpthread
// Run:   ./syspulsed

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

constexpr const char* SOCKET_PATH = "/tmp/syspulse.sock";

std::atomic<bool> running{true};
void signalHandler(int) { running = false; }

// ---------- procfs readers ----------

struct CpuTimes {
    long long user, nice, system, idle, iowait, irq, softirq, steal;
    long long total() const {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
    long long active() const { return total() - idle - iowait; }
};

bool readCpuTimes(CpuTimes &t) {
    std::ifstream f("/proc/stat");
    if (!f.is_open()) return false;
    std::string line;
    std::getline(f, line); // first line: "cpu  user nice system idle iowait irq softirq steal"
    std::istringstream iss(line);
    std::string label;
    iss >> label >> t.user >> t.nice >> t.system >> t.idle
        >> t.iowait >> t.irq >> t.softirq >> t.steal;
    return true;
}

double computeCpuPercent() {
    static CpuTimes prev{};
    static bool first = true;

    CpuTimes curr{};
    if (!readCpuTimes(curr)) return -1.0;

    if (first) {
        prev = curr;
        first = false;
        return 0.0;
    }

    long long totalDiff  = curr.total()  - prev.total();
    long long activeDiff = curr.active() - prev.active();
    prev = curr;

    if (totalDiff <= 0) return 0.0;
    return 100.0 * static_cast<double>(activeDiff) / static_cast<double>(totalDiff);
}

struct MemInfo {
    long long totalKb = 0;
    long long availableKb = 0;
};

bool readMemInfo(MemInfo &m) {
    std::ifstream f("/proc/meminfo");
    if (!f.is_open()) return false;
    std::string key;
    long long value;
    std::string unit;
    while (f >> key >> value >> unit) {
        if (key == "MemTotal:") m.totalKb = value;
        else if (key == "MemAvailable:") m.availableKb = value;
    }
    return true;
}

double computeMemPercent() {
    MemInfo m{};
    if (!readMemInfo(m) || m.totalKb == 0) return -1.0;
    double usedKb = static_cast<double>(m.totalKb - m.availableKb);
    return 100.0 * usedKb / static_cast<double>(m.totalKb);
}

// ---------- shared state ----------

std::mutex stateMutex;
double g_cpuPercent = 0.0;
double g_memPercent = 0.0;
double g_cpuThreshold = 80.0;
bool   g_alertActive  = false;

// ---------- monitor thread ----------

void monitorLoop() {
    while (running) {
        double cpu = computeCpuPercent();
        double mem = computeMemPercent();

        {
            std::lock_guard<std::mutex> lock(stateMutex);
            g_cpuPercent = cpu;
            g_memPercent = mem;
            bool shouldAlert = cpu > g_cpuThreshold;
            if (shouldAlert != g_alertActive) {
                g_alertActive = shouldAlert;
                std::cout << "[syspulsed] ALERT state changed: "
                          << (g_alertActive ? "ACTIVE (CPU high)" : "cleared")
                          << " | cpu=" << cpu << "%\n";
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// ---------- CLI command handling ----------

std::string handleCommand(const std::string &cmd) {
    std::istringstream iss(cmd);
    std::string op;
    iss >> op;

    std::lock_guard<std::mutex> lock(stateMutex);

    if (op == "STATUS") {
        std::ostringstream oss;
        oss << "cpu=" << g_cpuPercent
            << " mem=" << g_memPercent
            << " threshold=" << g_cpuThreshold
            << " alert=" << (g_alertActive ? "1" : "0");
        return oss.str();
    } else if (op == "SET_THRESHOLD") {
        double val;
        if (iss >> val) {
            g_cpuThreshold = val;
            return "OK threshold set to " + std::to_string(val);
        }
        return "ERR missing value";
    } else {
        return "ERR unknown command";
    }
}

// ---------- UNIX socket server ----------

void serverLoop() {
    int serverFd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "socket() failed\n";
        return;
    }

    unlink(SOCKET_PATH);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed\n";
        close(serverFd);
        return;
    }

    if (listen(serverFd, 5) < 0) {
        std::cerr << "listen() failed\n";
        close(serverFd);
        return;
    }

    std::cout << "[syspulsed] listening on " << SOCKET_PATH << "\n";

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(serverFd, &readfds);
        timeval tv{1, 0}; // 1 second timeout so we can check `running`

        int sel = select(serverFd + 1, &readfds, nullptr, nullptr, &tv);
        if (sel <= 0) continue;

        int clientFd = accept(serverFd, nullptr, nullptr);
        if (clientFd < 0) continue;

        char buf[256] = {0};
        ssize_t n = read(clientFd, buf, sizeof(buf) - 1);
        if (n > 0) {
            std::string cmd(buf, n);
            // trim trailing newline
            while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r')) cmd.pop_back();
            std::string response = handleCommand(cmd) + "\n";
            write(clientFd, response.c_str(), response.size());
        }
        close(clientFd);
    }

    close(serverFd);
    unlink(SOCKET_PATH);
}

// ---------- main ----------

int main() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::cout << "SysPulse daemon starting...\n";

    std::thread monitorThread(monitorLoop);
    std::thread serverThread(serverLoop);

    monitorThread.join();
    serverThread.join();

    std::cout << "SysPulse daemon stopped.\n";
    return 0;
}