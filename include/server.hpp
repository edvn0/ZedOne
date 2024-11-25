#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <mutex>
#include <vector>

namespace ZedOne {

class Server {
public:
  explicit Server(std::int32_t port);
  ~Server();

  Server(const Server &) = delete;
  Server &operator=(const Server &) = delete;

  Server(Server &&) noexcept;
  Server &operator=(Server &&) noexcept;

  auto run() -> void;
  auto stop() -> void;

private:
  std::int32_t server_file_descriptor;
  std::int32_t port;

  std::mutex client_thread_mutex;
  std::vector<std::jthread> client_threads;
  std::atomic_bool running{false};
  std::uint32_t active_connections{0};

  auto setup_server_socket() -> void;
  auto accept_connections() -> void;
  static auto handle_client(const std::int32_t client_file_descriptor) -> void;
};

} // namespace ZedOne
