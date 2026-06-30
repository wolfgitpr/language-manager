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
            "timeout error",  // TimeoutError
            "already initialized error"  // AlreadyInitialized
        };

        static const size_t messageCount = sizeof(messages) / sizeof(messages[0]);

        // §14.21 fix: eagerly initialize all cached strings to avoid data race
        // on concurrent lazy initialization of the static array.
        static const auto cached = []() {
            std::array<std::shared_ptr<std::string>, messageCount + 1> arr;
            for (size_t i = 0; i < messageCount; ++i) {
                arr[i] = std::make_shared<std::string>(messages[i]);
            }
            arr[messageCount] = std::make_shared<std::string>("unknown error");
            return arr;
        }();

        const auto idx = static_cast<int>(type);
        if (idx >= 0 && idx < static_cast<int>(messageCount)) {
            return cached[idx];
        }

        // Unknown error type
        return cached[messageCount];
    }

} // namespace LangCore
