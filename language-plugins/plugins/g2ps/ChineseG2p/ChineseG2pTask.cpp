#include "ChineseG2pTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Task/G2pTask.h>

#include <cpp-pinyin/G2pglobal.h>
#include <cpp-pinyin/Pinyin.h>

#include <inferutil/Verifier.h>

namespace LangPlugins
{
    namespace fs = std::filesystem;

    static LangMgr::Expected<LangMgr::NO<Chinese::ChineseG2pConfiguration>> getConfig(const LangMgr::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangMgr::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "ChineseG2p configuration is nullptr");
        if (!(genericConfig->className() == Chinese::API_CLASS && genericConfig->objectName() == Chinese::API_NAME))
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid ChineseG2p configuration");
        return genericConfig.as<Chinese::ChineseG2pConfiguration>();
    }

    class ChineseG2pTask::Impl {
    public:
        LangMgr::NO<LangMgr::G2pResult> result;
        std::unique_ptr<Pinyin::Pinyin> m_mandarin;
        std::unique_ptr<inferUtil::Verifier> verifier;
        mutable std::shared_mutex mutex;
    };

    ChineseG2pTask::ChineseG2pTask(const LangMgr::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    ChineseG2pTask::~ChineseG2pTask() = default;

    LangMgr::Expected<void> ChineseG2pTask::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "ChineseG2p task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Chinese::API_NAME) {
        //     return LangMgr::Error(LangMgr::Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid ChineseG2p task init args name: expected "%1", got
        //                           "%2")",
        //                                         Chinese::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get ChineseG2p config
        auto expConfig = getConfig(spec()->as<LangMgr::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        impl.verifier = std::make_unique<inferUtil::Verifier>(config->verifyEntry);

        Pinyin::setDictionaryPath(config->dictPath);
        impl.m_mandarin = std::make_unique<Pinyin::Pinyin>();

        if (!impl.m_mandarin->initialized()) {
            setState(Failed);
            return {};
        }

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    static std::vector<std::vector<LangMgr::G2pRes>> groupLyrics(const std::vector<LangMgr::G2pRes> &input) {
        std::vector<std::vector<LangMgr::G2pRes>> groups;
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

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
    ChineseG2pTask::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.m_mandarin->initialized()) {
                setState(Failed);
                return LangMgr::Error(LangMgr::Error::SessionError, "ChineseG2pTask: chinese g2p not initialized");
            }
        }

        setState(Running);

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangMgr::G2pSpec>()); !expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }

        if (!input) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "g2p input is nullptr");
        }

        // if (const auto &name = input->objectName(); name != Chinese::API_NAME) {
        //     setState(Failed);
        //     return LangMgr::Error(
        //         LangMgr::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Chinese::API_NAME,
        //         name));
        // }

        std::vector<LangMgr::G2pRes> res;
        const auto g2pInput = input.as<LangMgr::G2pStartInput>();
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangMgr::G2pRes{lyric, spec()->name().text(), "", {}, mode, error});

        const auto groupLyric = groupLyrics(res);

        // Create result
        auto g2pResult = LangMgr::NO<LangMgr::G2pResult>::create(LangMgr::G2P_API_NAME, LangMgr::G2P_API_CLASS,
                                                                 LangMgr::G2P_API_LEVEL);

        for (const auto &lyrics : groupLyric) {
            std::vector<std::string> _input;
            for (const auto &lyric : lyrics)
                _input.push_back(lyric.lyric);
            auto g2pRes =
                impl.m_mandarin->hanziToPinyin(_input, Pinyin::ManTone::NORMAL, Pinyin::Default, true, false, false);
            for (auto &[hanzi, pinyin, candidates, error] : g2pRes) {
                g2pResult->g2pResult.emplace_back(hanzi, spec()->id(), pinyin, candidates, error ? "copy" : "convert",
                                                  error);
            }
        }


        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangMgr::Expected<void> ChineseG2pTask::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                       const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool ChineseG2pTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangMgr::NO<LangMgr::TaskResult> ChineseG2pTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins
