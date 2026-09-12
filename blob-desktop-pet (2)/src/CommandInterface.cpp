#include "CommandInterface.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>

std::string CommandInterface::SocketPath() {
    const char* runtimeDir = std::getenv("XDG_RUNTIME_DIR");
    std::string base = runtimeDir && *runtimeDir ? runtimeDir : "/tmp";
    return base + "/blob-pet-" + std::to_string(getuid()) + ".sock";
}

CommandInterface::CommandInterface() = default;
CommandInterface::~CommandInterface() { Shutdown(); }

bool CommandInterface::IsRunning() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::string path = SocketPath();
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    bool connected = (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0);
    close(fd);
    return connected;
}

std::optional<std::string> CommandInterface::SendCommand(const std::string& command) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return std::nullopt;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::string path = SocketPath();
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return std::nullopt;
    }

    ssize_t sent = write(fd, command.c_str(), command.size());
    (void)sent;

    char buf[256] = {0};
    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);

    if (n <= 0) return std::nullopt;
    return std::string(buf, static_cast<size_t>(n));
}

bool CommandInterface::BecomeServer() {
    // If something is already listening, refuse to start a second server.
    if (IsRunning()) return false;

    std::string path = SocketPath();
    // Remove a stale socket file left behind by a crashed instance (safe
    // now that we've confirmed via IsRunning() that nothing is listening).
    unlink(path.c_str());

    listenFd_ = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listenFd_ < 0) return false;

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(listenFd_);
        listenFd_ = -1;
        return false;
    }
    if (listen(listenFd_, 4) != 0) {
        close(listenFd_);
        listenFd_ = -1;
        unlink(path.c_str());
        return false;
    }

    // Non-blocking so PollIncoming() can be called once per frame without
    // ever stalling the render loop.
    int flags = fcntl(listenFd_, F_GETFL, 0);
    fcntl(listenFd_, F_SETFL, flags | O_NONBLOCK);

    isServer_ = true;
    return true;
}

std::string CommandInterface::PollIncoming() {
    if (listenFd_ < 0) return "";

    int clientFd = accept(listenFd_, nullptr, nullptr);
    if (clientFd < 0) return ""; // EAGAIN/EWOULDBLOCK: nothing pending

    char buf[64] = {0};
    ssize_t n = read(clientFd, buf, sizeof(buf) - 1);
    std::string cmd = (n > 0) ? std::string(buf, static_cast<size_t>(n)) : std::string();

    std::string reply = "ok";
    if (cmd == "status") reply = "running";

    ssize_t written = write(clientFd, reply.c_str(), reply.size());
    (void)written;
    close(clientFd);
    return cmd;
}

void CommandInterface::Shutdown() {
    if (listenFd_ >= 0) {
        close(listenFd_);
        listenFd_ = -1;
    }
    if (isServer_) {
        unlink(SocketPath().c_str());
        isServer_ = false;
    }
}
