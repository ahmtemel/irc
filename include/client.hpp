#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

class User {
private:
  int Id;
  int socket;
  std::string nickName;
  std::string userName;
  std::string realName;
  std::string password;
  std::string hostName;
  bool isRegistered;

  User();

public:
  User(int socket);
  int getId() const;
  int getSocket() const;
  std::string getNickName() const;
  std::string getUserName() const;
  std::string getRealName() const;
  std::string getPassword() const;
  std::string getHostName() const;
  bool getIsRegistered();
  void setId(int Id);
  void setNickName(std::string nickName);
  void setUserName(std::string userName);
  void setRealName(std::string realName);
  void setPassword(std::string password);
  void setHostName(std::string hostName);
  void setIsRegistered(bool isRegistered);
  ~User();
};

#endif
