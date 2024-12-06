#include "../include/server.hpp"
#include <iostream>

#define BACKLOG 10

Server::Server(int port, std::string password) {
  this->password = password;
  this->yes = 1;
  this->idCounter = 0;

  FD_ZERO(&master); // master'i ve temp'i temizle
  FD_ZERO(&read_fds);

  if ((this->listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    std::cerr << "Error creating socket" << std::endl;
    exit(1);
  }
  FD_SET(listener, &master);
  fdmax = listener;

  if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  this->serverAddr.sin_family = AF_INET; // IPv4
  this->serverAddr.sin_port = htons(port);
  this->serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(&(this->serverAddr.sin_zero), '\0', 8);

  if (bind(listener, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) ==
      -1) {
    perror("bind");
    close(listener);
    exit(1);
  }

  if (listen(listener, BACKLOG) == -1) {
    perror("listen");
    close(listener);
    exit(1);
  }
  memset(buf, 0, sizeof(buf));
}

int Server::generateId() {
  idCounter++;
  std::cout << "Id generated: " << idCounter << std::endl;
  return idCounter;
}

void Server::removeUser(int socket) {
  std::vector<User>::iterator it;
  it = users.begin();
  while (it != users.end()) {
    if (it->getSocket() == socket) {
      break;
    }
    it++;
  }
  if (it != users.end()) {
    users.erase(it);
  }

  std::vector<Channel>::iterator it2;
  it2 = channels.begin();
  while (it2 != channels.end()) {
    if (it2->isMember(socket)) {
      it2->removeMember(findUserBySocket(socket));
    }
    it2++;
  }
}

void Server::handleNewConnections() {
  socklen_t sin_size = sizeof(remoteAddr);
  int fdNewSock = accept(listener, (struct sockaddr *)&remoteAddr, &sin_size);
  if (fdNewSock == -1) {
    perror("accept");
  } else {
    FD_SET(fdNewSock, &master); // ana listeye ekle
    if (fdNewSock > fdmax) {    // azami miktarı güncelle
      fdmax = fdNewSock;
    }
  }
  User user(fdNewSock);
  user.setId(generateId());
  users.push_back(user);
  std::cout << "New user connected from " << inet_ntoa(remoteAddr.sin_addr)
            << " on " << fdNewSock << " socket" << std::endl;
}

User &Server::findUserBySocket(int socket) {
  std::vector<User>::iterator it;
  for (it = users.begin(); it != users.end(); it++) {
    if (it->getSocket() == socket) {
      return (*it);
    }
  }
  throw std::invalid_argument("User not found");
}

User &Server::findUserById(int id) {
  std::vector<User>::iterator it;
  for (it = users.begin(); it != users.end(); it++) {
    if (it->getId() == id) {
      return (*it);
    }
  }
  throw std::invalid_argument("User not found");
}

User &Server::findUserByNick(std::string nick) {
  std::vector<User>::iterator it;
  for (it = users.begin(); it != users.end(); it++) {
    if (it->getNickName() == nick) {
      return (*it);
    }
  }
  throw std::invalid_argument("User not found");
}

Channel &Server::findChannelByName(std::string name) {
  std::vector<Channel>::iterator it;
  for (it = channels.begin(); it != channels.end(); it++) {
    if (it->getName() == name) {
      return (*it);
    }
  }
  throw std::invalid_argument("Channel not found");
}

std::string Server::totalName(User &user) {
  return user.getNickName() + "!" + user.getUserName() + "@" +
         user.getHostName();
}

void Server::sendMsg(User &user, std::string msg) {
  send(user.getSocket(), msg.c_str(), msg.length(), 0);
}

void Server::sendMsgToChannel(Channel &channel, std::string msg) {
  std::vector<int> members = channel.getMembers();
  std::vector<int>::iterator it;
  for (it = members.begin(); it != members.end(); it++) {
    sendMsg(findUserBySocket(*it), msg);
  }
}

void Server::sendAllChanMembers(Channel &channel, std::string msg) {
  std::vector<int> members = channel.getMembers();
  std::vector<int>::iterator it;
  for (it = members.begin(); it != members.end(); it++) {
    sendMsg(findUserBySocket(*it), msg);
  }
}

void Server::handleData(int socket) {
  // istemciden gelen veri icin gerekeni yap
  if ((nbytes = recv(socket, buf, sizeof(buf), 0)) <= 0) {
    // bir hata var ya da istemci baglantiyi kesti
    if (nbytes == 0) {
      // baglanti kesilmis
      printf("selectserver: socket %d hung up\n", socket);
    } else {
      perror("recv");
    }
    cmdQUIT(findUserBySocket(socket));
  } else {

    if (buf[nbytes - 1] != '\n') {
      halfMsgs[socket] = halfMsgs[socket] + std::string(buf);
      return;
    } else if (buf[nbytes - 1] == '\n' && halfMsgs[socket].empty()) {
    } else {
      halfMsgs[socket] = halfMsgs[socket] + std::string(buf);
      halfMsgs[socket].erase(halfMsgs[socket].size() - 1);
      halfMsgs[socket] = halfMsgs[socket] + "\r\n";
      std::string temp = halfMsgs[socket];
      memset(buf, 0, sizeof(buf));
      strncpy(buf, temp.c_str(), sizeof(buf));
      halfMsgs[socket].clear();
    }

    std::istringstream iss(buf);
    std::string token;

    while (iss >> token) {
      if (token == "CAP") {
        cmdCAP(iss, findUserBySocket(socket));
      } else if (token == "PING") {
        cmdPING(findUserBySocket(socket));
        while (iss >> token)
          ;
      } else if (token == "NICK") {
        cmdNICK(iss, findUserBySocket(socket));
      } else if (token == "PASS") {
        cmdPASS(iss, findUserBySocket(socket));
      } else if (token == "USER") {
        cmdUSER(iss, findUserBySocket(socket));
      } else if (token == "PRIVMSG" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdPRIVMSG(iss, findUserBySocket(socket));
      } else if (token == "JOIN" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdJOIN(iss, findUserBySocket(socket));
      } else if (token == "PART" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdPART(iss, findUserBySocket(socket));
      } else if (token == "QUIT" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdQUIT(findUserBySocket(socket));
        iss.str("");
        iss.clear();
        break;

      } else if (token == "KICK" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdKICK(iss, findUserBySocket(socket));
      } else if (token == "TOPIC" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdTOPIC(iss, findUserBySocket(socket));
      } else if (token == "MODE" &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdMODE(iss, findUserBySocket(socket));
      } else if ((token == "INVITE" || token == "invite") &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdINVITE(iss, findUserBySocket(socket));
      } else if ((token == "LIST" || token == "list") &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdLIST(iss, findUserBySocket(socket));
      } else if ((token == "ROLL" || token == "roll") &&
                 findUserBySocket(socket).getIsRegistered() == true) {
        cmdROLL(iss, findUserBySocket(socket));
      }

      if (!findUserBySocket(socket).getNickName().empty() &&
          !findUserBySocket(socket).getPassword().empty() &&
          !findUserBySocket(socket).getUserName().empty() &&
          !findUserBySocket(socket).getHostName().empty() &&
          findUserBySocket(socket).getIsRegistered() == false) {
        std::string welcome = ": 001 " +
                              findUserBySocket(socket).getNickName() +
                              " :Welcome to the "
                              "Internet Relay Chat "
                              "Network " +
                              findUserBySocket(socket).getNickName() + "\r\n";
        findUserBySocket(socket).setIsRegistered(true);
        std::cout << "one time" << std::endl;
        sendMsg(findUserBySocket(socket), welcome);
      }
    }

    try {
      User u1 = findUserBySocket(socket);
      std::cout << "User " << u1.getId() << ": " << buf;
      int sendTo = std::atoi(buf);
      User u2 = findUserById(sendTo);
      std::string message = buf;
      message = message + "\r";
      message.erase(0, 2);
      sendMsg(u2, message);
    } catch (std::exception &e) {
    }

    memset(buf, 0, sizeof(buf));
  }
}

void Server::start() {
  int i;
  while (true) {
    read_fds = master;

    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(1);
    }
    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == this->listener) {
          handleNewConnections();
        } else {
          handleData(i);
        }
      }
    }
  }
}

Server::~Server() { close(listener); }
