#include "../include/server.hpp"

int main(int argc, char *argv[]) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " <port> <password>" << std::endl;
    return 1;
  }
  int port = std::atoi(argv[1]);
  Server server(port, argv[2]);
  std::cout << "Server started on port " << port << std::endl;
  server.start();
  return 0;
}
