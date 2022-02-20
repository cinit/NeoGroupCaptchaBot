//
// Created by kinit on 2022-02-18.
//
#include <iostream>

#include "SessionManager.h"
#include "utils/log/Log.h"
#include "utils/SyncUtils.h"

#include "ClientSession.h"

static constexpr const auto LOG_TAG = "ClientSession";

namespace td_api = td::td_api;

namespace core {

static std::string optionValueToString(const td_api::object_ptr<td_api::OptionValue> &option_value) {
    if (option_value == nullptr) {
        return "<null>";
    }
    switch (option_value->get_id()) {
        case td_api::optionValueBoolean::ID:
            return std::to_string(static_cast<const td_api::optionValueBoolean *>(option_value.get())->value_);
        case td_api::optionValueEmpty::ID:
            return "<empty>";
        case td_api::optionValueInteger::ID:
            return std::to_string(static_cast<const td_api::optionValueInteger *>(option_value.get())->value_);
        case td_api::optionValueString::ID:
            return std::string("\"") + static_cast<const td_api::optionValueString *>(option_value.get())->value_ + "\"";
        default:
            return "<UNKNOWN ID: " + std::to_string(option_value->get_id()) + ">";
    }
}

static std::string messageContentToString(const td_api::MessageContent *message_content) {
    if (message_content == nullptr) {
        return "<null>";
    }
    switch (message_content->get_id()) {
        case td_api::messageText::ID:
            return std::string("\"") + static_cast<const td_api::messageText *>(message_content)->text_->text_ + "\"";
        case td_api::messageAnimation::ID:
            return "<animation>";
        case td_api::messageAudio::ID:
            return "<audio>";
        case td_api::messageDocument::ID:
            return "<document>";
        case td_api::messagePhoto::ID:
            return "<photo>";
        case td_api::messageSticker::ID:
            return "<sticker>";
        case td_api::messageVideo::ID:
            return "<video>";
        case td_api::messageVideoNote::ID:
            return "<videoNote>";
        case td_api::messageVoiceNote::ID:
            return "<voiceNote>";
        case td_api::messageLocation::ID:
            return "<location>";
        case td_api::messageVenue::ID:
            return "<venue>";
        case td_api::messageContact::ID:
            return "<contact>";
        case td_api::messageDice::ID:
            return "<dice>";
        case td_api::messageGame::ID:
            return "<game>";
        case td_api::messagePoll::ID:
            return "<poll>";
        case td_api::messageInvoice::ID:
            return "<invoice>";
        case td_api::messageCall::ID:
            return "<call>";
        case td_api::messageBasicGroupChatCreate::ID:
            return "<basicGroupChatCreate>";
        case td_api::messageSupergroupChatCreate::ID:
            return "<supergroupChatCreate>";
        case td_api::messageChatChangeTitle::ID:
            return "<chatChangeTitle>";
        case td_api::messageChatChangePhoto::ID:
            return "<chatChangePhoto>";
        case td_api::messageChatDeletePhoto::ID:
            return "<chatDeletePhoto>";
        case td_api::messageChatAddMembers::ID:
            return "<chatAddMembers>";
        case td_api::messageChatJoinByLink::ID:
            return "<chatJoinByLink>";
        case td_api::messageChatDeleteMember::ID:
            return "<chatDeleteMember>";
        case td_api::messageChatUpgradeTo::ID:
            return "<chatUpgradeTo>";
        case td_api::messageChatUpgradeFrom::ID:
            return "<chatUpgradeFrom>";
        case td_api::messagePinMessage::ID:
            return "<pinMessage>";
        case td_api::messageScreenshotTaken::ID:
            return "<screenshotTaken>";
        case td_api::messageChatSetTtl::ID:
            return "<chatSetTtl>";
        case td_api::messageCustomServiceAction::ID:
            return "<customServiceAction>";
        case td_api::messageGameScore::ID:
            return "<gameScore>";
        case td_api::messagePaymentSuccessful::ID:
            return "<paymentSuccessful>";
        case td_api::messagePaymentSuccessfulBot::ID:
            return "<paymentSuccessfulBot>";
        case td_api::messageContactRegistered::ID:
            return "<contactRegistered>";
        case td_api::messageWebsiteConnected::ID:
            return "<websiteConnected>";
        case td_api::messagePassportDataSent::ID:
            return "<passportDataSent>";
        case td_api::messagePassportDataReceived::ID:
            return "<passportDataReceived>";
        case td_api::messageUnsupported::ID:
            return "<unsupported>";
        default:
            return "<UNKNOWN ID: " + std::to_string(message_content->get_id()) + ">";
    }
}

static std::string messageToString(const td_api::message *msg) {
    if (msg == nullptr) {
        return "<null>";
    }
    std::string result;
    result += "{";
    result += "\"type\":\"" + std::to_string(msg->get_id()) + "\",";
    result += "\"chat_id\":\"" + std::to_string(msg->chat_id_) + "\",";

    const auto *content = msg->content_.get();
    if (content == nullptr) {
        result += "\"content\":<null>";
    } else {
        result += "\"content\":" + messageContentToString(content);
    }
    result += "}";
    return result;
}

ClientSession::ClientSession(SessionManager *sessionManager, int32_t id, const TdLibParameters &param)
        : mSessionManager(sessionManager), mTdLibParameters(param), mTdLibObjectId(id) {}

int ClientSession::getTdLibObjectId() const {
    return mTdLibObjectId;
}

void ClientSession::execute(td::td_api::object_ptr<td::td_api::Function> request,
                            std::function<void(td::td_api::object_ptr<td::td_api::Object>)> callback) {
    mSessionManager->sendRequestWithClientId(mTdLibObjectId, std::move(request), std::move(callback));
}

void ClientSession::execute(td::td_api::object_ptr<td::td_api::Function> request, nullptr_t) {
    mSessionManager->sendRequestWithClientId(mTdLibObjectId, std::move(request), nullptr);
}

void ClientSession::terminate() {
    mSessionManager->terminateSession(getTdLibObjectId());
}

void ClientSession::onTerminate() {
    // nothing
}

bool ClientSession::handleUpdate(td::td_api::object_ptr<td::td_api::Object> update) {
    if (update == nullptr) {
        return false;
    }
    int32_t updateType = update->get_id();
    switch (updateType) {
        case td_api::updateAuthorizationState::ID: {
            auto updateAuthState = td_api::move_object_as<td_api::updateAuthorizationState>(std::move(update));
            auto state = std::move(updateAuthState->authorization_state_);
            return handleUpdateAuthorizationState(std::move(state));
        }
        case td_api::updateOption::ID: {
            auto updateOption = td_api::move_object_as<td_api::updateOption>(std::move(update));
            std::string name = updateOption->name_;
            auto value = std::move(updateOption->value_);
            handleUpdateOption(name, value);
            return true;
        }
        case td_api::updateConnectionState::ID: {
            auto updateConnectionState = td_api::move_object_as<td_api::updateConnectionState>(std::move(update));
            if (updateConnectionState->state_ != nullptr) {
                auto state = std::move(updateConnectionState->state_);
                handleUpdateConnectionState(state->get_id());
            }
            return true;
        }
        case td_api::updateAnimationSearchParameters::ID: {
            LOGD("updateAnimationSearchParameters not implemented");
            return true;
        }
        case td_api::updateSelectedBackground::ID: {
            LOGD("updateSelectedBackground not implemented");
            return true;
        }
        case td_api::updateChatThemes::ID: {
            LOGD("updateChatThemes not implemented");
            return true;
        }
        case td_api::updateDiceEmojis::ID: {
            LOGD("updateDiceEmojis not implemented");
            return true;
        }
        case td_api::updateRecentStickers::ID: {
            LOGD("updateRecentStickers not implemented");
            return true;
        }
        case td_api::updateHavePendingNotifications::ID: {
            LOGI("TODO: updateHavePendingNotifications not implemented");
            return true;
        }
        case td_api::updateChatFilters::ID: {
            LOGW("TODO: updateChatFilters not implemented");
            return true;
        }
        case td_api::updateChatActionBar::ID: {
            LOGW("TODO: updateChatActionBar not implemented");
            return true;
        }
        case td_api::updateChatTitle::ID: {
            LOGW("TODO: updateChatTitle not implemented");
            return true;
        }
        case td_api::updateScopeNotificationSettings::ID: {
            LOGI("TODO: updateScopeNotificationSettings not implemented");
            return true;
        }
        case td_api::updateChatMessageTtl::ID: {
            LOGI("TODO: updateChatMessageTtl not implemented");
            return true;
        }
        case td_api::updateReactions::ID: {
            LOGW("TODO: updateReactions not implemented");
            return true;
        }
        case td_api::updateMessageContent::ID: {
            LOGW("TODO: updateMessageContent not implemented");
            return true;
        }
        case td_api::updateMessageEdited::ID: {
            LOGW("TODO: updateMessageEdited not implemented");
            return true;
        }
        case td_api::updateChatLastMessage::ID: {
            return true;
        }
        case td_api::updateChatReadInbox::ID: {
            LOGI("TODO: updateChatReadInbox not implemented");
            return true;
        }
        case td_api::updateChatReadOutbox::ID: {
            LOGI("TODO: updateChatReadOutbox not implemented");
            return true;
        }
        case td_api::updateChatNotificationSettings::ID: {
            LOGI("TODO: updateChatNotificationSettings not implemented");
            return true;
        }
        case td_api::updateChatPosition::ID: {
            return true;
        }
        case td_api::updateSupergroupFullInfo::ID: {
            LOGW("TODO: updateSupergroupFullInfo not implemented");
            return true;
        }
        case td_api::updateChatUnreadMentionCount::ID: {
            LOGW("TODO: updateChatUnreadMentionCount not implemented");
            return true;
        }
        case td_api::updateMessageMentionRead::ID: {
            LOGD("updateMessageMentionRead not implemented");
            return true;
        }
        case td_api::updateChatVideoChat::ID: {
            LOGW("TODO: updateChatVideoChat not implemented");
            return true;
        }
        case td_api::updateUserStatus::ID: {
            LOGW("TODO: updateUserStatus not implemented");
            return true;
        }
        case td_api::updateChatAction::ID: {
            LOGW("TODO: updateChatAction not implemented");
            return true;
        }
        case td_api::updateUserFullInfo::ID: {
            LOGW("TODO: updateUserFullInfo not implemented");
            return true;
        }
        case td_api::updateChatAvailableReactions::ID: {
            LOGI("TODO: updateChatAvailableReactions not implemented");
            return true;
        }
        case td_api::updateChatMessageSender::ID: {
            LOGW("TODO: updateChatMessageSender not implemented");
            return true;
        }
        case td_api::updateMessageInteractionInfo::ID: {
            LOGI("TODO: updateMessageInteractionInfo not implemented");
            return true;
        }
        case td_api::updateUser::ID: {
            auto updateUser = td_api::move_object_as<td_api::updateUser>(std::move(update));
            handleUpdateUser(std::move(updateUser->user_));
            return true;
        }
        case td_api::updateNewChat::ID: {
            auto updateNewChat = td_api::move_object_as<td_api::updateNewChat>(std::move(update));
            handleUpdateNewChat(std::move(updateNewChat->chat_));
            return true;
        }
        case td_api::updateNewMessage::ID: {
            auto updateNewMessage = td_api::move_object_as<td_api::updateNewMessage>(std::move(update));
            handleUpdateNewMessage(std::move(updateNewMessage->message_));
            return true;
        }
        case td_api::updateBasicGroup::ID: {
            auto updateBasicGroup = td_api::move_object_as<td_api::updateBasicGroup>(std::move(update));
            handleUpdateBasicGroup(std::move(updateBasicGroup->basic_group_));
            return true;
        }
        case td_api::updateSupergroup::ID: {
            auto updateSupergroup = td_api::move_object_as<td_api::updateSupergroup>(std::move(update));
            handleUpdateSupergroup(std::move(updateSupergroup->supergroup_));
            return true;
        }
        case td_api::updateDeleteMessages::ID: {
            auto updateDeleteMessages = td_api::move_object_as<td_api::updateDeleteMessages>(std::move(update));
            handleUpdateDeleteMessages(std::move(updateDeleteMessages));
            return true;
        }
        case td_api::updateMessageSendSucceeded::ID: {
            auto updateMessageSendSucceeded = td_api::move_object_as<td_api::updateMessageSendSucceeded>(std::move(update));
            handleUpdateMessageSendSucceeded(std::move(updateMessageSendSucceeded));
            return true;
        }
        default: {
            LOGE("Unknown update: id = %d", updateType);
            return false;
        }
    }
}

bool ClientSession::handleUpdateAuthorizationState(td::td_api::object_ptr<td::td_api::AuthorizationState> object) {
    if (object == nullptr) {
        return false;
    }
    int32_t type = object->get_id();
    int32_t clientId = mTdLibObjectId;
    LOGD("handleUpdateAuthorizationState: clientId = %d, type = %d", clientId, type);
    switch (type) {
        case td_api::authorizationStateWaitTdlibParameters::ID: {
            sendTdLibParameters();
            return true;
        }
        case td_api::authorizationStateWaitEncryptionKey::ID: {
            execute(td_api::make_object<td_api::checkDatabaseEncryptionKey>(), SessionManager::logIfResponseError);
            return true;
        }
        case td_api::authorizationStateWaitPhoneNumber::ID: {
            // check if we already have bot token
            if (mAuthState == AuthorizationState::INITIALIZATION) {
                // if we have access token, try to use it
                if (!mAuthBotToken.empty()) {
                    // send bot token
                    LOGD("try auth with bot token");
                    execute(td_api::make_object<td_api::checkAuthenticationBotToken>(mAuthBotToken), [this](auto result) {
                        if (result->get_id() == td_api::error::ID) {
                            auto error = td_api::move_object_as<td_api::error>(result);
                            LOGE("Error checking authentication bot token: %s", error->message_.c_str());
                            mAuthState = AuthorizationState::BAD_TOKEN;
                        } else {
                            if (result->get_id() == td_api::ok::ID) {
                                auto ok = td_api::move_object_as<td_api::ok>(result);
                                LOGD("auth success");
                                mAuthState = AuthorizationState::AUTHORIZED;
                            } else {
                                LOGE("Unexpected result checking authentication bot token: %d", result->get_id());
                                mAuthState = AuthorizationState::BAD_TOKEN;
                            }
                        }
                    });
                    mAuthBotToken.clear();
                    mAuthState = AuthorizationState::WAIT_RESPONSE;
                } else if (!mAuthUserPhone.empty()) {
                    // send phone number
                    LOGD("try auth with phone number");
                    auto authSettings = td_api::make_object<td_api::phoneNumberAuthenticationSettings>(false, false, false, false, std::vector<std::string>());
                    auto request = td_api::make_object<td_api::setAuthenticationPhoneNumber>(mAuthUserPhone, std::move(authSettings));
                    execute(std::move(request), [](auto resp) {
                        int32_t result = resp->get_id();
                        if (result == td_api::ok::ID) {
                            return;
                        }
                        LOGD("setAuthenticationPhoneNumber: %d", result);
                        if (resp->get_id() == td_api::error::ID) {
                            auto error = td_api::move_object_as<td_api::error>(resp);
                            LOGE("setAuthenticationPhoneNumber error: %s", error->message_.c_str());
                        }
                    });
                    mAuthBotToken.clear();
                    mAuthState = AuthorizationState::WAIT_RESPONSE;
                }
                return true;
            } else {
                mAuthState = AuthorizationState::WAIT_TOKEN;
            }
            return true;
        }
        case td_api::authorizationStateReady::ID: {
            mAuthState = AuthorizationState::AUTHORIZED;
            LOGI("Authorization success");
            return true;
        }
        case td_api::authorizationStateWaitCode::ID: {
            mAuthState = AuthorizationState::WAIT_CODE;
            LOGW("Authorization waiting code");
            // run on another thread
            SessionManager::getInstance().getExecutors().execute([this]() {
                std::cout << ">>> Enter code for client ID " << getTdLibObjectId() << ": " << std::endl;
                std::string code;
                std::getline(std::cin, code);
                // strip whitespace
                code.erase(std::remove_if(code.begin(), code.end(), isspace), code.end());
                // strip newline
                code.erase(std::remove(code.begin(), code.end(), '\n'), code.end());
                if (code.empty()) {
                    LOGE("Empty code, aborting");
                    mAuthState = AuthorizationState::WAIT_TOKEN;
                    return true;
                }
                execute(td_api::make_object<td_api::checkAuthenticationCode>(code), [this, code](auto result) {
                    if (result->get_id() == td_api::error::ID) {
                        auto error = td_api::move_object_as<td_api::error>(result);
                        LOGE("Error checking authentication code: %s", error->message_.c_str());
                        mAuthState = AuthorizationState::BAD_TOKEN;
                    } else {
                        if (result->get_id() == td_api::ok::ID) {
                            auto ok = td_api::move_object_as<td_api::ok>(result);
                            LOGD("send code '%s' success", code.c_str());
                            mAuthState = AuthorizationState::WAIT_RESPONSE;
                        } else {
                            LOGE("Unexpected result checking authentication code: %d", result->get_id());
                            mAuthState = AuthorizationState::BAD_TOKEN;
                        }
                    }
                });
                return true;
            });
            return true;
        }
        case td_api::authorizationStateWaitPassword::ID: {
            mAuthState = AuthorizationState::WAIT_PASSWORD;
            LOGW("Authorization waiting password");
            // run on another thread
            SessionManager::getInstance().getExecutors().execute([this]() {
                std::cout << ">>> Enter password for client ID " << getTdLibObjectId() << ": " << std::endl;
                std::string password;
                std::getline(std::cin, password);
                // strip newline
                password.erase(std::remove(password.begin(), password.end(), '\n'), password.end());
                if (password.empty()) {
                    LOGE("Empty password, aborting");
                    mAuthState = AuthorizationState::WAIT_TOKEN;
                    return true;
                }
                execute(td_api::make_object<td_api::checkAuthenticationPassword>(password), [this, password](auto result) {
                    if (result->get_id() == td_api::error::ID) {
                        auto error = td_api::move_object_as<td_api::error>(result);
                        LOGE("Error checking authentication password: %s", error->message_.c_str());
                        mAuthState = AuthorizationState::BAD_TOKEN;
                    } else {
                        if (result->get_id() == td_api::ok::ID) {
                            auto ok = td_api::move_object_as<td_api::ok>(result);
                            LOGD("send password <length=%d> success", int(password.size()));
                            mAuthState = AuthorizationState::WAIT_RESPONSE;
                        } else {
                            LOGE("Unexpected result checking authentication password: %d", result->get_id());
                            mAuthState = AuthorizationState::BAD_TOKEN;
                        }
                    }
                });
                return true;
            });
            return true;
        }
        default: {
            LOGE("unknown authorization state update: type = %d", type);
            return false;
        }
    }
}

void ClientSession::handleUpdateOption(const std::string &name, const td_api::object_ptr<td_api::OptionValue> &object) {
    if (name == "my_id" && object->get_id() == td_api::optionValueInteger::ID) {
        mUserId = static_cast<const td_api::optionValueInteger *>(object.get())->value_;
        LOGD("my_id = %ld", mUserId);
    } else if (name == "authorization_date" && object->get_id() == td_api::optionValueInteger::ID) {
        mAuthorizationDate = static_cast<const td_api::optionValueInteger *>(object.get())->value_;
        LOGD("authorization_date = %ld", mAuthorizationDate);
    } else if (name == "unix_time" && object->get_id() == td_api::optionValueInteger::ID) {
        uint64_t serverTimeSeconds = static_cast<const td_api::optionValueInteger *>(object.get())->value_;
        uint64_t localTimeSeconds = utils::getCurrentTimeMillis() / 1000;
        mServerTimeDeltaSeconds = serverTimeSeconds - localTimeSeconds;
        LOGD("server time delta = %ld", mServerTimeDeltaSeconds);
    } else {
        LOGI("ignore update option: %s = %s", name.c_str(), optionValueToString(object).c_str());
    }
}

bool ClientSession::isAuthorized() const {
    return mAuthState == AuthorizationState::AUTHORIZED;
}

void ClientSession::logInWithBotToken(const std::string &botToken) {
    if (botToken.empty()) {
        LOGE("bot token is empty");
        return;
    }
    if (mAuthState == AuthorizationState::AUTHORIZED) {
        LOGE("already authorized");
        return;
    }
    mAuthBotToken = botToken;
    if (mAuthState == AuthorizationState::WAIT_TOKEN || mAuthState == AuthorizationState::BAD_TOKEN) {
        LOGD("try auth with bot token");
        execute(td_api::make_object<td_api::checkAuthenticationBotToken>(mAuthBotToken), SessionManager::logIfResponseError);
        mAuthState = AuthorizationState::WAIT_RESPONSE;
    }
}

void ClientSession::logInWithPhoneNumber(const std::string &phoneNumber) {
    if (phoneNumber.empty()) {
        LOGE("phone number is empty");
        return;
    }
    if (mAuthState == AuthorizationState::AUTHORIZED) {
        LOGE("already authorized");
        return;
    }
    mAuthUserPhone = phoneNumber;
    if (mAuthState == AuthorizationState::WAIT_TOKEN || mAuthState == AuthorizationState::BAD_TOKEN) {
        LOGD("try auth with phone number");
        auto authSettings = td_api::make_object<td_api::phoneNumberAuthenticationSettings>(false, false, false, false, std::vector<std::string>());
        auto request = td_api::make_object<td_api::setAuthenticationPhoneNumber>(phoneNumber, std::move(authSettings));
        execute(std::move(request), [](auto resp) {
            int32_t result = resp->get_id();
            if (result == td_api::ok::ID) {
                return;
            }
            LOGD("setAuthenticationPhoneNumber: %d", result);
            if (resp->get_id() == td_api::error::ID) {
                auto error = td_api::move_object_as<td_api::error>(resp);
                LOGE("setAuthenticationPhoneNumber error: %s", error->message_.c_str());
            }
        });
        mAuthState = AuthorizationState::WAIT_RESPONSE;
    } else if (mAuthState == AuthorizationState::INITIALIZATION) {
        sendTdLibParameters();
    }
}

void ClientSession::handleUpdateConnectionState(int32_t state) {
    switch (state) {
        case td_api::connectionStateWaitingForNetwork::ID: {
            LOGI("ConnectionState: waiting for network");
            break;
        }
        case td_api::connectionStateConnectingToProxy::ID: {
            LOGI("ConnectionState: connecting to proxy");
            break;
        }
        case td_api::connectionStateConnecting::ID: {
            LOGI("ConnectionState: connecting");
            break;
        }
        case td_api::connectionStateUpdating::ID: {
            LOGI("ConnectionState: updating");
            break;
        }
        case td_api::connectionStateReady::ID: {
            LOGI("ConnectionState: ready");
            break;
        }
        default: {
            LOGE("unknown connection state update: %d", state);
            break;
        }
    }
}

ClientSession::AuthorizationState ClientSession::getAuthorizationState() const {
    return mAuthState;
}

void ClientSession::handleUpdateUser(td::td_api::object_ptr<td::td_api::user> user) {
    if (user) {
        mUser = std::move(user);
        std::string referenceName = mUser->username_;
        std::string name = mUser->first_name_;
        if (!mUser->last_name_.empty()) {
            name += " " + mUser->last_name_;
        }
        std::string info = name;
        if (!referenceName.empty()) {
            info += " @" + referenceName;
        }
        LOGI("User: %s", info.c_str());
        LOGI("User: id = %ld, is_fake = %d, is_verified = %d, is_support = %d",
             mUser->id_, mUser->is_fake_, mUser->is_verified_, mUser->is_support_);
    }
}

uint64_t ClientSession::getServerTimeSeconds() const {
    uint64_t currentTime = utils::getCurrentTimeMillis() / 1000;
    return currentTime + mServerTimeDeltaSeconds;
}

uint64_t ClientSession::getServerTimeMillis() const {
    uint64_t currentTime = utils::getCurrentTimeMillis();
    return currentTime + mServerTimeDeltaSeconds * 1000;
}

void ClientSession::handleUpdateNewChat(td::td_api::object_ptr<td::td_api::chat> chat) {
    if (chat) {
        LOGI("New chat: id = %ld, title = %s", chat->id_, chat->title_.c_str());
    }
}

void ClientSession::handleUpdateNewMessage(td::td_api::object_ptr<td::td_api::message> message) {
    if (message) {
        td::td_api::message *msg = message.get();
        if (msg->is_outgoing_) {
            // we don't need to handle outgoing messages
            return;
        }
        bool handled = false;
        if (mMessageHandler != nullptr) {
            handled = mMessageHandler.get()->operator()(this, msg);
        }
        if (!handled) {
            LOGI("Unhandled message: %s", messageToString(message.get()).c_str());
        }
    }
}

void ClientSession::handleUpdateSupergroup(td::td_api::object_ptr<td::td_api::supergroup> supergroup) {
    if (supergroup) {
        LOGI("Supergroup: id = %ld, ref_name = %s", supergroup->id_, supergroup->username_.c_str());
    }
}

void ClientSession::handleUpdateBasicGroup(td::td_api::object_ptr<td::td_api::basicGroup> basicGroup) {
    if (basicGroup) {
        LOGI("BasicGroup: id = %ld, upgraded_to_supergroup_id = %ld", basicGroup->id_, basicGroup->upgraded_to_supergroup_id_);
    }
}

ClientSession::MessageHandler *ClientSession::getMessageHandler() const {
    return mMessageHandler.get();
}

void ClientSession::setMessageHandler(ClientSession::MessageHandler messageHandler) {
    mMessageHandler = std::make_unique<ClientSession::MessageHandler>(std::move(messageHandler));
}

void ClientSession::handleUpdateDeleteMessages(td::td_api::object_ptr<td::td_api::updateDeleteMessages> update) {
    if (update) {
        std::string messageIds;
        for (auto const &messageId: update->message_ids_) {
            messageIds += std::to_string(messageId) + ", ";
        }
        LOGI("UpdateDeleteMessages: chat_id = %ld, message_ids = %s", update->chat_id_, messageIds.c_str());
    }
}

void ClientSession::sendTextMessage(int64_t chatId, const std::string &text, uint64_t replyId,
                                    std::function<void(td::td_api::object_ptr<td::td_api::Object>)> callback) {
    td::td_api::object_ptr<td::td_api::formattedText> formattedText =
            td::td_api::make_object<td::td_api::formattedText>(text, std::vector<td::td_api::object_ptr<td::td_api::textEntity>>());
    td::td_api::object_ptr<td::td_api::InputMessageContent> inputMessage =
            td::td_api::make_object<td::td_api::inputMessageText>(std::move(formattedText), false, false);
    td::td_api::object_ptr<td::td_api::sendMessage> request = td::td_api::make_object<td::td_api::sendMessage>(
            chatId, 0, replyId,
            td::td_api::make_object<td_api::messageSendOptions>(false, false, false, nullptr),
            nullptr, std::move(inputMessage));
    execute(std::move(request), std::move(callback));
}

void ClientSession::sendTextMessage(int64_t chatId, const std::string &text, uint64_t replyId) {
    sendTextMessage(chatId, text, replyId, [](td::td_api::object_ptr<td::td_api::Object> object) {
        if (object && object->get_id() == td::td_api::error::ID) {
            auto error = td::move_tl_object_as<td::td_api::error>(object);
            LOGE("sendTextMessage code:%d error: %s", error->code_, error->message_.c_str());
        }
    });
}

void ClientSession::handleUpdateMessageSendSucceeded(td::td_api::object_ptr<td::td_api::updateMessageSendSucceeded> update) {
    if (update) {
        LOGI("UpdateMessageSendSucceeded: message_id = %ld, message_thread_id = %ld",
             update->old_message_id_, update->message_->message_thread_id_);
    }
}

void ClientSession::sendTdLibParameters() {
    // construct parameters
    auto parameters = td_api::make_object<td_api::tdlibParameters>();
    parameters->use_test_dc_ = mTdLibParameters.use_test_dc_;
    parameters->database_directory_ = mTdLibParameters.database_directory_;
    parameters->files_directory_ = mTdLibParameters.files_directory_;
    parameters->use_file_database_ = mTdLibParameters.use_file_database_;
    parameters->use_chat_info_database_ = mTdLibParameters.use_chat_info_database_;
    parameters->use_message_database_ = mTdLibParameters.use_message_database_;
    parameters->use_secret_chats_ = mTdLibParameters.use_secret_chats_;
    parameters->api_id_ = mTdLibParameters.api_id_;
    parameters->api_hash_ = mTdLibParameters.api_hash_;
    parameters->system_language_code_ = mTdLibParameters.system_language_code_;
    parameters->device_model_ = mTdLibParameters.device_model_;
    parameters->system_version_ = mTdLibParameters.system_version_;
    parameters->application_version_ = mTdLibParameters.application_version_;
    parameters->enable_storage_optimizer_ = mTdLibParameters.enable_storage_optimizer_;
    parameters->ignore_file_names_ = mTdLibParameters.ignore_file_names_;
    // send parameters
    execute(td_api::make_object<td_api::setTdlibParameters>(
            std::move(parameters)), SessionManager::logIfResponseError);
    // send extra options: ignore_inline_thumbnails, reuse_uploaded_photos_by_hash,
    // disable_persistent_network_statistics, disable_time_adjustment_protection
    execute(td_api::make_object<td_api::setOption>(
            "ignore_inline_thumbnails", td_api::make_object<td_api::optionValueBoolean>(true)), SessionManager::logIfResponseError);
    execute(td_api::make_object<td_api::setOption>(
            "reuse_uploaded_photos_by_hash", td_api::make_object<td_api::optionValueBoolean>(true)), SessionManager::logIfResponseError);
    execute(td_api::make_object<td_api::setOption>(
            "disable_persistent_network_statistics", td_api::make_object<td_api::optionValueBoolean>(true)), SessionManager::logIfResponseError);
    execute(td_api::make_object<td_api::setOption>(
            "disable_time_adjustment_protection", td_api::make_object<td_api::optionValueBoolean>(true)), SessionManager::logIfResponseError);
}

}
