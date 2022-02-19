//
// Created by kinit on 2022-02-18.
//

#include <thread>
#include "CachedThreadPool.h"

#include "SyncUtils.h"

namespace utils {

static std::unique_ptr<CachedThreadPool> sThreadPool = nullptr;

void async(std::function<void()> func) {
    if (!func) {
        return;
    }
    if (!sThreadPool) {
        sThreadPool = std::make_unique<CachedThreadPool>(4, 16);
    }
    sThreadPool->execute(std::move(func));
}

void Thread::sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

}
