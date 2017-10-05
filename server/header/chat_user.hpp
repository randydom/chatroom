#pragma once

#include <cstddef>
#include <string>

class ChatUserProfile;

namespace protocol {
    class State;
}

using ChatUserID = std::size_t;

class ChatUser {
private:
    const ChatUserID id;
    const ChatUserProfile& profile;
    protocol::State& protocol_state;

public:
    ChatUser(const ChatUserProfile& profile, protocol::State& protocol_state, const ChatUserID id) noexcept;
    ChatUser(ChatUser const &) = delete;
    ChatUser(ChatUser&&) = default;
    ChatUser& operator=(const ChatUser&) = delete;
    ChatUser& operator=(ChatUser&&) = default;

    ChatUserID get_id() const noexcept;
    const ChatUserProfile& get_profile() const noexcept;
    void send_anonymous_message(const std::string& message);
    void send_anonymous_private_message(const std::string& message);
    void send_message(const std::string& name, const std::string& message);
    void send_private_message(const std::string& name, const std::string& message);
};

class ChatUserProfile {
private:
    const std::string name;
    const std::string password;

public:
    ChatUserProfile(const std::string name, const std::string password);

    bool compare_password(const std::string& password) const noexcept;
    std::string get_name() const noexcept;
};
