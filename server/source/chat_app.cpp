#include <algorithm>
#include <cctype>
#include <unordered_set>

#include <chat_app.hpp>
#include <protocol/state.hpp>

using namespace std;

const char* IncorrectPasswordException::what() const noexcept {
    return "Incorrect password";
}

const char* UserAlreadyRegisteredException::what() const noexcept {
    return "User already registered";
}

const char* UserDoesNotExistException::what() const noexcept {
    return "User does not exist";
}

vector<string> ChatApp::get_online_user_list() const {
    unordered_set<string> online_users_set;
    vector<string> online_users_list;

    for (const auto& iterator : users_online) {
        const auto name = iterator.second.get_profile().get_name();

        if (online_users_set.find(name) == online_users_set.cend()) {
            online_users_set.emplace(name);
            online_users_list.emplace_back(name);
        }
    }

    return online_users_list;
}

const ChatUserProfile& ChatApp::get_user_profile(const ChatUserID user_id) const {
    auto iterator = users_online.find(user_id);
    
    if (iterator == users_online.end()) {
        throw UserDoesNotExistException();
    }
    
    return iterator->second.get_profile();
}

ChatUserProfile& ChatApp::get_user_profile(std::string name) {
    transform(name.begin(), name.end(), name.begin(), ::tolower);
    auto iterator = user_profiles.find(name);
    
    if (iterator == user_profiles.end()) {
        throw UserDoesNotExistException();
    }
    
    return iterator->second;
}

ChatUserID ChatApp::login(protocol::State& protocol_state, const string& name, const string& password) {
    const auto& user_profile = get_user_profile(name);
    
    if (!user_profile.compare_password(password)) {
        throw IncorrectPasswordException();
    }

    const auto user_id = ++user_sequence_number;
    users_online.emplace(user_id, ChatUser(user_profile, protocol_state, user_id));
    return user_id;
}

void ChatApp::logout(const ChatUserID user_id) {
    users_online.erase(user_id);
}

void ChatApp::register_user(string name, string password) {
    string name_lowercase = name;
    transform(name_lowercase.begin(), name_lowercase.end(), name_lowercase.begin(), ::tolower);

    if (user_profiles.find(name_lowercase) != user_profiles.cend()) {
        throw UserAlreadyRegisteredException();
    }

    user_profiles.emplace(name_lowercase, ChatUserProfile(name, password));
}

void ChatApp::send_anonymous_message(const ChatUserID user_id, const string& message) {
    for (auto& iterator : users_online) {
        if (iterator.first != user_id) {
            iterator.second.send_anonymous_message(message);
        }
    }
}

bool ChatApp::send_anonymous_private_message(const ChatUserID user_id, const string& name, const string& message) {
    auto sent = false;

    for (auto& iterator : users_online) {
        if (iterator.first != user_id && iterator.second.get_profile().get_name() == name) {
            iterator.second.send_anonymous_private_message(message);
            sent = true;
        }
    }

    return sent;
}

void ChatApp::send_message(const ChatUserID user_id, const string& message) {
    const auto& user_profile = get_user_profile(user_id);

    for (auto& iterator : users_online) {
        if (iterator.first != user_id) {
            iterator.second.send_message(user_profile.get_name(), message);
        }
    }
}

bool ChatApp::send_private_message(const ChatUserID user_id, const string& name, const string& message) {
    auto sent = false;
    const auto& user_profile = get_user_profile(user_id);

    for (auto& iterator : users_online) {
        if (iterator.first != user_id && iterator.second.get_profile().get_name() == name) {
            iterator.second.send_private_message(user_profile.get_name(), message);
            sent = true;
        }
    }

    return sent;
}
