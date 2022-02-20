#include <unistd.h>
#include <sys/utsname.h>
#include <iostream>
#include <functional>
#include <string>

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>

#include "utils/file_utils.h"
#include "utils/ProcessUtils.h"
#include "utils/config/ConfigManager.h"
#include "manager/SessionManager.h"
#include "manager/ClientSession.h"
#include "utils/SyncUtils.h"
#include "utils/log/Log.h"

using namespace utils;
using utils::config::ConfigManager;
using core::SessionManager;
using core::ClientSession;
using utils::async;

namespace tdapi = td::td_api;

static constexpr const char *LOG_TAG = "startup";

int main() {
    Log::setLogHandler([](Log::Level level, const char *tag, const char *msg) {
        uint64_t timestamp = utils::getCurrentTimeMillis();
        uint64_t timeOfDay = timestamp % (24 * 60 * 60 * 1000) + 8 * 60 * 60 * 1000; // UTC+8
        // HH:MM:SS.SSS LEVEL TAG MSG
        int len = snprintf(nullptr, 0, "%02llu:%02llu:%02llu.%03llu %s %s %s",
                           timeOfDay / (60 * 60 * 1000),
                           (timeOfDay / (60 * 1000)) % 60,
                           (timeOfDay / 1000) % 60,
                           timestamp % 1000,
                           Log::levelToString(level),
                           tag,
                           msg);
        std::string str(len + 1, '\0');
        snprintf(&str[0], len + 1, "%02llu:%02llu:%02llu.%03llu %s %s %s",
                 timeOfDay / (60 * 60 * 1000),
                 (timeOfDay / (60 * 1000)) % 60,
                 (timeOfDay / 1000) % 60,
                 timestamp % 1000,
                 Log::levelToString(level),
                 tag,
                 msg);
        std::cout << str.c_str() << std::endl;
    });

    int32_t tgApiId = 0;
    std::string tgApiHash;
    std::string tgBotToken;
    std::string tgUserPhone;

    // read env vars
    if (const char *env = getenv("TG_API_ID")) {
        tgApiId = atoi(env);
    }
    if (const char *env = getenv("TG_API_HASH")) {
        tgApiHash = env;
    }
    if (const char *env = getenv("TG_BOT_TOKEN")) {
        tgBotToken = env;
    }
    if (const char *env = getenv("TG_USER_PHONE")) {
        tgUserPhone = env;
    }
    if (tgApiId <= 0 || tgApiHash.empty() || tgBotToken.empty() || tgUserPhone.empty()) {
        std::cerr << "Please set environ variable TG_API_ID, TG_API_HASH, TG_BOT_TOKEN and TG_USER_PHONE." << std::endl;
        return 1;
    }
    auto exePath = getCurrentExecutablePath();
    auto exeDir = getParentDirectory(exePath);
    if (exeDir.empty()) {
        std::cout << "get exe dir failed" << std::endl;
        return -1;
    }
    // set working directory
    if (chdir(exeDir.c_str()) != 0) {
        std::cout << "set working directory failed: " << exeDir << std::endl;
        return -1;
    }
//    ConfigManager::initialize(exeDir + kPathSeparator + "config");
//    auto &cfg = ConfigManager::getDefaultConfig();
    td::ClientManager::execute(tdapi::make_object<tdapi::setLogVerbosityLevel>(1));
    auto &sessionManager = SessionManager::getInstance();

    ClientSession::TdLibParameters parameters;
    parameters.api_id_ = tgApiId;
    parameters.api_hash_ = tgApiHash;
    parameters.use_test_dc_ = false;
    parameters.database_directory_ = exeDir + kPathSeparator + "database" + kPathSeparator + "bot_1";

    // update parameter system info
    if (struct utsname uts = {}; uname(&uts) == 0) {
        std::string os = std::string(uts.sysname) + " " + std::string(uts.machine);
        parameters.device_model_ = os;
        parameters.system_version_ = uts.release;
    }

    auto botClient = sessionManager.createSession(parameters);
    botClient->execute(tdapi::make_object<tdapi::getOption>("version"), nullptr);

    botClient->logInWithBotToken(tgBotToken);

    static bool isBotLoggedIn = false;
    uint64_t startupTime = getCurrentTimeMillis();

    botClient->setMessageHandler([startupTime](ClientSession *session, const tdapi::message *message) {
        const auto *content = message->content_.get();
        static int sendCount = 0;
        if (sendCount > 10) {
            throw std::runtime_error("send too many messages");
        }
        // don't handle old messages
        uint64_t msgTime = message->date_ * 1000;
        if (msgTime < startupTime) {
            LOGI("ignore old message at time: %llu, startup_time: %llu", msgTime, startupTime);
            return true;
        }

        std::cout << "message: type=" << content->get_id() << std::endl;
        if (content->get_id() == tdapi::messageText::ID) {
            std::string text = static_cast<const tdapi::messageText *>(content)->text_->text_;
            std::string reply = "Hello, " + text;
            session->sendTextMessage(message->chat_id_, reply);
            sendCount++;
            return true;
        }
        return false;
    });

    // wait 120s for login success
    for (int i = 0; i < 120; ++i) {
        if (botClient->isAuthorized()) {
            isBotLoggedIn = true;
            break;
        }
        sleep(1);
    }

    if (!isBotLoggedIn) {
        LOGE("bot login failed or timeout");
        return -1;
    }

    // login user
    LOGI("start login user");
    parameters.database_directory_ = exeDir + kPathSeparator + "database" + kPathSeparator + "user_1";

    auto userClient = sessionManager.createSession(parameters);
    botClient->execute(tdapi::make_object<tdapi::getOption>("version"), nullptr);
    userClient->logInWithPhoneNumber(tgUserPhone);

    sessionManager.getExecutors().awaitTermination(-1);
    return 0;
}
