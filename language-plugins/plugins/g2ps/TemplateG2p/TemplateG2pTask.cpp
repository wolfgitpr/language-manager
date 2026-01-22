#include "TemplateG2pTask.h"

#include <iostream>
#include <mutex>
#include <numeric>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangMgr/Module/G2pModule.h>
#include <LangMgr/Module/Module.h>
#include <LangMgr/Task/G2pTask.h>

#include <LangPlugins/Support/PhonemeDict.h>

#include "inferutil/Verifier.h"


namespace LangPlugins
{
    namespace fs = std::filesystem;

    static LangMgr::Expected<LangMgr::NO<Template::TemplateG2pConfiguration>>
    getConfig(const LangMgr::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangMgr::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "TemplateG2p configuration is nullptr");
        if (!(genericConfig->className() == Template::API_CLASS && genericConfig->objectName() == Template::API_NAME))
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid TemplateG2p configuration");
        return genericConfig.as<Template::TemplateG2pConfiguration>();
    }


    class TemplateG2pTask::Impl {
    public:
        LangMgr::NO<LangMgr::G2pResult> result;
        LangMgr::NO<Task> g2pInference;
        bool enableOnnxG2p;
        std::unique_ptr<inferUtil::Verifier> verifier;
        PhonemeDict phonemeDict;
        mutable std::shared_mutex mutex;
    };

    TemplateG2pTask::TemplateG2pTask(const LangMgr::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    TemplateG2pTask::~TemplateG2pTask() = default;

    LangMgr::Expected<void> TemplateG2pTask::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "TemplateG2p task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Template::API_NAME) {
        //     return LangMgr::Error(LangMgr::Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid TemplateG2p task init args name: expected "%1", got
        //                           "%2")",
        //                                         Template::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get TemplateG2p config
        auto expConfig = getConfig(spec()->as<LangMgr::G2pSpec>());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        if (!config->enableOnnxG2p) {
            impl.g2pInference = nullptr;
            impl.enableOnnxG2p = config->enableOnnxG2p;
        } else if (auto res = getObject("g2p", config->onnxG2pId); res) {
            impl.g2pInference = res.take().as<Task>();
        } else {
            setState(Failed);
            return res.takeError();
        }

        impl.verifier = std::make_unique<inferUtil::Verifier>(config->verifyEntry);

        // Load phoneme dict
        if (config->dictPath.empty())
            std::cout << "No dictPath specified" << std::endl;
        else if (std::error_code ec; !impl.phonemeDict.load(config->dictPath, &ec))
            std::cout << "Failed to read dictionary " << config->dictPath << ":" << ec.value();

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    std::vector<std::string> TemplateG2pTask::lookup(const std::string &key) const {
        __stdc_impl_t;
        if (const auto it = impl.phonemeDict.find(key.c_str()); it != impl.phonemeDict.end()) {
            const auto &phonemes = it->second;
            std::vector<std::string> tokens;
            for (const char *buf : phonemes) {
                tokens.push_back(buf);
            }
            return tokens;
        }
        return {};
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
    TemplateG2pTask::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.g2pInference && impl.enableOnnxG2p) {
                setState(Failed);
                return LangMgr::Error(LangMgr::Error::SessionError, "TemplateG2pTask: g2p inference not initialized");
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

        // if (const auto &name = input->objectName(); name != Template::API_NAME) {
        //     setState(Failed);
        //     return LangMgr::Error(
        //         LangMgr::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Template::API_NAME,
        //         name));
        // }

        const auto g2pInput = input.as<LangMgr::G2pStartInput>();
        std::vector<LangMgr::G2pRes> res;
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangMgr::G2pRes{lyric, spec()->name().text(), "", {}, mode, error});

        for (auto &it : res) {
            if (it.mode == "copy") {
                it.pronunciation = it.lyric;
                it.candidates = {it.pronunciation};
            } else if (it.mode == "convert") {
                if (const auto findResult = lookup(it.lyric); findResult.empty()) {
                    const auto lstmInput = LangMgr::NO<LangMgr::G2pStartInput>::create(
                        LangMgr::G2P_API_NAME, LangMgr::G2P_API_CLASS, LangMgr::G2P_API_LEVEL);
                    lstmInput->g2pInput.push_back({it.lyric});

                    if (!impl.enableOnnxG2p) {
                        it.error = true;
                        continue;
                    }

                    std::cout << "Dict not contains word: " << it.lyric << "; Starting lstmG2p inference..."
                              << std::endl;
                    auto resultExp = impl.g2pInference->start(lstmInput);
                    if (!resultExp)
                        std::cerr << "lstmG2p inference failed: " << resultExp.error().message() << std::endl;

                    auto result = resultExp.take();
                    if (const auto g2pResult = result.as<LangMgr::G2pResult>()) {
                        it.pronunciation = g2pResult->g2pResult[0].pronunciation;
                    } else {
                        if (!g2pResult->errorMessage.empty())
                            std::cout << "Error: " << g2pResult->errorMessage << std::endl;
                        std::cerr << "unexpected result type" << std::endl;
                        it.error = true;
                    }
                } else {
                    it.pronunciation = std::accumulate(findResult.begin(), findResult.end(), std::string(),
                                                       [](const std::string &a, const std::string &b)
                                                       { return a.empty() ? b : a + " " + b; });
                }
            } else
                throw std::errc::invalid_argument;
        }

        // Create result
        auto g2pResult = LangMgr::NO<LangMgr::G2pResult>::create(LangMgr::G2P_API_NAME, LangMgr::G2P_API_CLASS,
                                                                 LangMgr::G2P_API_LEVEL);
        g2pResult->g2pResult = res;

        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangMgr::Expected<void> TemplateG2pTask::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                        const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool TemplateG2pTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangMgr::NO<LangMgr::TaskResult> TemplateG2pTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins
