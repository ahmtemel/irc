#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "../include/client.hpp"
#include <algorithm>
#include <vector>

class Channel {
private:
  std::string name;
  std::vector<int> memberFd;
  std::vector<int> operatorFd;
  std::vector<int> bannedFd;
  std::vector<std::string> modes;
  bool isInviteOnly;
  std::vector<int> inviteList;
  std::string topic;
  Channel();

public:
  Channel(std::string name);
  void addMember(User &user);
  void removeMember(User &user);
  void banMember(User &user);
  void unbanMember(int fd);
  void addOperator(User &user);
  void removeOperator(User &user);
  void setTopic(std::string topic);
  std::string getTopic() const;
  std::string getName() const;
  std::vector<int> getMembers();
  std::vector<int> getOperators();
  std::vector<int> getInvited();
  std::vector<int> getBanned();
  bool isMember(int fd) const;
  bool isOperator(int fd) const;
  bool isBanned(int fd) const;
  bool isInvited(int fd) const;
  void removeInvite(int fd);
  void sendMsgToMembers(std::string msg);
  void setInviteOnly(bool isInviteOnly);
  bool getInviteOnly() const;
  void inviteMember(User &user);
  ~Channel();
};

#endif
