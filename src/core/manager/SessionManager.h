//
// Created by kinit on 2022-02-18.
//

#ifndef NEOGROUPCAPTCHABOT_SESSIONMANAGER_H
#define NEOGROUPCAPTCHABOT_SESSIONMANAGER_H

#include <string>
#include <mutex>
#include <memory>
#include <atomic>
#include <cstdint>
#include <functional>
#include <pthread.h>
#include <condition_variable>

#include <td/telegram/Client.h>
#include <td/telegram/td_api.h>

#include "utils/ConcurrentHashMap.h"
#include "utils/CachedThreadPool.h"
#include "ClientSession.h"

namespace core {

class SessionManager {

private:
    SessionManager() = default;

public:
    ~SessionManager();

    SessionManager(const SessionManager &) = delete;

    SessionManager &operator=(const SessionManager &) = delete;

    td::ClientManager *getClientManager();

    uint64_t nextQueryId() noexcept;

    uint64_t sendRequestWithClientId(int32_t clientId, td::td_api::object_ptr<td::td_api::Function> request,
                                     std::function<void(td::td_api::object_ptr<td::td_api::Object>)> callback);

    uint64_t sendRequestWithClientId(int32_t clientId, td::td_api::object_ptr<td::td_api::Function> request, nullptr_t);

    [[nodiscard]] std::shared_ptr<ClientSession> createSession(const ClientSession::TdLibParameters &parameters);

    [[nodiscard]] std::shared_ptr<ClientSession> getSession(int32_t tdLibId) const;

    void terminateSession(int32_t tdLibId);

    [[nodiscard]] utils::CachedThreadPool &getExecutors();

    static SessionManager &getInstance();

    static void runLooper(SessionManager *sessionManager);

    void dispatchResponse(td::ClientManager::Response &response);

    static void logIfResponseError(const td::td_api::object_ptr<td::td_api::Object> &object);

private:
    bool onInterceptUpdate(int32_t clientId, const td::td_api::object_ptr<td::td_api::Object> &object);

private:
    std::mutex mMutex;
    std::unique_ptr<td::ClientManager> mClientManager;
    std::atomic_uint64_t mQuerySequence = 1;
    ConcurrentHashMap<uint64_t, std::function<void(td::td_api::object_ptr<td::td_api::Object>)>> mQueryCallbacks;
    ConcurrentHashMap<int32_t, std::shared_ptr<ClientSession>> mClientSessions;
    pthread_t mWorkerThread = 0;
    std::mutex mLooperMutex;
    std::condition_variable mLooperCondition;
    std::atomic_bool mLooperRunning = false;
    utils::CachedThreadPool mThreadPool = utils::CachedThreadPool(4, 16);
};

}

#endif //NEOGROUPCAPTCHABOT_SESSIONMANAGER_H
