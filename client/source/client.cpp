#include <algorithm>
#include <cctype>
#include <chrono>
#include <exception>
#include <iostream>
#include <system_error>
#include <thread>
#include <vector>

#include <client.hpp>
#include <util.hpp>

using namespace protocol;
using namespace std;


Client::Client(string address, string port) :
    is_running(true),
    write_buffer_mutex(),
    read_buffer(),
    socket(address, port),
    write_buffer()
{
    socket.set_non_blocking(true);
    reset_read_state();
}

void Client::run() {
    thread ui_thread(&Client::ui_handler, this);
    
    while (is_running) {
        try {
            if (read()) {
                break;
            }

            write();
            this_thread::sleep_for(chrono::milliseconds{200});
        } catch (const exception& error) {
             cerr << "Closing due to error: " << error.what() << endl;
             break;
        }
    }

    ui_thread.join();
}

bool Client::read() {
    auto helper = [=](bool should_close = false) {
        if (read_buffer.is_ready()) {
            switch (read_state) {
                case ReadState::MessageData:
                    parse_message();
                    reset_read_state();
                    break;
                
                case ReadState::MessageHeader: {
                    auto invalid_message_type = false;
                    server_message_type = static_cast<ServerMessageType>(read_buffer.read_u8());
                    
                    switch (server_message_type) {
                        case ServerMessageType::HeaderErrorResponse:
                        case ServerMessageType::ListUsersResponse:
                        case ServerMessageType::LoginResponse:
                        case ServerMessageType::LogoutResponse:
                        case ServerMessageType::RegisterResponse:
                        case ServerMessageType::SendPrivateMessageEvent:
                        case ServerMessageType::SendPrivateMessageResponse:
                        case ServerMessageType::SendPublicMessageEvent:
                        case ServerMessageType::SendPublicMessageResponse:
                            break;
                        
                        default:
                            invalid_message_type = true;
                            break;
                    }

                    if (invalid_message_type) {
                        cout << "<*CLIENT*>: Received an unknown message type from server (this is a bug)" << endl;
                        reset_read_state();
                        break;
                    }
    
                    const auto message_size = read_buffer.read_u16();

                    if (message_size > read_buffer_size - header_size) {
                        cout << "<*CLIENT*>: Received a message that exceeds buffer size from server (this is a bug)" << endl;
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
        cerr << "Closing due to error: " << error.what() << endl;
        return helper(true);
    }

    return helper();
}

void Client::parse_message() {
    switch (server_message_type) {
        case ServerMessageType::HeaderErrorResponse:
            parse_and_handle_header_error_response();
            break;

        case ServerMessageType::ListUsersResponse:
            parse_and_handle_list_users_response_message();
            break;

        case ServerMessageType::LoginResponse:
            parse_and_handle_login_response_message();
            break;

        case ServerMessageType::LogoutResponse:
            parse_and_handle_logout_response_message();
            break;

        case ServerMessageType::RegisterResponse:
            parse_and_handle_register_response_message();
            break;
        
        case ServerMessageType::SendPrivateMessageEvent:
            parse_and_handle_send_private_message_event_message();
            break;

        case ServerMessageType::SendPrivateMessageResponse:
            parse_and_handle_send_private_message_response_message();
            break;

        case ServerMessageType::SendPublicMessageEvent:
            parse_and_handle_send_public_message_event_message();
            break;
        
        case ServerMessageType::SendPublicMessageResponse:
            parse_and_handle_send_public_message_response_message();
            break;
    }
}

void Client::parse_and_handle_header_error_response() {
    const auto error_code = static_cast<HeaderErrorCode>(read_buffer.read_u8());

    cout << "<*SERVER*>: Message header error -";

    switch (error_code) {
        case HeaderErrorCode::MaximumMessageSizeExceeded:
            cout << "Maximum message size exceeded";
            break;
        
        case HeaderErrorCode::UnknownMessageType:
            cout << "Unknown message type";
            break;
    }

    cout << " (this is a bug)" << endl;
}

void Client::parse_and_handle_list_users_response_message() {
    const auto response_code = static_cast<ListUsersResponseCode>(read_buffer.read_u8());

    switch (response_code) {
        case ListUsersResponseCode::Success: {
            const auto user_count = read_buffer.read_u8();
            
            cout << "<*SERVER*>: " << static_cast<size_t>(user_count) << " user(s) online: " << endl;
            
            for (size_t i = 0; i < user_count; ++i) {
                const auto name_length = read_buffer.read_u8();
                string name;
                name.reserve(name_length);
        
                for (size_t j = 0; j < name_length; ++j) {
                    name += read_buffer.read_u8();
                }
        
                cout << " - " << name << endl;
            }

            break;
        }

        case ListUsersResponseCode::Unauthenticated:
            cout << "<*SERVER*> List users error - Not logged in" << endl;
            break;
    }
}

void Client::parse_and_handle_login_response_message() {
    const auto response_code = static_cast<LoginResponseCode>(read_buffer.read_u8());

    switch (response_code) {
        case LoginResponseCode::Success:
            cout << "<*SERVER*>: Successfully logged in" << endl;
            break;
        
        case LoginResponseCode::IncorrectPassword:
            cout << "<*SERVER*>: Login error - Incorrect password" << endl;
            break;
        
        case LoginResponseCode::InvalidName:
            cout << "<*SERVER*>: Login error - Invalid name (name can contain only alphanumerical characters)" << endl;
            break;

        case LoginResponseCode::InvalidNameLength:
            cout << "<*SERVER*>: Login error - Invalid name length (name must be between 4 and 8 characters)" << endl;
            break;
        
        case LoginResponseCode::InvalidPassword:
            cout << "<*SERVER*>: Login error - Invalid password (password can contain only alphanumerical characters)" << endl;
            break;

        case LoginResponseCode::InvalidPasswordLength:
            cout << "<*SERVER*>: Login error - Invalid password length (password must be between 4 and 8 characters)" << endl;
            break;

        case LoginResponseCode::MissingName:
            cout << "<*SERVER*>: Login error - Missing name (this is a bug)" << endl;
            break;

        case LoginResponseCode::MissingNameLength:
            cout << "<*SERVER*>: Login error - Missing name length (this is a bug)" << endl;
            break;
        
        case LoginResponseCode::MissingPassword:
            cout << "<*SERVER*>: Login error - Missing password (this is a bug)" << endl;
            break;

        case LoginResponseCode::MissingPasswordLength:
            cout << "<*SERVER*>: Login error - Missing password length (this is a bug)" << endl;
            break;

        case LoginResponseCode::Unauthorized:
            cout << "<*SERVER*>: Login error - Already logged in" << endl;
            break;

        case LoginResponseCode::UserDoesNotExist:
            cout << "<*SERVER*>: Login error - User does not exist" << endl;
            break;
    }
}

void Client::parse_and_handle_logout_response_message() {
    const auto response_code = static_cast<LogoutResponseCode>(read_buffer.read_u8());

    switch (response_code) {
        case LogoutResponseCode::Success:
            cout << "<*SERVER*>: Successfully logged out" << endl;
            break;
        
        case LogoutResponseCode::Unauthenticated:
            cout << "<*SERVER*>: Logout error - Not logged in" << endl;
            break;
    }
}

void Client::parse_and_handle_register_response_message() {
    const auto response_code = static_cast<RegisterResponseCode>(read_buffer.read_u8());

    switch (response_code) {
        case RegisterResponseCode::Success:
            cout << "<*SERVER*>: Successfully registered (you can login now)" << endl;
            break;

        case RegisterResponseCode::InvalidName:
            cout << "<*SERVER*>: Register error - Invalid name (name can contain only alphanumerical characters)" << endl;
            break;

        case RegisterResponseCode::InvalidNameLength:
            cout << "<*SERVER*>: Register error - Invalid name length (name must be between 4 and 8 characters)" << endl;
            break;
        
        case RegisterResponseCode::InvalidPassword:
            cout << "<*SERVER*>: Register error - Invalid password (password can contain only alphanumerical characters)" << endl;
            break;

        case RegisterResponseCode::InvalidPasswordLength:
            cout << "<*SERVER*>: Register error - Invalid password length (password must be between 4 and 8 characters)" << endl;
            break;

        case RegisterResponseCode::MissingName:
            cout << "<*SERVER*>: Register error - Missing name (this is a bug)" << endl;
            break;

        case RegisterResponseCode::MissingNameLength:
            cout << "<*SERVER*>: Register error - Missing name length (this is a bug)" << endl;
            break;
        
        case RegisterResponseCode::MissingPassword:
            cout << "<*SERVER*>: Register error - Missing password (this is a bug)" << endl;
            break;

        case RegisterResponseCode::MissingPasswordLength:
            cout << "<*SERVER*>: Register error - Missing password length (this is a bug)" << endl;
            break;

        case RegisterResponseCode::Unauthorized:
            cout << "<*SERVER*>: Register error - Cannot register when logged in" << endl;
            break;

        case RegisterResponseCode::UserAlreadyRegistered:
            cout << "<*SERVER*>: Register error - User already registered" << endl;
            break;
    }
}

void Client::parse_and_handle_send_private_message_event_message() {
    const auto options = read_buffer.read_u8();
    const auto is_anonymous = options & 0x01;

    if (!is_anonymous) {
        const auto name_length = read_buffer.read_u8();
        string name;

        for (size_t i = 0; i < name_length; ++i) {
            name += read_buffer.read_u8();
        }

        const auto message_length = read_buffer.read_u16();
        string message;

        for (size_t i = 0; i < message_length; ++i) {
            message += read_buffer.read_u8();
        }

        cout << "<~" << name << "~>: " << message << endl;
    } else {
        const auto message_length = read_buffer.read_u16();
        string message;

        for (size_t i = 0; i < message_length; ++i) {
            message += read_buffer.read_u8();
        }

        cout << "<~ANONYMOUS~>: " << message << endl;
    }
}

void Client::parse_and_handle_send_private_message_response_message() {
    const auto response_code = static_cast<SendPrivateMessageResponseCode>(read_buffer.read_u8());
    
    switch (response_code) {
        case SendPrivateMessageResponseCode::Success:
            break;

        case SendPrivateMessageResponseCode::CannotMessageSelf:
            cout << "<*SERVER*>: Send private message error - Cannot private message yourself" << endl;
            break;
        
        case SendPrivateMessageResponseCode::InvalidMessage:
            cout << "<*SERVER*>: Send private message error - Invalid message (message can only contain printable characters)" << endl;
            break;
        
        case SendPrivateMessageResponseCode::InvalidMessageLength:
            cout << "<*SERVER*>: Send private message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
            break;

        case SendPrivateMessageResponseCode::InvalidName:
            cout << "<*SERVER*>: Send private message error - Invalid name (name can contain only alphanumerical characters)" << endl;
            break;
        
        case SendPrivateMessageResponseCode::InvalidNameLength:
            cout << "<*SERVER*>: Send private message error - Invalid name length (name must be between 4 and 8 characters)" << endl;
            break;

        case SendPrivateMessageResponseCode::MissingMessage:
            cout << "<*SERVER*>: Send private message error - Missing message (this is a bug)" << endl;
            break;

        case SendPrivateMessageResponseCode::MissingMessageLength:
            cout << "<*SERVER*>: Send private message error - Missing message length (this is a bug)" << endl;
            break;

        case SendPrivateMessageResponseCode::MissingName:
            cout << "<*SERVER*>: Send private message error - Missing name (this is a bug)" << endl;
            break;

        case SendPrivateMessageResponseCode::MissingNameLength:
            cout << "<*SERVER*>: Send private message error - Missing name length (this is a bug)" << endl;
            break;

        case SendPrivateMessageResponseCode::MissingOptions:
            cout << "<*SERVER*>: Send private message error - Missing options (this is a bug)" << endl;
            break;

        case SendPrivateMessageResponseCode::Unauthenticated:
            cout << "<*SERVER*>: Send private message error - Not logged in" << endl;
            break;
        
        case SendPrivateMessageResponseCode::UserNotOnline:
            cout << "<*SERVER*>: Send private message error - No such user" << endl;
            break;
    }
}

void Client::parse_and_handle_send_public_message_event_message() {
    const auto options = read_buffer.read_u8();
    const auto is_anonymous = options & 0x01;

    if (!is_anonymous) {
        const auto name_length = read_buffer.read_u8();
        string name;

        for (size_t i = 0; i < name_length; ++i) {
            name += read_buffer.read_u8();
        }

        const auto message_length = read_buffer.read_u16();
        string message;

        for (size_t i = 0; i < message_length; ++i) {
            message += read_buffer.read_u8();
        }

        cout << "<" << name << ">: " << message << endl;
    } else {
        const auto message_length = read_buffer.read_u16();
        string message;

        for (size_t i = 0; i < message_length; ++i) {
            message += read_buffer.read_u8();
        }

        cout << "<*ANONYMOUS*>: " << message << endl;
    }
}

void Client::parse_and_handle_send_public_message_response_message() {
    const auto response_code = static_cast<SendPublicMessageResponseCode>(read_buffer.read_u8());

    switch (response_code) {
        case SendPublicMessageResponseCode::Success:
            break;
        
        case SendPublicMessageResponseCode::InvalidMessage:
            cout << "<*SERVER*>: Send message error - Invalid message (message can only contain printable characters)" << endl;
            break;
        
        case SendPublicMessageResponseCode::InvalidMessageLength:
            cout << "<*SERVER*>: Send message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
            break;

        case SendPublicMessageResponseCode::MissingMessage:
            cout << "<*SERVER*>: Send message error - Missing message (this is a bug)" << endl;
            break;

        case SendPublicMessageResponseCode::MissingMessageLength:
            cout << "<*SERVER*>: Send message error - Missing message length (this is a bug)" << endl;
            break;

        case SendPublicMessageResponseCode::MissingOptions:
            cout << "<*SERVER*>: Send message error - Missing options (this is a bug)" << endl;
            break;

        case SendPublicMessageResponseCode::Unauthenticated:
            cout << "<*SERVER*>: Send message error - Not logged in" << endl;
            break;
    }
}

void Client::reset_read_state() {
    read_buffer.reset(header_size);
    read_state = ReadState::MessageHeader;
}

void Client::write() {
    lock_guard<mutex> lock(write_buffer_mutex);
    
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
                cerr << "Closing due to error: " << error.what() << endl;
                break;
            
            default:
                throw error;
        }
    }
}

void Client::handle_list_command() {
    lock_guard<mutex> lock(write_buffer_mutex);
    
    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::ListUsers));
    write_buffer.write_u16(0);
}

void Client::handle_login_command(const string& name, const string& password) {
    lock_guard<mutex> lock(write_buffer_mutex);
    const auto request_size = name.size() + password.size() + 2;

    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::Login));
    write_buffer.write_u16(request_size);
    write_buffer.write_u8(name.size());
    
    for (const auto c : name) {
        write_buffer.write_u8(c);
    }

    write_buffer.write_u8(password.size());

    for (const auto c : password) {
        write_buffer.write_u8(c);
    }
}

void Client::handle_logout_command() {
    lock_guard<mutex> lock(write_buffer_mutex);

    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::Logout));
    write_buffer.write_u16(0);
}

void Client::handle_quit_command() {
    handle_logout_command();
}

void Client::handle_register_command(const string& name, const string& password) {
    lock_guard<mutex> lock(write_buffer_mutex);
    const auto request_size = name.size() + password.size() + 2;

    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::Register));
    write_buffer.write_u16(request_size);
    write_buffer.write_u8(name.size());
    
    for (const auto c : name) {
        write_buffer.write_u8(c);
    }

    write_buffer.write_u8(password.size());

    for (const auto c : password) {
        write_buffer.write_u8(c);
    }
}

void Client::handle_send_command(const string& message, const bool anonymous) {
    lock_guard<mutex> lock(write_buffer_mutex);
    const auto request_size = message.size() + 3;

    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::SendPublicMessage));
    write_buffer.write_u16(request_size);
    write_buffer.write_u8(static_cast<unsigned char>(anonymous));
    write_buffer.write_u16(message.size());
    
    for (const auto c : message) {
        write_buffer.write_u8(c);
    }
}

void Client::handle_sendpriv_command(const string& name, const string& message, const bool anonymous) {
    lock_guard<mutex> lock(write_buffer_mutex);
    const auto request_size = name.size() + message.size() + 4;

    write_buffer.write_u8(static_cast<unsigned char>(ClientMessageType::SendPrivateMessage));
    write_buffer.write_u16(request_size);
    write_buffer.write_u8(static_cast<unsigned char>(anonymous));
    write_buffer.write_u8(name.size());

    for (const auto c : name) {
        write_buffer.write_u8(c);
    }

    write_buffer.write_u16(message.size());
    
    for (const auto c : message) {
        write_buffer.write_u8(c);
    }
}

void Client::parse_list_command(string command, string input_line) {
	if (input_line.size() != command.size()) {
        cerr << "<*CLIENT*>: Invalid use of \"list\" command - Usage: list" << endl;
		return;
    }

	handle_list_command();
}

void Client::parse_login_command(string command, string input_line) {
    auto print_error_message = [&]() {
        cerr << "<*CLIENT*>: Invalid use of \"login\" command - Usage: login name password" << endl;
    };

	if (input_line.size() == command.size()) {
		print_error_message();
		return;
    }
    
    const auto parts = split(input_line.substr(command.size() + 1), " ");
    
    if (parts.size() != 2) {
        print_error_message();
		return;
    }

	handle_login_command(parts[0], parts[1]);
}

void Client::parse_logout_command(string command, string input_line) {
	if (input_line.size() != command.size()) {
        cerr << "<*CLIENT*>: Invalid use of \"logout\" command - Usage: logout" << endl;
		return;
    }

	handle_logout_command();
}

bool Client::parse_quit_command(string command, string input_line) {
	if (input_line.size() > command.size()) {
		cerr << "<*CLIENT*>: Invalid use of \"quit\" command - Usage: quit" << endl;
		return false;
	}

	handle_quit_command();
	return true;
}

void Client::parse_register_command(string command, string input_line) {
    auto print_error_message = [&]() {
        cerr << "<*CLIENT*>: Invalid use of \"register\" command - Usage: register name password" << endl;
    };

	if (input_line.size() == command.size()) {
		print_error_message();
		return;
    }
    
    const auto parts = split(input_line.substr(command.size() + 1), " ");
    
    if (parts.size() != 2) {
        print_error_message();
		return;
    }

	handle_register_command(parts[0], parts[1]);
}

void Client::parse_send_command(std::string command, std::string input_line) {
	if (input_line.size() <= command.size() + 1) {
		cerr << "<*CLIENT*>: Invalid use of \"send\" command - Usage: send message" << endl;
		return;
    }
    
    const auto message = input_line.substr(command.size() + 1);
    
    if (message.size() > 4096) {
        cerr << "<*CLIENT*>: Send message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
        return;
    }

	handle_send_command(message, false);
}

void Client::parse_senda_command(std::string command, std::string input_line) {
	if (input_line.size() <= command.size() + 1) {
		cerr << "<*CLIENT*>: Invalid use of \"senda\" command - Usage: senda message" << endl;
		return;
    }
    
    const auto message = input_line.substr(command.size() + 1);
    
    if (message.size() > 4096) {
        cerr << "<*CLIENT*>: Send anonymous message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
        return;
    }

	handle_send_command(message, true);
}

void Client::parse_sendpriv_command(string command, string input_line) {
    auto print_error_message = [&]() {
        cerr << "<*CLIENT*>: Invalid use of \"sendpriv\" command - Usage: sendpriv name message" << endl;
    };

	if (input_line.size() == command.size()) {
		print_error_message();
		return;
    }
    
    const auto parts = split(input_line.substr(command.size() + 1), " ");
    
    if (parts.size() < 2) {
        print_error_message();
        return;
    } else if (parts.size() == 2 && parts[1] == "") {
        print_error_message();
        return;
    }

    const auto message = input_line.substr(command.size() + parts[0].size() + 2);

    if (message.size() > 4096) {
        cerr << "<*CLIENT*>: Send private message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
        return;
    }

	handle_sendpriv_command(parts[0], message, false);
}

void Client::parse_sendpriva_command(string command, string input_line) {
    auto print_error_message = [&]() {
        cerr << "<*CLIENT*>: Invalid use of \"sendpriva\" command - Usage: sendpriva name message" << endl;
    };

	if (input_line.size() == command.size()) {
		print_error_message();
		return;
    }
    
    const auto parts = split(input_line.substr(command.size() + 1), " ");
    
    if (parts.size() < 2) {
        print_error_message();
        return;
    } else if (parts.size() == 2 && parts[1] == "") {
        print_error_message();
        return;
    }

    const auto message = input_line.substr(command.size() + parts[0].size() + 2);

    if (message.size() > 4096) {
        cerr << "<*CLIENT*>: Send private message error - Invalid message length (message must be between 1 and 4096 characters)" << endl;
        return;
    }

	handle_sendpriv_command(parts[0], message, true);
}

void Client::ui_handler() {
    string input_line;
    
	while (true) {
		getline(cin, input_line);

		const auto command_end_index = input_line.find_first_of(' ');
		auto command = input_line.substr(0, command_end_index);

		transform(command.begin(), command.end(), command.begin(), ::tolower);

        if (command == "list") {
            parse_list_command(command, input_line);
        } else if (command == "login") {
			parse_login_command(command, input_line);
		} else if (command == "logout") {
		 	parse_logout_command(command, input_line);
		} else if (command == "send") {
            parse_send_command(command, input_line);
        } else if (command == "senda") {
            parse_senda_command(command, input_line);  
        } else if (command == "sendpriv") {
            parse_sendpriv_command(command, input_line);
        } else if (command == "sendpriva") {
            parse_sendpriva_command(command, input_line);
        } else if (command == "quit") {
			if (parse_quit_command(command, input_line)) {
                is_running = false;
				break;
			}
		} else if (command == "register") {
            parse_register_command(command, input_line);
        } else {
			cerr << "<*CLIENT*>: Unknown command \"" << command << "\"" << endl;
		}
    }
}
