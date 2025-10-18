#include "OnnxSession.h"

#include <stdcorelib/pimpl.h>

#include "internal/Env.h"
#include "internal/Session.h"

namespace LangPlugins {

    class OnnxSession::Impl {
    public:
        Impl() : sessionId(onnxdriver::Env::nextId()) {
        }
        ~Impl() {
        }

        int64_t sessionId;
        onnxdriver::Session session;
    };

    OnnxSession::OnnxSession() : _impl(std::make_unique<Impl>()) {
    }

    OnnxSession::~OnnxSession() {
        __stdc_impl_t;
    }

    LangMgr::Expected<void> OnnxSession::open(const std::filesystem::path &path,
                                          const LangMgr::NO<InferenceSessionOpenArgs> &args) {
        __stdc_impl_t;
        auto openArgs = args.as<Api::Onnx::SessionOpenArgs>();
        if (!openArgs) {
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "session open args is null pointer",
            };
        }
        return impl.session.open(path, openArgs);
    }

    bool OnnxSession::isOpen() const {
        __stdc_impl_t;
        return impl.session.isOpen();
    }

    LangMgr::Expected<void> OnnxSession::close() {
        __stdc_impl_t;
        return impl.session.close();
    }

    int64_t OnnxSession::id() const {
        __stdc_impl_t;
        return impl.sessionId;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>> OnnxSession::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        return impl.session.run(input);
    }

    LangMgr::Expected<void> OnnxSession::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                const StartAsyncCallback &callback) {
        __stdc_impl_t;
        return impl.session.runAsync(input, callback);
    }

    LangMgr::NO<LangMgr::TaskResult> OnnxSession::result() const {
        __stdc_impl_t;
        return impl.session.result();
    }

    bool OnnxSession::stop() {
        __stdc_impl_t;
        impl.session.terminate();
        return true;
    }

}