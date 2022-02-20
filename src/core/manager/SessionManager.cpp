//
// Created by kinit on 2022-02-18.
//

#include "ClientSession.h"
#include "utils/log/Log.h"

#include "SessionManager.h"

namespace td_api = td::td_api;

static constexpr const char *LOG_TAG = "SessionManager";

namespace core {

SessionManager &SessionManager::getInstance() {
    static SessionManager instance;
    return instance;
}

td::ClientManager *SessionManager::getClientManager() {
    std::scoped_lock lock(mMutex);
    if (mClientManager == nullptr) {
        mClientManager = std::make_unique<td::ClientManager>();
    }
    return mClientManager.get();
}

uint64_t SessionManager::nextQueryId() noexcept {
    return mQuerySequence++;
}

uint64_t SessionManager::sendRequestWithClientId(int32_t clientId, td_api::object_ptr<td::td_api::Function> request,
                                                 std::function<void(td_api::object_ptr<td::td_api::Object>)> callback) {
    auto requestId = nextQueryId();
    if (callback) {
        mQueryCallbacks.put(requestId, std::move(callback));
    }
    mLooperCondition.notify_all();
    getClientManager()->send(clientId, requestId, std::move(request));
    return requestId;
}

uint64_t SessionManager::sendRequestWithClientId(int32_t clientId,
                                                 td_api::object_ptr<td::td_api::Function> request, nullptr_t) {
    auto requestId = nextQueryId();
    mLooperCondition.notify_all();
    getClientManager()->send(clientId, requestId, std::move(request));
    return requestId;
}

std::shared_ptr<ClientSession> SessionManager::createSession(const ClientSession::TdLibParameters &parameters) {
    int32_t id = getClientManager()->create_client_id();
    auto sp = std::make_shared<ClientSession>(this, id, parameters);
    mClientSessions.put(id, sp);
    if (mWorkerThread == 0) {
        mLooperRunning = true;
        int rc = pthread_create(&mWorkerThread, nullptr,
                                reinterpret_cast<void *(*)(void *)>(&SessionManager::runLooper), this);
        if (rc != 0) {
            throw std::runtime_error("Failed to create worker thread: error code " + std::to_string(rc));
        }
    }
    return sp;
}

std::shared_ptr<ClientSession> SessionManager::getSession(int32_t tdLibId) const {
    auto session = mClientSessions.get(tdLibId);
    if (session == nullptr) {
        return nullptr;
    }
    return *session;
}

void SessionManager::runLooper(SessionManager *sessionManager) {
    auto *clientManager = sessionManager->getClientManager();
    while (sessionManager->mLooperRunning) {
        auto resp = clientManager->receive(300);
        if (resp.object != nullptr) {
            sessionManager->dispatchResponse(resp);
        }
    }
}

void SessionManager::dispatchResponse(td::ClientManager::Response &response) {
    uint64_t requestId = response.request_id;
    int32_t clientId = response.client_id;
    auto &object = response.object;
    if (object == nullptr) {
        return;
    }
    uint32_t objectType = object->get_id();
    if (requestId != 0) {
        auto pCallback = mQueryCallbacks.get(requestId);
        if (pCallback != nullptr) {
            // LOGD("Dispatching response for request id %ld", requestId);
            auto callback = std::move(*pCallback);
            mQueryCallbacks.remove(requestId);
            callback(std::move(object));
        } else {
            LOGE("No callback for request id %ld", requestId);
        }
    } else {
        // it's an update
        if (!onInterceptUpdate(clientId, object)) {
            auto session = mClientSessions.get(clientId);
            if (session != nullptr) {
                session->get()->handleUpdate(std::move(object));
            } else {
                LOGE("No session for client id %d, objectType = %d", clientId, objectType);
            }
        }
    }
}

void SessionManager::logIfResponseError(const td_api::object_ptr<td_api::Object> &object) {
    if (object == nullptr) {
        return;
    }
    if (object->get_id() == td_api::error::ID) {
        auto error = static_cast<const td_api::error *>(object.get());
        LOGE("Error received: code = %d, message = %s", error->code_, error->message_.c_str());
    }
}

utils::CachedThreadPool &SessionManager::getExecutors() {
    return mThreadPool;
}

SessionManager::~SessionManager() {
    mThreadPool.shutdown();
    mThreadPool.awaitTermination(-1);
}

void SessionManager::terminateSession(int32_t tdLibId) {
    if (tdLibId <= 0) {
        return;
    }
    LOGD("Terminating session with id = %d", tdLibId);
    // TODO: send request to terminate session
}

bool SessionManager::onInterceptUpdate(int32_t clientId, const td_api::object_ptr<td_api::Object> &object) {
    return false;
}

}
