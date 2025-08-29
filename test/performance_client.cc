#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <mutex>
#include <algorithm>
#include <numeric>

using namespace std;

struct Stats {
    atomic<long> total_requests{0};
    vector<long> latencies;  // 微秒
    mutex lat_mtx;
};

// 循环发送确保完整发送
ssize_t write_n(int sock, const char* buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t s = send(sock, buf + total, n - total, 0);
        if (s <= 0) return s;
        total += s;
    }
    return total;
}

// 循环接收确保完整接收
ssize_t read_n(int sock, char* buf, size_t n) {
    size_t total = 0;
    while (total < n) {
        ssize_t r = recv(sock, buf + total, n - total, 0);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

void clientWorker(const string& ip, int port, int connections, Stats& stats) {
    vector<int> socks;
    socks.reserve(connections);

    // 建立连接
    for (int i = 0; i < connections; i++) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) { perror("socket"); continue; }

        // 关闭 Nagle 算法，立即发送小包
        int flag = 1;
        setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

        sockaddr_in serv_addr{};
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);

        if (connect(sock, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("connect");
            close(sock);
            continue;
        }
        socks.push_back(sock);
    }

    const char* msg = "ping";
    char buf[4];

    vector<long> local_latencies;
    long local_count = 0;

    while (true) {
        for (size_t i = 0; i < socks.size(); i++) {
            int sock = socks[i];
            if (sock <= 0) continue;

            auto start = chrono::high_resolution_clock::now();
            if (write_n(sock, msg, 4) <= 0) {
                close(sock);
                socks[i] = -1;
                continue;
            }

            if (read_n(sock, buf, 4) <= 0) {
                close(sock);
                socks[i] = -1;
                continue;
            }

            auto end = chrono::high_resolution_clock::now();
            long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

            local_latencies.push_back(us);
            local_count++;
        }

        // 每100条消息批量合并到全局 Stats
        if (local_count >= 100) {
            lock_guard<mutex> lock(stats.lat_mtx);
            stats.total_requests += local_count;
            stats.latencies.insert(stats.latencies.end(), local_latencies.begin(), local_latencies.end());
            local_latencies.clear();
            local_count = 0;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 5) {
        cerr << "Usage: " << argv[0] << " <server_ip> <port> <threads> <connections_per_thread>\n";
        return 1;
    }

    string ip = argv[1];
    int port = stoi(argv[2]);
    int threads = stoi(argv[3]);
    int conns = stoi(argv[4]);

    Stats stats;

    vector<thread> workers;
    for (int i = 0; i < threads; i++) {
        workers.emplace_back(clientWorker, ip, port, conns, ref(stats));
    }

    // 主线程打印统计信息
    while (true) {
        this_thread::sleep_for(chrono::seconds(1));

        long qps = stats.total_requests.exchange(0);

        vector<long> snapshot;
        {
            lock_guard<mutex> lock(stats.lat_mtx);
            snapshot.swap(stats.latencies);
        }

        double avg = 0;
        long p95 = 0;
        if (!snapshot.empty()) {
            sort(snapshot.begin(), snapshot.end());
            avg = accumulate(snapshot.begin(), snapshot.end(), 0.0) / snapshot.size();
            p95 = snapshot[(int)(snapshot.size() * 0.95)];
        }

        cout << "[Stats] QPS=" << qps
             << " avgRTT=" << avg/1000 << "ms"
             << " p95RTT=" << p95/1000 << "ms"
             << " samples=" << snapshot.size()
             << endl;
    }

    for (auto& t : workers) t.join();
    return 0;
}
