#pragma once
#include <string>
#include <optional>

// Single-instance enforcement and IPC for the CLI (--remove / --toggle /
// --status), implemented over a Unix domain socket. This is how a fresh
// `blob --remove` invocation talks to the already-running blob process
// without needing to signal/kill it directly.
class CommandInterface {
public:
    CommandInterface();
    ~CommandInterface();

    // --- Client-side helpers (used by a fresh CLI invocation) ---

    // True if a blob server is currently listening.
    static bool IsRunning();

    // Sends a command to a running instance and returns its reply, or
    // std::nullopt if nothing is listening / the connection failed.
    static std::optional<std::string> SendCommand(const std::string& command);

    // --- Server-side (used by the running blob process) ---

    // Cleans up any stale socket file and starts listening. Returns false
    // if another instance is genuinely already running.
    bool BecomeServer();

    // Non-blocking: accepts at most one pending connection per call, reads
    // a single command, replies, and returns the command text. Returns
    // empty string if nothing was pending. Intended to be polled from the
    // main loop once per frame (cheap: a single non-blocking accept()).
    std::string PollIncoming();

    void Shutdown();

private:
    static std::string SocketPath();

    int listenFd_ = -1;
    bool isServer_ = false;
};
