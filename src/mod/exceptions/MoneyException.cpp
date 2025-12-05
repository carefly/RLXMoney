#include "mod/exceptions/MoneyException.h"
#include <sstream>

namespace rlx_money {

MoneyException::MoneyException(ErrorCode code, const std::string& message) : mCode(code), mMessage(message) {}

const char* MoneyException::what() const noexcept {
    if (mWhatMessage.empty()) {
        mWhatMessage = "[" + errorCodeToString(mCode) + "] " + mMessage;
    }
    return mWhatMessage.c_str();
}

std::string MoneyException::getDetailedMessage() const {
    std::ostringstream oss;
    oss << "错误码: " << static_cast<int>(mCode) << " (" << errorCodeToString(mCode) << ")\n"
        << "错误信息: " << mMessage;
    return oss.str();
}

} // namespace rlx_money