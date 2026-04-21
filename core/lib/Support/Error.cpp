#include "Error.h"

#include <array>

namespace LangCore
{

    std::shared_ptr<std::string> Error::defaultMessage(const Type type) {
        static const char* const messages[] = {
            "",  // Success
            "config error",  // ConfigError
            "file system error",  // FileSystemError
            "dependency error",  // DependencyError
            "runtime error",  // RuntimeError
            "not implemented error",  // NotImplementedError
            "initialization error",  // InitializationError
            "validation error",  // ValidationError
            "null pointer error",  // NullPointerError
            "index error",  // IndexError
            "timeout error"  // TimeoutError
        };

        static const size_t messageCount = sizeof(messages) / sizeof(messages[0]);
        static std::array<std::shared_ptr<std::string>, messageCount + 1> cached;

        const auto idx = static_cast<int>(type);
        if (idx >= 0 && idx < static_cast<int>(messageCount)) {
            if (!cached[idx]) {
                cached[idx] = std::make_shared<std::string>(messages[idx]);
            }
            return cached[idx];
        }

        // Unknown error type
        if (!cached[messageCount]) {
            cached[messageCount] = std::make_shared<std::string>("unknown error");
        }
        return cached[messageCount];
    }

} // namespace LangCore
