#include "../include/channel.hpp"
#include "../include/client.hpp"
#include "../include/server.hpp"

void Server::cmdCAP(std::istringstream &iss, User &user) {
  std::string token;
  iss >> token;
  if (token == "LS") {
    sendMsg(user, "CAP * LS :multi-prefix\r\n");
  } else if (token == "REQ") {
    std::string cap;
    iss >> cap;
    sendMsg(user, "CAP * ACK " + cap + "\r\n");
  }
}

void Server::cmdPING(User &user) { sendMsg(user, "PONG\r\n"); }

void Server::cmdNICK(std::istringstream &iss, User &user) {
  int isRename = false;
  std::string oldNick;
  std::string nick;
  iss >> nick;
  std::vector<User>::iterator it;

  if (!user.getNickName().empty()) {
    isRename = true;
    oldNick = user.getNickName();
  }
  it = users.begin();
  for (it = users.begin(); it != users.end(); it++) {
    if (it->getNickName() == nick) {
      sendMsg(user, ": 433 * " + nick + " :Nickname is already in use\r\n");
      return;
    }
  }
  if (isRename) {
    std::vector<User>::iterator it;
    for (it = users.begin(); it != users.end(); it++) {
      sendMsg(*it, ":" + totalName(user) + " NICK " + nick + "\r\n");
    }
  }
  user.setNickName(nick);
  std::cout << "---------------------------------------------------------\n";
  it = users.begin();
  for (it = users.begin(); it != users.end(); it++) {
    std::cout << "User: " << it->getId() << " Nick: " << it->getNickName()
              << " Socket " << it->getSocket() << std::endl;
  }
  std::cout << "---------------------------------------------------------\n";
}

void Server::cmdPASS(std::istringstream &iss, User &user) {
  std::string pass;
  iss >> pass;
  if (pass.at(0) == ':') {
    pass.erase(0, 1);
  }
  if (pass == password) {
    std::cout << "Password yes" << std::endl;
    user.setPassword(pass);
  } else {
    std::cout << "Password no" << std::endl;
    sendMsg(user, ": 464 * :Password incorrect\r\n");
  }
}

void Server::cmdUSER(std::istringstream &iss, User &user) {
  std::string username;
  std::string hostname;
  std::string servername;
  std::string realname;

  iss >> username;
  iss >> hostname;
  iss >> servername;
  iss >> realname;

  if (realname.length() > 0 && realname.at(0) == ':') {
    realname.erase(0, 1);
  }

  if (realname.length() > 4 && realname.at(0) == '1' && realname.at(1) == ',' &&
      realname.at(2) == '8') {
    realname.erase(0, 3);
  }

  user.setUserName(username);
  user.setHostName(servername);
  user.setRealName(realname);
}

void Server::PRIVMSGtoUser(std::istringstream &iss, User &user,
                           std::string target) {
  try {
    User targetUser = findUserByNick(target);
    std::string message;
    std::getline(iss, message);
    if (message.at(0) == ' ' && message.at(1) == ':') {
      message.erase(0, 2);
    }

    std::cout << "From: " << user.getNickName() << " To: " << target
              << std::endl;
    std::cout << "Message: " << message << std::endl;
    sendMsg(targetUser, ":" + totalName(user) + " PRIVMSG " + target + " :" +
                            message + "\r\n");
  } catch (std::exception &e) {
    sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
    return;
  }
}

void Server::PRIVMSGtoChan(std::istringstream &iss, User &user,
                           std::string target) {
  try {
    Channel &channel = findChannelByName(target);
    if (!channel.isMember(user.getSocket())) {
      sendMsg(user, ": 404 * " + target + " :You're not on that channel\r\n");
      return;
    }
    std::string message;
    std::getline(iss, message);
    if (message.at(0) == ' ' && message.at(1) == ':') {
      message.erase(0, 2);
    }
    std::vector<int> members = channel.getMembers();
    std::vector<int>::iterator it;
    for (it = members.begin(); it != members.end(); it++) {
      if (*it != user.getSocket()) {
        sendMsg(findUserBySocket(*it), ":" + totalName(user) + " PRIVMSG " +
                                           target + " :" + message + "\r\n");
      }
    }
    std::cout << "mem size: " << channel.getMembers().size() << std::endl;
    std::cout << "op socket: " << channel.getOperators().front() << std::endl;
  } catch (std::exception &e) {
    sendMsg(user, ": 403 * " + target + " :No such channel\r\n");
    return;
  }
}

void Server::cmdPRIVMSG(std::istringstream &iss, User &user) {
  std::string targetName;
  iss >> targetName;
  if (targetName.at(0) == '#') {
    PRIVMSGtoChan(iss, user, targetName);
  } else {
    PRIVMSGtoUser(iss, user, targetName);
  }
}

void Server::cmdJOIN(std::istringstream &iss, User &user) {
  std::string channelName;
  iss >> channelName;
  try {
    Channel &channel = findChannelByName(channelName);
    if (channel.isMember(user.getSocket())) {
      sendMsg(user, ":42IRC 405 * " + channelName +
                        " :You're already on that "
                        "channel\r\n");
      return;
    }
    if (channel.isBanned(user.getSocket())) {
      sendMsg(user, ":42IRC 474 * " + channelName +
                        " :You're banned from that "
                        "channel\r\n");
      return;
    }

    if (channel.getInviteOnly()) {
      if (!channel.isInvited(user.getSocket())) {
        sendMsg(user, ":42IRC 473 * " + channelName +
                          " :You're not invited to that "
                          "channel\r\n");
        return;
      } else {
        channel.removeInvite(user.getSocket());
      }
    }

    channel.addMember(user);
    sendAllChanMembers(channel,
                       ":" + totalName(user) + " JOIN " + channelName + "\r\n");

    std::vector<int> members = channel.getMembers();
    std::string namesList;
    for (std::vector<int>::iterator it = members.begin(); it != members.end();
         it++) {
      User &member = findUserBySocket(*it);
      if (channel.isOperator(member.getSocket()))
        namesList += "@" + member.getNickName() + " ";
      else
        namesList += member.getNickName() + " ";
    }
    std::string namesReply = ":server 353 " + user.getNickName() + " = " +
                             channelName + " :" + namesList + "\r\n";
    std::string endOfNamesReply = ":server 366 " + user.getNickName() + " " +
                                  channelName + " :End of /NAMES list.\r\n";

    sendMsg(user, namesReply);
    sendMsg(user, endOfNamesReply);

    if (channel.getTopic().empty()) {
      sendMsg(user, ": 331 * " + channelName + " :No topic is set\r\n");
    } else {
      sendMsg(user,
              ": 332 * " + channelName + " :" + channel.getTopic() + "\r\n");
    }
  } catch (std::exception &e) {
    if (channelName.at(0) != '#') {
      sendMsg(user, ": 403 * " + channelName + " :Invalid channel name\r\n");
      return;
    }

    Channel channel(channelName);
    channel.addMember(user);
    channel.addOperator(user);
    channels.push_back(channel);
    sendMsg(user, ":" + totalName(user) + " JOIN " + channelName + "\r\n");

    sendMsg(user, ":server 353 " + user.getNickName() + " = " + channelName +
                      " :@" + user.getNickName() + "\r\n");
    sendMsg(user, ": 331 * " + channelName + " :No topic is set\r\n");
  }
}

void Server::cmdPART(std::istringstream &iss, User &user) {
  std::string channelName;
  iss >> channelName;
  try {
    Channel &chan = findChannelByName(channelName);
    sendAllChanMembers(chan, ":" + totalName(user) + " PART " + channelName +
                                 " :Leaving\r\n");
    chan.removeMember(user);
    if (chan.isOperator(user.getSocket())) {
      chan.removeOperator(user);
      if (chan.getOperators().empty() && !chan.getMembers().empty()) {
        User &newOp = findUserBySocket(chan.getMembers().front());
        chan.addOperator(newOp);
        sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                     channelName + " +o " +
                                     newOp.getNickName() + "\r\n");
      }
    }
    if (chan.getMembers().empty()) {
      std::vector<Channel>::iterator it;
      it = channels.begin();
      while (it != channels.end()) {
        if (it->getName() == chan.getName()) {
          break;
        }
        it++;
      }
      channels.erase(it);
    }

  } catch (std::exception &e) {
    sendMsg(user, ": 403 * " + channelName + " :No such channel\r\n");
  }
}

void Server::cmdQUIT(User &user) {
  std::cout << "User: " << user.getNickName() << " quit" << std::endl;
  std::vector<Channel>::iterator it;

  for (it = channels.begin(); it != channels.end(); it++) {
    if (it->isMember(user.getSocket())) {
      sendAllChanMembers(*it, ":" + totalName(user) + " QUIT :Leaving\r\n");
      it->removeMember(user);
      if (it->isOperator(user.getSocket())) {
        it->removeOperator(user);
        if (it->getOperators().empty() && !it->getMembers().empty()) {
          User &newOp = findUserBySocket(it->getMembers().front());
          it->addOperator(newOp);
          sendAllChanMembers(*it, ":" + totalName(user) + " MODE " +
                                      it->getName() + " +o " +
                                      newOp.getNickName() + "\r\n");
        }
      }
    }
  }

  for (it = channels.begin(); it != channels.end(); it++) {
    it->unbanMember(user.getSocket());
  }
  int socket = user.getSocket();
  close(user.getSocket());
  halfMsgs[socket].clear();
  close(socket);
  FD_CLR(socket, &master);

  removeUser(socket);
}

void Server::cmdKICK(std::istringstream &iss, User &user) {
  std::string channelName;
  std::string target;
  iss >> channelName;
  iss >> target;
  try {
    Channel &chan = findChannelByName(channelName);
    try {
      User &targetUser = findUserByNick(target);
      if (chan.isOperator(user.getSocket())) {
        if (!chan.isMember(targetUser.getSocket())) {
          sendMsg(user,
                  ": 441 * " + target + " :They aren't on that channel\r\n");
          return;
        }
        if (chan.isOperator(targetUser.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You can't kick an operator\r\n");
          return;
        }
        sendAllChanMembers(chan, ":" + totalName(user) + " KICK " +
                                     channelName + " " + target +
                                     " :Kicked\r\n");
        chan.removeMember(targetUser);
        chan.removeOperator(targetUser);
      } else {
        sendMsg(user,
                ": 482 * " + channelName + " :You're not channel operator\r\n");
      }
    } catch (std::exception &e) {
      sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
    }
  } catch (std::exception &e) {
    sendMsg(user, ": 403 * " + channelName + " :No such channel\r\n");
  }
}

void Server::cmdTOPIC(std::istringstream &iss, User &user) {
  std::string channelName;
  std::string topic;
  iss >> channelName;
  iss >> topic;
  if (topic.length() > 0 && topic.at(0) == ':') {
    topic.erase(0, 1);
  }
  try {
    Channel &chan = findChannelByName(channelName);
    if (chan.isMember(user.getSocket())) {
      if (!chan.isOperator(user.getSocket())) {
        sendMsg(user,
                ": 482 * " + channelName + " :You're not channel operator\r\n");
        return;
      }
      chan.setTopic(topic);
      std::vector<int> members = chan.getMembers();
      std::vector<int>::iterator it;
      for (it = members.begin(); it != members.end(); it++) {
        sendMsg(findUserBySocket(*it), ":" + totalName(user) + " TOPIC " +
                                           channelName + " :" + topic + "\r\n");
      }

    } else {
      sendMsg(user,
              ": 442 * " + channelName + " :You're not on that channel\r\n");
    }
  } catch (std::exception &e) {
    sendMsg(user, ": 403 * " + channelName + " :No such channel\r\n");
  }
}

void Server::cmdMODE(std::istringstream &iss, User &user) {
  std::string channelName;
  std::string mode;
  iss >> channelName;
  iss >> mode;
  try {
    Channel &chan = findChannelByName(channelName);
    if (chan.isMember(user.getSocket())) {
      if (mode == "+o") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        std::string target;
        iss >> target;
        try {
          User &targetUser = findUserByNick(target);
          if (chan.isMember(targetUser.getSocket())) {
            chan.addOperator(targetUser);
            sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                         channelName + " +o " + target +
                                         "\r\n");

          } else {
            sendMsg(user, ": 441 * " + target +
                              " :They aren't on that "
                              "channel\r\n");
          }
        } catch (std::exception &e) {
          sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
        }
      } else if (mode == "-o") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        std::string target;
        iss >> target;
        try {
          User &targetUser = findUserByNick(target);
          if (chan.isMember(targetUser.getSocket())) {
            if (!chan.isOperator(targetUser.getSocket())) {
              sendMsg(user, ": 482 * " + channelName +
                                " :You can't remove operator from a non "
                                "operator\r\n");
              return;
            }
            if (user.getSocket() == targetUser.getSocket()) {
              sendMsg(user,
                      ": 482 * " + channelName +
                          " :You can't remove operator from yourself\r\n");
              return;
            }
            chan.removeOperator(targetUser);
            sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                         channelName + " -o " + target +
                                         "\r\n");
          } else {
            sendMsg(user, ": 441 * " + target +
                              " :They aren't on that "
                              "channel\r\n");
          }
        } catch (std::exception &e) {
          sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
        }
      } else if (mode == "+b") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        std::string target;
        iss >> target;
        try {
          User &targetUser = findUserByNick(target);
          if (chan.isMember(targetUser.getSocket())) {
            if (chan.isOperator(targetUser.getSocket())) {
              sendMsg(user, ": 482 * " + channelName +
                                " :You can't ban an operator\r\n");
              return;
            }
            sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                         channelName + " +b " + target +
                                         "\r\n");
            sendAllChanMembers(chan, ":" + totalName(user) + " KICK " +
                                         channelName + " " + target +
                                         " :Banned\r\n");

            chan.banMember(targetUser);
          } else {
            sendMsg(user, ": 441 * " + target +
                              " :They aren't on that "
                              "channel\r\n");
          }
        } catch (std::exception &e) {
          sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
        }
      } else if (mode == "-b") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        std::string target;
        iss >> target;
        try {
          User &targetUser = findUserByNick(target);
          chan.unbanMember(targetUser.getSocket());
          sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                       channelName + " -b " + target + "\r\n");
        } catch (std::exception &e) {
          sendMsg(user, ": 401 * " + target + " :No such nick\r\n");
        }
      } else if (mode == "b") {
        // return banned list to user
        std::vector<int> banned = chan.getBanned();
        std::vector<int>::iterator it;
        for (it = banned.begin(); it != banned.end(); it++) {
          sendMsg(user, ": 367 * " + channelName + " " +
                            findUserBySocket(*it).getNickName() + "\r\n");
        }
        sendMsg(user, ": 368 * " + channelName + " :End of /BAN list\r\n");
      } else if (mode == "+i") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        if (chan.getInviteOnly()) {
          sendMsg(user, ": 467 * " + channelName +
                            " :Channel is already invite only\r\n");
          return;
        }
        chan.setInviteOnly(true);
        sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                     channelName + " +i\r\n");
      } else if (mode == "-i") {
        if (!chan.isOperator(user.getSocket())) {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
          return;
        }
        if (!chan.getInviteOnly()) {
          sendMsg(user, ": 467 * " + channelName +
                            " :Channel is already not invite only\r\n");
          return;
        }
        chan.setInviteOnly(false);
        sendAllChanMembers(chan, ":" + totalName(user) + " MODE " +
                                     channelName + " -i\r\n");
      } else if (mode == "") {

        if (chan.getInviteOnly()) {
          sendMsg(user, ": 324 * " + channelName + " +i\r\n");
        } else {
          sendMsg(user, ": 324 * " + channelName + " -i\r\n");
        }
      } else {
        sendMsg(user, ": 472 * " + mode + " :Unknown MODE flag\r\n");
      }
    } else {
      sendMsg(user,
              ": 442 * " + channelName + " :You're not on that channel\r\n");
    }
  } catch (std::exception &e) {
    sendMsg(user, ": 403 * " + channelName + " :No such channel\r\n");
  }
}

void Server::cmdINVITE(std::istringstream &iss, User &user) {
  std::string target;
  std::string channelName;
  iss >> target;
  iss >> channelName;

  try {
    User &targetUser = findUserByNick(target);
    Channel &chan = findChannelByName(channelName);
    if (chan.isMember(user.getSocket())) {
      if (chan.getInviteOnly()) {
        if (chan.isOperator(user.getSocket())) {
          chan.inviteMember(targetUser);
          sendMsg(targetUser, ":" + totalName(user) + " INVITE " + target +
                                  " " + channelName + "\r\n");
        } else {
          sendMsg(user, ": 482 * " + channelName +
                            " :You're not channel operator\r\n");
        }
      } else {
        sendMsg(targetUser, ":" + totalName(user) + " INVITE " + target + " " +
                                channelName + "\r\n");
      }

    } else {
      sendMsg(user,
              ": 442 * " + channelName + " :You're not on that channel\r\n");
    }
  } catch (std::exception &e) {
    sendMsg(user, ": 401 * " + target + " :No such nick or channel\r\n");
  }
}

void Server::cmdLIST(std::istringstream &iss, User &user) {
  std::string channelName;
  iss >> channelName;

  std::stringstream ss;

  sendMsg(user, "321 " + user.getNickName() + " Channel :Users Name\r\n");
  if (channelName.empty()) {
    // If no parameter is given, list all channels
    std::vector<Channel>::iterator it;
    for (it = channels.begin(); it != channels.end(); it++) {
      ss.str("");
      ss << it->getMembers().size();
      sendMsg(user, "322 " + user.getNickName() + " " + it->getName() + " " +
                        ss.str() + " :" + it->getTopic() + "\r\n");
      std::cout << "Topic: " << it->getTopic() << std::endl;
    }
  } else {
    // If a parameter is given, list that specific channel
    try {
      Channel &chan = findChannelByName(channelName);
      ss.str("");
      ss << chan.getMembers().size();
      sendMsg(user, "322 " + user.getNickName() + " " + chan.getName() + " " +
                        ss.str() + " :" + chan.getTopic() + "\r\n");
      std::cout << "Topic: " << chan.getTopic() << std::endl;
    } catch (std::exception &e) {
      sendMsg(user, "403 " + user.getNickName() + " " + channelName +
                        " :No such channel\r\n");
    }
  }

  sendMsg(user, "323 " + user.getNickName() + " :End of LIST\r\n");
}

void Server::cmdROLL(std::istringstream &iss, User &user) {
  std::string dice;
  std::stringstream ss;
  iss >> dice;
  if (dice.empty()) {
    sendMsg(user, ":server 42IRC 42IRC :You must specify a dice type\r\n");
    return;
  }
  for (long unsigned int i = 0; i < dice.length(); i++) {
    if (!isdigit(dice.at(i))) {
      sendMsg(user, ":server 42IRC 42IRC :Invalid dice type\r\n");
      return;
    }
  }
  int d = std::atoi(dice.c_str());
  if (d < 1) {
    sendMsg(user, ":server 42IRC 42IRC :Invalid dice type\r\n");
    return;
  }
  int roll = rand() % d + 1;
  ss.str("");
  ss << roll;

  sendMsg(user, ":server 42IRC 42IRC :You rolled " + ss.str() + "\r\n");
}
