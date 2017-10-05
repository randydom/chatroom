#pragma once

#include <array>
#include <cstddef>
#include <exception>

#include <chat_app.hpp>
#include <chat_user.hpp>
#include <socket/tcp_client_socket.hpp>

#include <protocol/message.hpp>
#include <protocol/read_buffer.hpp>
#include <protocol/write_buffer.hpp>

namespace protocol {
    class State {
    private:
        static constexpr std::size_t read_buffer_size = 8192;
        static constexpr std::size_t write_buffer_size = 8192;

        ChatApp& chat_app;
        ChatUserID chat_user_id;
        ClientMessageType client_message_type;
        ReadBuffer<read_buffer_size> read_buffer;
        ReadState read_state;
        WriteBuffer<write_buffer_size> write_buffer;

        void reset_read_state();

        void parse_message();
        void parse_list_users_message();
        void parse_login_message();
        void parse_logout_message();
        void parse_register_message();
        void parse_send_private_message_message();
        void parse_send_public_message_message();

        void send_header_error_response_message(const HeaderErrorCode error_code);
        void send_list_users_response_message(const ListUsersResponseCode response_code);
        void send_list_users_response_message(const vector<string>& users_list);
        void send_login_response_message(const LoginResponseCode response_code);
        void send_logout_response_message(const LogoutResponseCode response_code);
        void send_register_response_message(const RegisterResponseCode response_code);
        void send_send_private_message_response_message(const SendPrivateMessageResponseCode response_code);
        void send_send_public_message_response_message(const SendPublicMessageResponseCode response_code);

    public:
        State(ChatApp& chat_app) noexcept;
        ~State();

        bool is_ready_to_write() const noexcept;
        bool read(TCPClientSocket& socket);
        bool write(TCPClientSocket& socket);

        void send_send_private_message_event_message(const std::string& message);
        void send_send_private_message_event_message(const std::string& name, const std::string& message);
        void send_send_public_message_event_message(const std::string& message);
        void send_send_public_message_event_message(const std::string& name, const std::string& message);
    };
}
