#include "../include/channel.hpp"
#include "../include/server.hpp"

Channel::Channel(std::string name) {
  this->name = name;
  this->isInviteOnly = false;
}

void Channel::addMember(User &user) { memberFd.push_back(user.getSocket()); }

void Channel::addOperator(User &user) {
  operatorFd.push_back(user.getSocket());
}

void Channel::removeMember(User &user) {
  std::vector<int>::iterator it;
  it = std::find(memberFd.begin(), memberFd.end(), user.getSocket());
  if (it != memberFd.end()) {
    memberFd.erase(it);
  }
}

void Channel::banMember(User &user) {
  removeMember(user);
  bannedFd.push_back(user.getSocket());
}

void Channel::unbanMember(int fd) {
  std::vector<int>::iterator it;
  it = std::find(bannedFd.begin(), bannedFd.end(), fd);
  if (it != bannedFd.end()) {
    bannedFd.erase(it);
  }
}

void Channel::removeOperator(User &user) {
  std::vector<int>::iterator it;
  it = std::find(operatorFd.begin(), operatorFd.end(), user.getSocket());
  if (it != operatorFd.end()) {
    operatorFd.erase(it);
  }
}

void Channel::setTopic(std::string topic) { this->topic = topic; }

std::string Channel::getTopic() const { return topic; }

std::string Channel::getName() const { return name; }

std::vector<int> Channel::getMembers() { return memberFd; }

std::vector<int> Channel::getOperators() { return operatorFd; }

std::vector<int> Channel::getBanned() { return bannedFd; }

bool Channel::isMember(int fd) const {
  std::vector<int>::const_iterator it;
  it = std::find(memberFd.begin(), memberFd.end(), fd);
  if (it != memberFd.end()) {
    return true;
  }
  return false;
}
bool Channel::isOperator(int fd) const {
  std::vector<int>::const_iterator it;
  it = std::find(operatorFd.begin(), operatorFd.end(), fd);
  if (it != operatorFd.end()) {
    return true;
  }
  return false;
}

bool Channel::isBanned(int fd) const {
  std::vector<int>::const_iterator it;
  it = std::find(bannedFd.begin(), bannedFd.end(), fd);
  if (it != bannedFd.end()) {
    return true;
  }
  return false;
}

bool Channel::isInvited(int fd) const {
  std::vector<int>::const_iterator it;
  it = std::find(inviteList.begin(), inviteList.end(), fd);
  if (it != inviteList.end()) {
    return true;
  }
  return false;
}

void Channel::removeInvite(int fd) {
  std::vector<int>::iterator it;
  it = std::find(inviteList.begin(), inviteList.end(), fd);
  if (it != inviteList.end()) {
    inviteList.erase(it);
  }
}

void Channel::setInviteOnly(bool isInviteOnly) {
  this->isInviteOnly = isInviteOnly;
}
bool Channel::getInviteOnly() const { return isInviteOnly; }

std::vector<int> Channel::getInvited() { return inviteList; }

void Channel::inviteMember(User &user) {
  inviteList.push_back(user.getSocket());
}

Channel::~Channel() {}
