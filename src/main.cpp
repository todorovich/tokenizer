#include <sys/epoll.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <simdjson.h>

constexpr int PORT = 8080;
constexpr int MAX_EVENTS = 64;

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void handle_request(int client_fd) {
    char buf[1024];
    ssize_t len = recv(client_fd, buf, sizeof(buf), 0);
    if (len <= 0) return;

    char* body = strstr(buf, "\r\n\r\n");
    if (!body) return;
    body += 4;

    simdjson::ondemand::parser parser;
    simdjson::padded_string json(body);
    auto doc = parser.iterate(json);
    int64_t a = doc["a"].get_int64().value_unsafe();
    int64_t b = doc["b"].get_int64().value_unsafe();
    int64_t result = a + b;

    char json_out[64];
    int json_len = snprintf(json_out, sizeof(json_out), "{\"result\":%ld}", result);

    char response[256];
    int resp_len = snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s",
        json_len, json_out);

    send(client_fd, response, resp_len, 0);
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);
    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, SOMAXCONN);

    int epoll_fd = epoll_create1(0);
    epoll_event ev = { EPOLLIN, { .fd = server_fd } };
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev);

    epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == server_fd) {
                int client_fd = accept4(server_fd, nullptr, nullptr, SOCK_NONBLOCK);
                epoll_event client_ev = { EPOLLIN | EPOLLET, { .fd = client_fd } };
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);
            } else {
                handle_request(fd);
                close(fd);
            }
        }
    }
}
