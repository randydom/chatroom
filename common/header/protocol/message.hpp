#pragma once

namespace protocol {
    constexpr std::size_t header_size = 3;
  
    enum class ReadState {
        MessageData,
        MessageHeader
    };

    enum class ClientMessageType {
        ListUsers,
        Login,
        Logout,
        Register,
        SendPrivateMessage,
        SendPublicMessage
    };

    enum class ServerMessageType {
        HeaderErrorResponse,
        ListUsersResponse,
        LoginResponse,
        LogoutResponse,
        RegisterResponse,
        SendPrivateMessageEvent,
        SendPrivateMessageResponse,
        SendPublicMessageEvent,
        SendPublicMessageResponse
    };

    enum class HeaderErrorCode {
        MaximumMessageSizeExceeded,
        UnknownMessageType
    };

    enum class ListUsersResponseCode {
        Success,

        Unauthenticated
    };

    enum class LoginResponseCode {
        Success,

        IncorrectPassword,
        InvalidName,
        InvalidNameLength,
        InvalidPassword,
        InvalidPasswordLength,
        MissingName,
        MissingNameLength,
        MissingPassword,
        MissingPasswordLength,
        Unauthorized,
        UserDoesNotExist
    };

    enum class LogoutResponseCode {
        Success,

        Unauthenticated
    };

    enum class RegisterResponseCode {
        Success,

        InvalidName,
        InvalidNameLength,
        InvalidPassword,
        InvalidPasswordLength,
        MissingName,
        MissingNameLength,
        MissingPassword,
        MissingPasswordLength,
        Unauthorized,
        UserAlreadyRegistered
    };

    enum class SendPrivateMessageResponseCode {
        Success,

        CannotMessageSelf,
        InvalidMessage,
        InvalidMessageLength,
        InvalidName,
        InvalidNameLength,
        MissingMessage,
        MissingMessageLength,
        MissingName,
        MissingNameLength,
        MissingOptions,
        Unauthenticated,
        UserNotOnline
    };

    enum class SendPublicMessageResponseCode {
        Success,

        InvalidMessage,
        InvalidMessageLength,
        MissingMessage,
        MissingMessageLength,
        MissingOptions,
        Unauthenticated
    };
}
