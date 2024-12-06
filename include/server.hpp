#ifndef SERVER_HPP
#define SERVER_HPP

#include "channel.hpp"
#include "client.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

class Server {
private:
  int listener;
  int yes;
  fd_set master;
  fd_set read_fds;
  char buf[512];
  int nbytes;
  int fdmax;
  std::vector<User> users;
  std::vector<Channel> channels;
  std::string password;
  struct sockaddr_in serverAddr;
  struct sockaddr_in remoteAddr;
  int idCounter;
  std::map<int, std::string> halfMsgs;
  Server();

public:
  Server(int port, std::string password);
  void addUser(User user);
  std::vector<User> getUserList();
  void removeUser(int socket);
  User &findUserById(int id);
  User &findUserBySocket(int socket);
  User &findUserByNick(std::string nick);
  Channel &findChannelByName(std::string name);
  void start();
  void handleNewConnections();
  void handleData(int socket);
  int generateId();
  std::string totalName(User &user);
  void cmdCAP(std::istringstream &iss, User &user);
  void cmdPING(User &user);
  void cmdNICK(std::istringstream &iss, User &user);
  void cmdPASS(std::istringstream &iss, User &user);
  void cmdUSER(std::istringstream &iss, User &user);
  void cmdPRIVMSG(std::istringstream &iss, User &user);
  void PRIVMSGtoUser(std::istringstream &iss, User &user, std::string target);
  void PRIVMSGtoChan(std::istringstream &iss, User &user, std::string target);
  void sendAllChanMembers(Channel &channel, std::string msg);
  void cmdJOIN(std::istringstream &iss, User &user);
  void cmdPART(std::istringstream &iss, User &user);
  void cmdQUIT(User &user);
  void cmdKICK(std::istringstream &iss, User &user);
  void cmdTOPIC(std::istringstream &iss, User &user);
  void cmdMODE(std::istringstream &iss, User &user);
  void cmdINVITE(std::istringstream &iss, User &user);
  void cmdLIST(std::istringstream &iss, User &user);
  void cmdROLL(std::istringstream &iss, User &user);
  void sendMsg(User &user, std::string msg);
  void sendMsgToChannel(Channel &channel, std::string msg);
  ~Server();
};

#endif
