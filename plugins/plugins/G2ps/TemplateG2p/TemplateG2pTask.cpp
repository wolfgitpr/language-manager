#include "TemplateG2pTask.h"

#include <mutex>
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
        bool enableOnnxG2p{};
        bool enableDict{};
        std::unique_ptr<InferUtil::Verifier> verifier;
        PhonemeDict phonemeDict = {};
        mutable std::shared_mutex mutex;
    };

    TemplateG2pTask::TemplateG2pTask(const LangCore::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    TemplateG2pTask::~TemplateG2pTask() = default;

    LangCore::Expected<void> TemplateG2pTask::initialize(const LangCore::NO<LangCore::TaskInitArgs> &args) {
        __stdc_impl_t;
        if (!args) {
            return LangCore::Error(LangCore::Error::InvalidArgument, "TemplateG2p task init args is nullptr");
        }

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        // Get TemplateG2p config
        auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>());
        if (!expConfig)
            return expConfig.takeError();
        const auto config = expConfig.take();

        impl.enableOnnxG2p = config->enableOnnxG2p;
        impl.enableDict = config->enableDict;
        if (!config->enableOnnxG2p) {
            impl.g2pInference = nullptr;
        } else if (auto res = getObject("g2p", config->onnxG2pId); res) {
            impl.g2pInference = res.take().as<Task>();
        } else {
            return res.takeError();
        }

        auto expVerifier = InferUtil::Verifier::Create(config->verifyEntry);
        if (!expVerifier)
            return expVerifier.takeError();
        impl.verifier = expVerifier.take();

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
        // return success
        return {};
    }

    std::vector<std::string> TemplateG2pTask::lookup(const std::string &key) const {
        __stdc_impl_t;
        if (const auto it = impl.phonemeDict.find(key.c_str()); it != impl.phonemeDict.end()) {
            const auto &phonemes = it->second;
            std::vector<std::string> tokens;
            for (const char *buf : phonemes) {
                tokens.emplace_back(buf);
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
            if (!impl.g2pInference && impl.enableOnnxG2p)
                return LangCore::Error(LangCore::Error::SessionError, "TemplateG2pTask: g2p inference not initialized");
        }

        // Get configuration
        if (auto expConfig = getConfig(spec()->as<LangCore::G2pSpec>()); !expConfig)
            return expConfig.takeError();

        if (!input)
            return LangCore::Error(LangCore::Error::InvalidArgument, "g2p input is nullptr");

        const auto g2pInput = input.as<LangCore::G2pStartInput>();
        std::vector<LangCore::G2pRes> res;
        const auto verifyRes = impl.verifier->verify(g2pInput->g2pInput);
        res.reserve(verifyRes.size());
        for (const auto &[lyric, mode, error] : verifyRes)
            res.emplace_back(LangCore::G2pRes{
                lyric, spec()->name().text(), "", {}, mode, error, error ? LangCore::InvalidLyric : LangCore::NoError});

        for (auto &it : res) {
            if (it.mode == "copy") {
                it.pronunciation = it.lyric;
                it.candidates = {it.pronunciation};
            } else if (it.mode == "convert") {
                if (const auto findResult = lookup(it.lyric); impl.enableDict && !findResult.empty()) {
                    std::string pronStr;
                    for (auto &phone : findResult)
                        pronStr += phone + " ";
                    it.pronunciation = pronStr;
                } else {
                    const auto lstmInput = LangCore::NO<LangCore::G2pStartInput>::create(
                        LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS, LangCore::G2P_API_LEVEL);
                    lstmInput->g2pInput.push_back({it.lyric});

                    if (!impl.enableOnnxG2p) {
                        it.error = true;
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                        it.errorType = LangCore::G2pDepNotEnabled;
                        continue;
                    }

                    if (!impl.g2pInference) {
                        it.error = true;
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                        it.errorType = LangCore::G2pDepInitError;
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
                        it.pronunciation = it.lyric;
                        it.candidates = {it.pronunciation};
                        it.errorType = LangCore::G2pDepRuntimeError;
                    }
                }
            } else
                return LangCore::Error(
                    LangCore::Error::InvalidArgument,
                    stdc::formatN(R"(Task "%1" - Fail: it.mode - "%2")", this->spec()->name().text(), it.mode));
        }

        // Create result
        auto g2pResult = LangCore::NO<LangCore::G2pResult>::create(LangCore::G2P_API_NAME, LangCore::G2P_API_CLASS,
                                                                   LangCore::G2P_API_LEVEL);
        g2pResult->g2pResult = res;

        impl.result = g2pResult;
        return g2pResult;
    }
} // namespace LangPlugins::TemplateG2p
