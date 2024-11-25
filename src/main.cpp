#include <chrono>
#include <csignal>
#include <server.hpp>

#include <iostream>
#include <span>
#include <thread>

std::atomic_bool server_running{true};

auto signal_handler(int signum) { server_running = false; }

auto main(int argc, char **argv, char **env) -> int {
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  ZedOne::Server server{8080};

  std::jthread server_thread([&server] { server.run(); });

  while (server_running) {
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }

  server.stop();

  if (server_thread.joinable()) {
    server_thread.join();
  }

  return 0;
}
