#include <array>
#include <server.hpp>
#include <sstream>

namespace ZedOne {

Server::Server(std::int32_t p) : server_file_descriptor(-1), port(p) {
  setup_server_socket();
}

Server::~Server() {
  stop();
  if (server_file_descriptor != -1) {
    close(server_file_descriptor);
    server_file_descriptor = -1;
  }
}

Server::Server(Server &&other) noexcept
    : server_file_descriptor(other.server_file_descriptor), port(other.port),
      running(other.running.load()),
      client_threads(std::move(other.client_threads)) {
  other.server_file_descriptor = -1;
  other.running = false;
}

Server &Server::operator=(Server &&other) noexcept {
  if (this != &other) {
    stop();

    server_file_descriptor = other.server_file_descriptor;
    port = other.port;
    running = other.running.load();
    client_threads = std::move(other.client_threads);

    other.server_file_descriptor = -1;
    other.running = false;
  }
  return *this;
}

auto Server::run() -> void {
  running = true;
  accept_connections();
}

auto Server::stop() -> void {

  running = false;

  if (server_file_descriptor != -1) {
    close(server_file_descriptor);
    server_file_descriptor = -1;
  }

  std::lock_guard<std::mutex> lock(client_thread_mutex);
  for (auto &t : client_threads) {
    if (t.joinable()) {
      t.request_stop();
      t.join();
    }
  }
  client_threads.clear();
}

auto Server::setup_server_socket() -> void {
  server_file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (server_file_descriptor == -1) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(server_file_descriptor, SOL_SOCKET,
                 SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
    perror("setsockopt failed");
    close(server_file_descriptor);
    exit(EXIT_FAILURE);
  }

  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  if (bind(server_file_descriptor, reinterpret_cast<sockaddr *>(&address),
           sizeof(address)) < 0) {
    perror("bind failed");
    close(server_file_descriptor);
    exit(EXIT_FAILURE);
  }

  if (listen(server_file_descriptor, SOMAXCONN) == -1) {
    perror("listen failed");
    close(server_file_descriptor);
    exit(EXIT_FAILURE);
  }
}

void Server::accept_connections() {
  sockaddr_in client_address{};
  socklen_t client_address_len = sizeof(client_address);

  while (running) {
    int client_fd = accept(server_file_descriptor,
                           reinterpret_cast<sockaddr *>(&client_address),
                           &client_address_len);
    if (client_fd == -1) {
      if (errno == EINTR) {
        continue;
      }
      if (!running) {
        break;
      }
      perror("accept failed");
      continue;
    }

    std::jthread client_thread(
        [client_fd]() mutable { handle_client(client_fd); });

    // Store the client thread
    {
      std::lock_guard<std::mutex> lock(client_thread_mutex);
      client_threads.push_back(std::move(client_thread));
    }
  }
}

void Server::handle_client(int client_fd) {
  constexpr size_t BUFFER_SIZE = 4096;
  std::array<char, BUFFER_SIZE> buffer{};
  std::string request;
  ssize_t bytes_received;

  while ((bytes_received = recv(client_fd, buffer.data(), BUFFER_SIZE, 0)) >
         0) {
    request.append(buffer.data(), bytes_received);
    if (request.find("\r\n\r\n") != std::string::npos) {
      break;
    }
  }

  if (bytes_received == -1) {
    perror("recv failed");
    close(client_fd);
    return;
  }

  std::istringstream request_stream(request);
  std::string method, uri, version;
  request_stream >> method >> uri >> version;

  std::ostringstream response_stream;
  if (method == "GET") {
    std::string body = "<html><body><h1>Hello, World!</h1></body></html>";
    response_stream << "HTTP/1.1 200 OK\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << body;
  } else {
    std::string body =
        "<html><body><h1>405 Method Not Allowed</h1></body></html>";
    response_stream << "HTTP/1.1 405 Method Not Allowed\r\n"
                    << "Content-Length: " << body.size() << "\r\n"
                    << "Content-Type: text/html\r\n"
                    << "Connection: close\r\n"
                    << "\r\n"
                    << body;
  }

  std::string response = response_stream.str();
  ssize_t bytes_sent = send(client_fd, response.c_str(), response.size(), 0);
  if (bytes_sent == -1) {
    perror("send failed");
  }

  close(client_fd);
}

} // namespace ZedOne
