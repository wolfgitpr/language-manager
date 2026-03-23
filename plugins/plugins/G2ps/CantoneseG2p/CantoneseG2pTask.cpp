#include "CantoneseG2pTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/G2pModule.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Jyutping.h>

#include <InferUtil/Verifier.h>

namespace LangPlugins::CantoneseG2p
{
    namespace fs = std::filesystem;

    static LangCore::Expected<LangCore::NO<Cantonese::CantoneseG2pConfiguration>>
    getConfig(const LangCore::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangCore::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangCore::Error(LangCore::Error::InvalidArgument, "CantoneseG2p configuration is nullptr");
        if (!(genericConfig->className() == Cantonese::API_CLASS && genericConfig->objectName() == Cantonese::API_NAME))
            return LangCore::Error(LangCore::Error::InvalidArgument, "invalid CantoneseG2p configuration");
        return genericConfig.as<Cantonese::CantoneseG2pConfiguration>();
    }

    class CantoneseG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResult> result;
        std::unique_ptr<Pinyin::Jyutping> m_cantonese;
        std::unique_ptr<InferUtil::Verifier> verifier;
        mutable std::shared_mutex mutex;
    };

    CantoneseG2pTask::CantoneseG2pTask(const LangCore::ModuleSpec *spec) :
        Task(spec), _impl(std::make_unique<Impl>()) {}

    CantoneseG2pTask::~CantoneseG2pTask() = default;

    LangCore::Expected<void> CantoneseG2pTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "CantoneseG2p task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Cantonese::API_NAME) {
        //     return Error(Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid CantoneseG2p task init args name: expected "%1", got
        //                           "%2")",
        //                                         Cantonese::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get CantoneseG2p config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        auto expVerifier = InferUtil::Verifier::Create(config->verifyEntry);
        if (!expVerifier) {
            setState(Failed);
            return expVerifier.takeError();
        }
        impl.verifier = expVerifier.take();

        Pinyin::setDictionaryPath(config->dictPath);
        impl.m_cantonese = std::make_unique<Pinyin::Jyutping>();

        if (!impl.m_cantonese->initialized()) {
            setState(Failed);
            return {};
        }

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    static std::vector<std::vector<LangCore::G2pRes>> groupLyrics(const std::vector<LangCore::G2pRes> &input) {
        std::vector<std::vector<LangCore::G2pRes>> groups;
        std::string lastMode;

        for (const auto &item : input) {
            if (groups.empty() || item.mode != lastMode) {
                groups.emplace_back();
                lastMode = item.mode;
            }
            groups.back().push_back(item);
        }

        return groups;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    CantoneseG2pTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.m_cantonese->initialized()) {
                setState(Failed);
                return LangCore::Error(LangCore::Error::SessionError, "CantoneseG2pTask: chinese g2p not initialized");
            }
        }

        setState(Running);

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>()); !expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }

        if (!input) {
            setState(Failed);
            return LangCore::Error(LangCore::Error::InvalidArgument, "g2p input is nullptr");
        }

        // if (const auto &name = input->objectName(); name != Cantonese::API_NAME) {
        //     setState(Failed);
        //     return Error(
        //         Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Cantonese::API_NAME,
        //         name));
        // }

        std::vector<LangCore::G2pRes> res;
        const auto g2pInput = input.as<LangCore::G2pStartInput>();
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangCore::G2pRes{
                lyric, spec()->name().text(), "", {}, mode, error, error ? LangCore::InvalidLyric : LangCore::NoError});

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResult>::create(LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS,
                                                                   LangCore::G2P_API_LEVEL);

        for (const auto &g2pResGroup : groupLyric) {
            const auto mode = g2pResGroup.front().mode;
            std::vector<std::string> _input;
            for (const auto &g2pRes : g2pResGroup)
                _input.push_back(g2pRes.lyric);

            auto pinyinRes = impl.m_cantonese->hanziToPinyin(_input, Pinyin::CanTone::NORMAL, Pinyin::Default, true);

            for (auto &[hanzi, pinyin, candidates, error] : pinyinRes) {
                g2pResult->g2pResult.emplace_back(hanzi, spec()->id(), mode == "convert" ? pinyin : hanzi, candidates,
                                                  mode, mode == "convert" && error,
                                                  mode == "convert" && error ? LangCore::G2pDepInternalError
                                                                             : LangCore::NoError);
            }
        }


        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangCore::Expected<void> CantoneseG2pTask::startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                                          const StartAsyncCallback &callback) {
        // TODO:
        return LangCore::Error(LangCore::Error::NotImplemented);
    }

    bool CantoneseG2pTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangCore::NO<LangCore::TaskResult> CantoneseG2pTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins::CantoneseG2p
