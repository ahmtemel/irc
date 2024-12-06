#include "../include/client.hpp"
#include "../include/server.hpp"

User::User(int socket) {
  this->socket = socket;
  this->isRegistered = false;
}

int User::getId() const { return this->Id; }

int User::getSocket() const { return this->socket; }

std::string User::getNickName() const { return this->nickName; }

std::string User::getUserName() const { return this->userName; }

std::string User::getRealName() const { return this->realName; }

std::string User::getPassword() const { return this->password; }

std::string User::getHostName() const { return this->hostName; }

bool User::getIsRegistered() { return this->isRegistered; }

void User::setId(int Id) { this->Id = Id; }

void User::setNickName(std::string nickName) { this->nickName = nickName; }

void User::setUserName(std::string userName) { this->userName = userName; }

void User::setRealName(std::string realName) { this->realName = realName; }

void User::setPassword(std::string password) { this->password = password; }

void User::setHostName(std::string hostName) { this->hostName = hostName; }

void User::setIsRegistered(bool isRegistered) {
  this->isRegistered = isRegistered;
}

User::~User() {}
