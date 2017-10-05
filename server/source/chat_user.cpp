#include <chat_user.hpp>
#include <protocol/state.hpp>

using namespace std;

ChatUser::ChatUser(const ChatUserProfile& profile, protocol::State& protocol_state, const ChatUserID id) noexcept :
    id(id),
    profile(profile),
    protocol_state(protocol_state)
{

}

ChatUserID ChatUser::get_id() const noexcept {
    return id;
}

const ChatUserProfile& ChatUser::get_profile() const noexcept {
    return profile;
}

void ChatUser::send_anonymous_message(const string& message) {
    protocol_state.send_send_public_message_event_message(message);
}

void ChatUser::send_anonymous_private_message(const string& message) {
    protocol_state.send_send_private_message_event_message(message);
}

void ChatUser::send_message(const string& name, const string& message) {
    protocol_state.send_send_public_message_event_message(name, message);
}

void ChatUser::send_private_message(const string& name, const string& message) {
    protocol_state.send_send_private_message_event_message(name, message);
}

ChatUserProfile::ChatUserProfile(const string name, const string password) :
    name(name),
    password(password)
{

}

bool ChatUserProfile::compare_password(const string& password) const noexcept {
    return this->password == password;
}

string ChatUserProfile::get_name() const noexcept {
    return name;
}
