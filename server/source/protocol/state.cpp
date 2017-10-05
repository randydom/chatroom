#include <algorithm>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <iostream>
#include <string>
#include <system_error>

#include <arpa/inet.h>

#include <protocol/message.hpp>
#include <protocol/state.hpp>

using namespace std;

namespace protocol {
    State::State(ChatApp& chat_app) noexcept :
        chat_app(chat_app),
        chat_user_id(0),
        read_buffer(),
        write_buffer()
    {
        reset_read_state();
    }

    State::~State() {
        if (chat_user_id != 0) {
            const auto name = chat_app.get_user_profile(chat_user_id).get_name();
            chat_app.logout(chat_user_id);
            cout << "<*EVENT*> User \"" << name << "\" has logged out" << endl;
        }
    }

    bool State::is_ready_to_write() const noexcept {
        return !write_buffer.is_empty();
    }

    bool State::read(TCPClientSocket& socket) {
        auto helper = [=](bool should_close = false) {
            while (read_buffer.is_ready()) {
                switch (read_state) {
                    case ReadState::MessageData:
                        parse_message();
                        reset_read_state();
                        break;
                    
                    case ReadState::MessageHeader: {
                        auto invalid_message_type = false;
                        client_message_type = static_cast<ClientMessageType>(read_buffer.read_u8());
                        
                        switch (client_message_type) {
                            case ClientMessageType::ListUsers:
                            case ClientMessageType::Login:
                            case ClientMessageType::Logout:
                            case ClientMessageType::Register:
                            case ClientMessageType::SendPrivateMessage:
                            case ClientMessageType::SendPublicMessage:
                                break;
                            
                            default:
                                invalid_message_type = true;
                                break;
                        }

                        if (invalid_message_type) {
                            send_header_error_response_message(HeaderErrorCode::UnknownMessageType);
                            reset_read_state();
                            break;
                        }
                        
                        const auto message_size = read_buffer.read_u16();

                        if (message_size > read_buffer_size - header_size) {
                            send_header_error_response_message(HeaderErrorCode::MaximumMessageSizeExceeded);
                            reset_read_state();
                            break;
                        }

                        read_state = ReadState::MessageData;
                        read_buffer.reset(message_size);
                    }
                }
            }
            
            return should_close;
        };

        try {
            read_buffer.read_from_socket(socket);
        } catch (const system_error& error) {
            switch (error.code().value()) {
                case EAGAIN:
#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                    return helper();

                case ECONNRESET:
                    return helper(true);
                
                default:
                    helper(true);
                    throw error;
            }
        } catch (const SocketClosedException& error) {
            return helper(true);
        }

        return helper();
    }

    bool State::write(TCPClientSocket& socket) {
        try {
            write_buffer.write_to_socket(socket);
        } catch (const system_error& error) {
            switch (error.code().value()) {
                case EAGAIN:

#if EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
                    break;

                case ECONNRESET:
                    return true;
                    break;
                
                default:
                    throw error;
            }
        }

        return false;
    }

    void State::parse_message() {
        switch (client_message_type) {
            case ClientMessageType::ListUsers:
                parse_list_users_message();
                break;

            case ClientMessageType::Login:
                parse_login_message();
                break;

            case ClientMessageType::Logout:
                parse_logout_message();
                break;

            case ClientMessageType::Register:
                parse_register_message();
                break;
            
            case ClientMessageType::SendPrivateMessage:
                parse_send_private_message_message();
                break;

            case ClientMessageType::SendPublicMessage:
                parse_send_public_message_message();
                break; 
        }
    }

    void State::parse_list_users_message() {
        if (chat_user_id == 0) {
            cout << "<*EVENT*> List users error - Unauthenticated" << endl;
            send_list_users_response_message(ListUsersResponseCode::Unauthenticated);
            return;
        }

        auto user_list = chat_app.get_online_user_list();
        sort(user_list.begin(), user_list.end());
        send_list_users_response_message(user_list);
    }

    void State::parse_login_message() {
        if (chat_user_id > 0) {
            cout << "<*EVENT*> User \"" << chat_app.get_user_profile(chat_user_id).get_name() << "\": List users error - Unauthorized" << endl;
            send_login_response_message(LoginResponseCode::Unauthorized);
            return;
        }

        // Read name length

        unsigned char name_length;

        if (!read_buffer.try_read_u8(name_length)) {
            cout << "<*EVENT*> Login error - Missing name length" << endl;
            send_login_response_message(LoginResponseCode::MissingNameLength);
            return;
        }
        
        if (name_length < 4 || name_length > 8) {
            cout << "<*EVENT*> Login error - Invalid name length" << endl;
            send_login_response_message(LoginResponseCode::InvalidNameLength);
            return;
        }

        // Read name

        string name;
        name.reserve(name_length);

        for (size_t i{0}; i < name_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                cout << "<*EVENT*> Login error - Missing name" << endl;
                send_login_response_message(LoginResponseCode::MissingName);
                return;
            }

            if (isalnum(c) == 0) {
                cout << "<*EVENT*> Login error - Invalid name" << endl;
                send_login_response_message(LoginResponseCode::InvalidName);
                return;
            }

            name += c;
        }

        // Read password length

        unsigned char password_length;
        
        if (!read_buffer.try_read_u8(password_length)) {
            cout << "<*EVENT*> Login error - Missing password length" << endl;
            send_login_response_message(LoginResponseCode::MissingPasswordLength);
            return;
        }
        
        if (password_length < 4 || password_length > 8) {
            cout << "<*EVENT*> Login error - Invalid password length" << endl;
            send_login_response_message(LoginResponseCode::InvalidPasswordLength);
            return;
        }

        // Read password

        string password;
        password.reserve(password_length);

        for (size_t i{0}; i < password_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                cout << "<*EVENT*> Login error - Missing password" << endl;
                send_login_response_message(LoginResponseCode::MissingPassword);
                return;
            }

            if (isalnum(c) == 0) {
                cout << "<*EVENT*> Login error - Invalid password" << endl;
                send_login_response_message(LoginResponseCode::InvalidPassword);
                return;
            }

            password += c;
        }

        try {
            chat_user_id = chat_app.login(*this, name, password);
        } catch (const UserDoesNotExistException&) {
            cout << "<*EVENT*> Login error - User does not exist" << endl;
            send_login_response_message(LoginResponseCode::UserDoesNotExist);
            return;
        } catch (const IncorrectPasswordException&) {
            cout << "<*EVENT*> Login error - Incorrect password" << endl;
            send_login_response_message(LoginResponseCode::IncorrectPassword);
            return;
        }

        send_login_response_message(LoginResponseCode::Success);

        // We get the name again since this is the name that will have the correct case sensitive
        // characters.
        
        name = chat_app.get_user_profile(chat_user_id).get_name();
        cout << "<*EVENT*> User \"" << name << "\" has logged in" << endl;
    }

    void State::parse_logout_message() {
        if (chat_user_id == 0) {
            cout << "<*EVENT*> Logout error - Unauthenticated" << endl;
            send_logout_response_message(LogoutResponseCode::Unauthenticated);
            return;
        }

        const auto name = chat_app.get_user_profile(chat_user_id).get_name();
        chat_app.logout(chat_user_id);
        chat_user_id = 0;
        
        send_logout_response_message(LogoutResponseCode::Success);
        cout << "<*EVENT*> User \"" << name << "\" has logged out" << endl;
    }

    void State::parse_register_message() {
        if (chat_user_id > 0) {
            cout << "<*EVENT*> User \"" << chat_app.get_user_profile(chat_user_id).get_name() << "\": Registration error - Unauthorized" << endl;
            send_register_response_message(RegisterResponseCode::Unauthorized);
            return;
        }

        // Read name length

        unsigned char name_length;

        if (!read_buffer.try_read_u8(name_length)) {
            cout << "<*EVENT*> Registration error - Missing name length" << endl;
            send_register_response_message(RegisterResponseCode::MissingNameLength);
            return;
        }
        
        if (name_length < 4 || name_length > 8) {
            cout << "<*EVENT*> Registration error - Invalid name length" << endl;
            send_register_response_message(RegisterResponseCode::InvalidNameLength);
            return;
        }

        // Read name

        string name;
        name.reserve(name_length);

        for (size_t i{0}; i < name_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                cout << "<*EVENT*> Registration error - Missing name" << endl;
                send_register_response_message(RegisterResponseCode::MissingName);
                return;
            }

            if (isalnum(c) == 0) {
                cout << "<*EVENT*> Registration error - Invalid name" << endl;
                send_register_response_message(RegisterResponseCode::InvalidName);
                return;
            }

            name += c;
        }

        // Read password length

        unsigned char password_length;
        
        if (!read_buffer.try_read_u8(password_length)) {
            cout << "<*EVENT*> Registration error - Missing password length" << endl;
            send_register_response_message(RegisterResponseCode::MissingPasswordLength);
            return;
        }
        
        if (password_length < 4 || password_length > 8) {
            cout << "<*EVENT*> Registration error - Invalid password length" << endl;
            send_register_response_message(RegisterResponseCode::InvalidPasswordLength);
            return;
        }

        // Read password

        string password;
        password.reserve(password_length);

        for (size_t i{0}; i < password_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                cout << "<*EVENT*> Registration error - Missing password" << endl;
                send_register_response_message(RegisterResponseCode::MissingPassword);
                return;
            }

            if (isalnum(c) == 0) {
                cout << "<*EVENT*> Registration error - Invalid password" << endl;
                send_register_response_message(RegisterResponseCode::InvalidPassword);
                return;
            }

            password += c;
        }

        try {
            chat_app.register_user(name, password);
        } catch (const UserAlreadyRegisteredException&) {
            cout << "<*EVENT*> Registration error - User already registered" << endl;
            send_register_response_message(RegisterResponseCode::UserAlreadyRegistered);
            return;
        }

        send_register_response_message(RegisterResponseCode::Success);
        cout << "<*EVENT*> New user \"" << name << "\" has been registered" << endl;
    }

    void State::parse_send_private_message_message() {
        if (chat_user_id == 0) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::Unauthenticated);
            return;
        }

        unsigned char options;

        if (!read_buffer.try_read_u8(options)) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::MissingOptions);
            return;
        }

        bool is_anonymous = options && 0x01;

        // Read name length

        unsigned char name_length;
        
        if (!read_buffer.try_read_u8(name_length)) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::MissingNameLength);
            return;
        }
        
        if (name_length < 4 || name_length > 8) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::InvalidNameLength);
            return;
        }

        // Read name

        string name;
        name.reserve(name_length);

        for (size_t i{0}; i < name_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                send_send_private_message_response_message(SendPrivateMessageResponseCode::MissingName);
                return;
            }

            if (isalnum(c) == 0) {
                send_send_private_message_response_message(SendPrivateMessageResponseCode::InvalidName);
                return;
            }

            name += c;
        }

        // Read message length

        unsigned short message_length;
        
        if (!read_buffer.try_read_u16(message_length)) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::MissingMessageLength);
            return;
        }

        if (message_length == 0 || message_length > 4096) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::InvalidMessageLength);
        }

        // Read message

        string message;
        message.reserve(message_length);

        for (size_t i{0}; i < message_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                send_send_private_message_response_message(SendPrivateMessageResponseCode::MissingMessage);
                return;
            }

            if (isprint(c) == 0) {
                send_send_private_message_response_message(SendPrivateMessageResponseCode::InvalidMessage);
                return;
            }

            message += c;
        }

        const auto sender_name = chat_app.get_user_profile(chat_user_id).get_name();

        if (sender_name == name) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::CannotMessageSelf);
            return;
        }

        bool online;

        if (is_anonymous) {
            online = chat_app.send_anonymous_private_message(chat_user_id, name, message);
        } else {
            online = chat_app.send_private_message(chat_user_id, name, message);
        }

        if (!online) {
            send_send_private_message_response_message(SendPrivateMessageResponseCode::UserNotOnline);
            return;
        }

        send_send_private_message_response_message(SendPrivateMessageResponseCode::Success);
        cout << "<*EVENT*> User \"" << sender_name << "\" sent private message " << "\"" << message << "\" to user \"" << name << "\"" << (is_anonymous ? " anonymously" : "") << endl;
    }

    void State::parse_send_public_message_message() {
        if (chat_user_id == 0) {
            send_send_public_message_response_message(SendPublicMessageResponseCode::Unauthenticated);
            return;
        }

        unsigned char options;

        if (!read_buffer.try_read_u8(options)) {
            send_send_public_message_response_message(SendPublicMessageResponseCode::MissingOptions);
            return;
        }

        bool is_anonymous = options && 0x01;

        // Read message length

        unsigned short message_length;
        
        if (!read_buffer.try_read_u16(message_length)) {
            send_send_public_message_response_message(SendPublicMessageResponseCode::MissingMessageLength);
            return;
        }

        if (message_length == 0 || message_length > 4096) {
            send_send_public_message_response_message(SendPublicMessageResponseCode::InvalidMessageLength);
        }

        // Read message

        string message;
        message.reserve(message_length);

        for (size_t i{0}; i < message_length; ++i) {
            unsigned char c;

            if (!read_buffer.try_read_u8(c)) {
                send_send_public_message_response_message(SendPublicMessageResponseCode::MissingMessage);
                return;
            }

            if (isprint(c) == 0) {
                send_send_public_message_response_message(SendPublicMessageResponseCode::InvalidMessage);
                return;
            }

            message += c;
        }

        const auto name = chat_app.get_user_profile(chat_user_id).get_name();

        if (is_anonymous) {
            chat_app.send_anonymous_message(chat_user_id, message);
        } else {
            chat_app.send_message(chat_user_id, message);
        }

        send_send_public_message_response_message(SendPublicMessageResponseCode::Success);
        cout << "<*EVENT*> User \"" << name << "\" sent message " << "\"" << message << "\"" << (is_anonymous ? " anonymously" : "") << endl;
    }

    void State::reset_read_state() {
        read_buffer.reset(header_size);
        read_state = ReadState::MessageHeader;
    }

    void State::send_header_error_response_message(const HeaderErrorCode error_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::HeaderErrorResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(error_code));
    }

    void State::send_list_users_response_message(const ListUsersResponseCode response_code) {
        assert(response_code != ListUsersResponseCode::Success);

        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::ListUsersResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }

    void State::send_list_users_response_message(const vector<string>& users_list) {
        size_t message_size = 2;

        for (const auto& name : users_list) {
            message_size += name.size() + 1;
        }
        
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::ListUsersResponse));
        write_buffer.write_u16(message_size);
        write_buffer.write_u8(static_cast<unsigned char>(ListUsersResponseCode::Success));
        write_buffer.write_u8(users_list.size());

        for (const auto& name : users_list) {
            write_buffer.write_u8(name.size());

            for (const auto c : name) {
                write_buffer.write_u8(c);
            }
        }
    }

    void State::send_login_response_message(const LoginResponseCode response_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::LoginResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }

    void State::send_logout_response_message(const LogoutResponseCode response_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::LogoutResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }

    void State::send_register_response_message(const RegisterResponseCode response_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::RegisterResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }

    void State::send_send_private_message_event_message(const string& message) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPrivateMessageEvent));
        write_buffer.write_u16(message.size() + 3);
        write_buffer.write_u8(static_cast<unsigned char>(true));
        
        write_buffer.write_u16(message.size());

        for (const auto c : message) {
            write_buffer.write_u8(c);
        }
    }

    void State::send_send_private_message_event_message(const string& name, const string& message) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPrivateMessageEvent));
        write_buffer.write_u16(name.size() + message.size() + 4);
        write_buffer.write_u8(static_cast<unsigned char>(false));
        write_buffer.write_u8(name.size());
        
        for (const auto c : name) {
            write_buffer.write_u8(c);
        }

        write_buffer.write_u16(message.size());

        for (const auto c : message) {
            write_buffer.write_u8(c);
        }
    }

    void State::send_send_private_message_response_message(const SendPrivateMessageResponseCode response_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPrivateMessageResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }

    void State::send_send_public_message_event_message(const string& message) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPublicMessageEvent));
        write_buffer.write_u16(message.size() + 3);
        write_buffer.write_u8(static_cast<unsigned char>(true));
        
        write_buffer.write_u16(message.size());

        for (const auto c : message) {
            write_buffer.write_u8(c);
        }
    }

    void State::send_send_public_message_event_message(const string& name, const string& message) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPublicMessageEvent));
        write_buffer.write_u16(name.size() + message.size() + 4);
        write_buffer.write_u8(static_cast<unsigned char>(false));
        write_buffer.write_u8(name.size());
        
        for (const auto c : name) {
            write_buffer.write_u8(c);
        }

        write_buffer.write_u16(message.size());

        for (const auto c : message) {
            write_buffer.write_u8(c);
        }
    }

    void State::send_send_public_message_response_message(const SendPublicMessageResponseCode response_code) {
        write_buffer.write_u8(static_cast<unsigned char>(ServerMessageType::SendPublicMessageResponse));
        write_buffer.write_u16(1);
        write_buffer.write_u8(static_cast<unsigned char>(response_code));
    }
}
