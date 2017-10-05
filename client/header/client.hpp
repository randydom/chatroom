#pragma once

#include <mutex>
#include <string>

#include <protocol/message.hpp>
#include <protocol/read_buffer.hpp>
#include <protocol/write_buffer.hpp>
#include <socket/tcp_client_socket.hpp>

class Client {
private:
    static constexpr std::size_t read_buffer_size = 8192;
    static constexpr std::size_t write_buffer_size = 8192;

    bool is_running;
    std::mutex write_buffer_mutex;
    protocol::ReadBuffer<read_buffer_size> read_buffer;
    protocol::ReadState read_state;
    protocol::ServerMessageType server_message_type;
    TCPClientSocket socket;
    protocol::WriteBuffer<write_buffer_size> write_buffer;

    bool read();
    void reset_read_state();
    void write();

    void parse_message();
    void parse_and_handle_header_error_response();
    void parse_and_handle_list_users_response_message();
    void parse_and_handle_login_response_message();
    void parse_and_handle_logout_response_message();
    void parse_and_handle_register_response_message();
    void parse_and_handle_send_private_message_event_message();
    void parse_and_handle_send_private_message_response_message();
    void parse_and_handle_send_public_message_event_message();
    void parse_and_handle_send_public_message_response_message();

    void handle_list_command();
    void handle_login_command(const std::string& name, const std::string& password);
    void handle_logout_command();
    void handle_quit_command();
    void handle_register_command(const std::string& name, const std::string& password);
    void handle_send_command(const std::string& message, const bool anonymous);
    void handle_sendpriv_command(const std::string& name, const std::string& message, const bool anonymous);

    void parse_list_command(std::string command, std::string input_line);
    void parse_login_command(std::string command, std::string input_line);
    void parse_logout_command(std::string command, std::string input_line);
    bool parse_quit_command(std::string command, std::string input_line);
    void parse_register_command(std::string command, std::string input_line);
    void parse_send_command(std::string command, std::string input_line);
    void parse_senda_command(std::string command, std::string input_line);
    void parse_sendpriv_command(std::string command, std::string input_line);
    void parse_sendpriva_command(std::string command, std::string input_line);

    void ui_handler();

public:
    Client(std::string address, std::string port);

    void run();
};
