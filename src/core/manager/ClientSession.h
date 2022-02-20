//
// Created by kinit on 2022-02-18.
//

#ifndef NEOGROUPCAPTCHABOT_CLIENTSESSION_H
#define NEOGROUPCAPTCHABOT_CLIENTSESSION_H

#include <cstdint>

#include <string>
#include <cstdint>
#include <atomic>
#include <memory>
#include <functional>

#include <td/telegram/td_api.h>

namespace core {

class SessionManager;

class ClientSession {
public:
    using MessageHandler = std::function<bool(ClientSession *, const td::td_api::message *)>;

    struct TdLibParameters {
        bool use_test_dc_ = false;
        std::string database_directory_; // to be set by client
        std::string files_directory_;
        bool use_file_database_ = true;
        bool use_chat_info_database_ = true;
        bool use_message_database_ = false;
        bool use_secret_chats_ = false;
        int32_t api_id_; // to be set by client
        std::string api_hash_; // to be set by client
        std::string system_language_code_ = "en";
        std::string device_model_ = "server";
        std::string system_version_ = "Linux";
        std::string application_version_ = "1.0";
        bool enable_storage_optimizer_ = true;
        bool ignore_file_names_ = true;
    };

    enum class AuthorizationState {
        INITIALIZATION,
        WAIT_TOKEN,
        WAIT_CODE,
        WAIT_PASSWORD,
        WAIT_RESPONSE,
        AUTHORIZED,
        BAD_TOKEN,
        CLOSED
    };

    ClientSession() = delete;

    explicit ClientSession(SessionManager *sessionManager, int32_t id, const TdLibParameters &param);

    ~ClientSession() = default;

    ClientSession(const ClientSession &) = delete;

    ClientSession &operator=(const ClientSession &) = delete;

    [[nodiscard]] int getTdLibObjectId() const;

    void execute(td::td_api::object_ptr<td::td_api::Function> request,
                 std::function<void(td::td_api::object_ptr<td::td_api::Object>)> callback);

    void execute(td::td_api::object_ptr<td::td_api::Function> request, nullptr_t);

    void sendTextMessage(int64_t chatId, const std::string &text, uint64_t replyId,
                         std::function<void(td::td_api::object_ptr<td::td_api::Object>)> callback);

    void sendTextMessage(int64_t chatId, const std::string &text, uint64_t replyId = 0);

    [[nodiscard]] bool isAuthorized() const;

    [[nodiscard]] AuthorizationState getAuthorizationState() const;

    [[nodiscard]] uint64_t getServerTimeSeconds() const;

    [[nodiscard]] uint64_t getServerTimeMillis() const;

    [[nodiscard]] MessageHandler *getMessageHandler() const;

    void setMessageHandler(MessageHandler messageHandler);

    void logInWithBotToken(const std::string &botToken);

    void logInWithPhoneNumber(const std::string &botToken);

    bool handleUpdate(td::td_api::object_ptr<td::td_api::Object> update);

    void onTerminate();

    void terminate();

private:
    void sendTdLibParameters();

    bool handleUpdateAuthorizationState(td::td_api::object_ptr<td::td_api::AuthorizationState> object);

    void handleUpdateConnectionState(int32_t state);

    void handleUpdateUser(td::td_api::object_ptr<td::td_api::user> user);

    void handleUpdateNewChat(td::td_api::object_ptr<td::td_api::chat> chat);

    void handleUpdateNewMessage(td::td_api::object_ptr<td::td_api::message> message);

    void handleUpdateBasicGroup(td::td_api::object_ptr<td::td_api::basicGroup> basicGroup);

    void handleUpdateSupergroup(td::td_api::object_ptr<td::td_api::supergroup> supergroup);

    void handleUpdateDeleteMessages(td::td_api::object_ptr<td::td_api::updateDeleteMessages> update);

    void handleUpdateMessageSendSucceeded(td::td_api::object_ptr<td::td_api::updateMessageSendSucceeded> update);

    void handleUpdateOption(const std::string &name, const td::td_api::object_ptr<td::td_api::OptionValue> &object);

private:
    SessionManager *mSessionManager = nullptr;
    TdLibParameters mTdLibParameters;
    int32_t mTdLibObjectId = 0;
    int64_t mUserId = 0;
    uint64_t mAuthorizationDate = 0;
    std::atomic<AuthorizationState> mAuthState = AuthorizationState::INITIALIZATION;
    std::string mAuthBotToken;
    std::string mAuthUserPhone;
    td::tl_object_ptr<td::td_api::user> mUser;
    uint64_t mServerTimeDeltaSeconds = 0;
    std::unique_ptr<MessageHandler> mMessageHandler;
};

}

#endif //NEOGROUPCAPTCHABOT_CLIENTSESSION_H
