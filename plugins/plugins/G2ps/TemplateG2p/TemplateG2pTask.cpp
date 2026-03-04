#include "TemplateG2pTask.h"

#include <mutex>
#include <numeric>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/G2pModule.h>
#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <LangPlugins/Support/PhonemeDict.h>

#include "InferUtil/Verifier.h"


namespace LangPlugins::TemplateG2p
{
    namespace fs = std::filesystem;

    static LangCore::Expected<LangCore::NO<Template::TemplateG2pConfiguration>>
    getConfig(const LangCore::ModuleSpec *spec) {
        const auto genericConfig = spec->as<LangCore::G2pSpec>()->configuration();
        if (!genericConfig)
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateG2p configuration is nullptr");
        if (!(genericConfig->className() == Template::API_CLASS && genericConfig->objectName() == Template::API_NAME))
            return LangCore::Error(LangCore::Error::InvalidArgument, "invalid TemplateG2p configuration");
        return genericConfig.as<Template::TemplateG2pConfiguration>();
    }


    class TemplateG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResult> result;
        LangCore::NO<Task> g2pInference;
        bool enableOnnxG2p;
        std::unique_ptr<InferUtil::Verifier> verifier;
        PhonemeDict phonemeDict;
        mutable std::shared_mutex mutex;
    };

    TemplateG2pTask::TemplateG2pTask(const LangCore::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    TemplateG2pTask::~TemplateG2pTask() = default;

    LangCore::Expected<void> TemplateG2pTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateG2p task init args is nullptr");
        }
        // if (auto name = args->objectName(); name != Template::API_NAME) {
        //     return LangCore::Error(LangCore::Error::InvalidArgument,
        //                           stdc::formatN(R"(invalid TemplateG2p task init args name: expected "%1", got
        //                           "%2")",
        //                                         Template::API_NAME, name));
        // }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get TemplateG2p config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
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

        impl.verifier = std::make_unique<InferUtil::Verifier>(config->verifyEntry);

        // Load phoneme dict
        if (config->enableDict) {
            if (config->dictPath.empty())
                return LangCore::Error(LangCore::Error::FileNotFound,
                                       stdc::formatN("Task '%1' - No dictPath specified", this->spec()->name().text()));
            if (std::error_code ec; !impl.phonemeDict.load(config->dictPath, &ec))
                return LangCore::Error(LangCore::Error::FileNotFound,
                                       stdc::formatN("Task '%1' - Failed to read dictionary %2:%3",
                                                     this->spec()->name().text(), config->dictPath, ec.value()));
        }

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

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    TemplateG2pTask::start(const LangCore::NO<LangCore::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.g2pInference && impl.enableOnnxG2p) {
                setState(Failed);
                return LangCore::Error(LangCore::Error::SessionError, "TemplateG2pTask: g2p inference not initialized");
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

        // if (const auto &name = input->objectName(); name != Template::API_NAME) {
        //     setState(Failed);
        //     return LangCore::Error(
        //         LangCore::Error::InvalidArgument,
        //         stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Template::API_NAME,
        //         name));
        // }

        const auto g2pInput = input.as<LangCore::G2pStartInput>();
        std::vector<LangCore::G2pRes> res;
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangCore::G2pRes{lyric, spec()->name().text(), "", {}, mode, error});

        for (auto &it : res) {
            if (it.mode == "copy") {
                it.pronunciation = it.lyric;
                it.candidates = {it.pronunciation};
            } else if (it.mode == "convert") {
                if (const auto findResult = lookup(it.lyric); findResult.empty()) {
                    const auto lstmInput = LangCore::NO<LangCore::G2pStartInput>::create(
                        LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS, LangCore::G2P_API_LEVEL);
                    lstmInput->g2pInput.push_back({it.lyric});

                    if (!impl.enableOnnxG2p) {
                        it.error = true;
                        continue;
                    }

                    auto resultExp = impl.g2pInference->start(lstmInput);
                    if (!resultExp)
                        return LangCore::Error(LangCore::Error::TaskError,
                                               stdc::formatN(R"(Task "%1" - LstmG2p inference failed: "%2")",
                                                             this->spec()->name().text(), resultExp.error().message()));

                    auto result = resultExp.take();
                    if (const auto g2pResult = result.as<LangCore::G2pResult>()) {
                        it.pronunciation = g2pResult->g2pResult[0].pronunciation;
                    } else {
                        if (!g2pResult->errorMessage.empty()) {
                            return LangCore::Error(LangCore::Error::TaskError,
                                                   stdc::formatN(R"(Task "%1" - Fail: "%2")",
                                                                 this->spec()->name().text(), g2pResult->errorMessage));
                        }
                        it.error = true;
                    }
                } else {
                    std::string pronStr = "";
                    for (auto &phone : findResult)
                        pronStr += phone + " ";
                    it.pronunciation = pronStr;
                }
            } else
                throw std::errc::invalid_argument;
        }

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResult>::create(LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS,
                                                                   LangCore::G2P_API_LEVEL);
        g2pResult->g2pResult = res;

        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangCore::Expected<void> TemplateG2pTask::startAsync(const LangCore::NO<LangCore::TaskStartInput> &input,
                                                         const StartAsyncCallback &callback) {
        // TODO:
        return LangCore::Error(LangCore::Error::NotImplemented);
    }

    bool TemplateG2pTask::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangCore::NO<LangCore::TaskResult> TemplateG2pTask::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins::TemplateG2p
