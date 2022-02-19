//
// Created by kinit on 2022-02-18.
//

#ifndef NEOGROUPCAPTCHABOT_SYNCUTILS_H
#define NEOGROUPCAPTCHABOT_SYNCUTILS_H

#include <functional>

namespace utils {

void async(std::function<void()> func);

class Thread {
public:
    static void sleep(int ms);
};

}

#endif //NEOGROUPCAPTCHABOT_SYNCUTILS_H
