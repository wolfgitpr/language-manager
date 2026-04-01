#include "OnnxSession.h"

#include <stdcorelib/pimpl.h>

#include "internal/Env.h"
#include "internal/Session.h"

namespace LangPlugins::OnnxDriver::V1
{

    class OnnxSession::Impl {
    public:
        Impl() : sessionId(Env::nextId()) {}
        ~Impl() = default;

        int64_t sessionId;
        Session session;
    };

    OnnxSession::OnnxSession() : _impl(std::make_unique<Impl>()) {}

    OnnxSession::~OnnxSession() = default;

    int OnnxSession::apiLevel() const { return 1; }

    LangCore::Expected<void> OnnxSession::initialize() { return {}; }

    LangCore::Expected<void> OnnxSession::open(const std::filesystem::path &path,
                                               const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        const auto openArgs = args.as<LangCore::SessionOpenArgs>();
        if (!openArgs) {
            return LangCore::Error{
                LangCore::Error::ConfigError,
                "session open args is null pointer",
            };
        }
        return impl.session.open(path, openArgs);
    }

    bool OnnxSession::isOpen() const {
        __stdc_impl_t;
        return impl.session.isOpen();
    }

    LangCore::Expected<void> OnnxSession::close() {
        __stdc_impl_t;
        return impl.session.close();
    }

    int64_t OnnxSession::id() const {
        __stdc_impl_t;
        return impl.sessionId;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    OnnxSession::start(const LangCore::NO<LangCore::TaskInput> &input) {
        __stdc_impl_t;
        return impl.session.run(input);
    }

    LangCore::Expected<void> OnnxSession::updateConfig(const std::string &config) {
        // 简单实现：将配置存储到 Task 基类中
        // 具体的配置解析和更新逻辑可以在需要时由插件自行实现
        return setConfig(config);
    }
} // namespace LangPlugins::OnnxDriver::V1
