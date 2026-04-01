#include "Error.h"

namespace LangCore
{

    std::shared_ptr<std::string> Error::defaultMessage(const int type) {
        switch (type) {
        case Success:
            {
                static auto message = std::make_shared<std::string>();
                return message;
            }
        case ConfigError:
            {
                static auto message = std::make_shared<std::string>("config error");
                return message;
            }
        case FileSystemError:
            {
                static auto message = std::make_shared<std::string>("file system error");
                return message;
            }
        case DependencyError:
            {
                static auto message = std::make_shared<std::string>("dependency error");
                return message;
            }
        case RuntimeError:
            {
                static auto message = std::make_shared<std::string>("runtime error");
                return message;
            }
        case NotImplementedError:
            {
                static auto message = std::make_shared<std::string>("not implemented error");
                return message;
            }
        case InitializationError:
            {
                static auto message = std::make_shared<std::string>("initialization error");
                return message;
            }
        default:
            break;
        }
        static auto message = std::make_shared<std::string>("unknown error");
        return message;
    }

} // namespace LangCore
