#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <chat_user.hpp>

namespace protocol {
    class State;
}

class IncorrectPasswordException: public std::exception {
public:
    virtual const char* what() const noexcept override;
};

class UserAlreadyRegisteredException: public std::exception {
public:
    virtual const char* what() const noexcept override;
};

class UserDoesNotExistException: public std::exception {
public:
    virtual const char* what() const noexcept override;
};

class ChatApp {
private:
    ChatUserID user_sequence_number;
    std::unordered_map<std::string, ChatUserProfile> user_profiles;
    std::unordered_map<ChatUserID, ChatUser> users_online;

public:
    ChatApp() = default;
    ChatApp(ChatApp const &) = delete;
    ChatApp(ChatApp&&) = default;
    ChatApp& operator=(const ChatApp&) = delete;
    ChatApp& operator=(ChatApp&&) = default;

    std::vector<std::string> get_online_user_list() const;
    const ChatUserProfile& get_user_profile(const ChatUserID user_id) const;
    ChatUserProfile& get_user_profile(std::string name);
    ChatUserID login(protocol::State& protocol_state, const std::string& name, const std::string& password);
    void logout(const ChatUserID user_id);
    void register_user(std::string name, std::string password);
    void send_anonymous_message(const ChatUserID user_id, const std::string& message);
    bool send_anonymous_private_message(const ChatUserID user_id, const std::string& name, const std::string& message);
    void send_message(const ChatUserID user_id, const std::string& message);
    bool send_private_message(const ChatUserID user_id, const std::string& name, const std::string& message);
};
