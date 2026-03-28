#include "TemplateG2pTask.h"

#include <mutex>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangCore/Module/Module.h>
#include <LangCore/Task/G2pTask.h>

#include <LangCore/Support/PhonemeDict.h>

#include <InferUtil/ErrorCollector.h>
#include <InferUtil/Parser.h>
#include <InferUtil/Verifier.h>

namespace LangPlugins::TemplateG2p::V1
{
    class TemplateG2pTask::Impl {
    public:
        LangCore::NO<LangCore::G2pResultV1> result;
        LangCore::NO<Task> g2pInference;
        bool enableOnnxG2p{};
        bool enableDict{};
        std::unique_ptr<InferUtil::Verifier> verifier;
        LangCore::PhonemeDict phonemeDict = {};
        mutable std::shared_mutex mutex;
    };

    TemplateG2pTask::TemplateG2pTask(const LangCore::ModuleSpec *spec) : Task(spec), _impl(std::make_unique<Impl>()) {}

    TemplateG2pTask::~TemplateG2pTask() = default;

    int TemplateG2pTask::apiLevel() const { return 1; }

    LangCore::Expected<void> TemplateG2pTask::initialize() {
        __stdc_impl_t;

        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        InferUtil::ErrorCollector ec;
        InferUtil::ConfigurationParser parser(spec(), &ec);

        std::string onnxG2pId;
        std::filesystem::path dictPath;
        std::vector<InferUtil::VerifyEntry> verifyEntry;

        parser.parse_verify_required(verifyEntry, "verify");
        parser.parse_bool_optional(impl.enableDict, "enableDict");
        parser.parse_path_required(dictPath, "dictPath");
        parser.parse_bool_optional(impl.enableOnnxG2p, "enableOnnxG2p");
        parser.parse_string_required(onnxG2pId, "onnxG2pId");

        if (!impl.enableOnnxG2p) {
            impl.g2pInference = nullptr;
        } else if (auto res = getObject("g2p", onnxG2pId); res) {
            impl.g2pInference = res.take().as<Task>();
        } else {
            return res.takeError();
        }

        auto expVerifier = InferUtil::Verifier::Create(verifyEntry);
        if (!expVerifier)
            return expVerifier.takeError();
        impl.verifier = expVerifier.take();

        // Load phoneme dict
        if (impl.enableDict) {
            if (dictPath.empty())
                return LangCore::Error(LangCore::Error::FileNotFound,
                                       stdc::formatN("Task '%1' - No dictPath specified", this->spec()->name().text()));
            if (std::error_code error_code; !impl.phonemeDict.load(dictPath, &error_code))
                return LangCore::Error(LangCore::Error::FileNotFound,
                                       stdc::formatN("Task '%1' - Failed to read dictionary %2:%3",
                                                     this->spec()->name().text(), dictPath, error_code.value()));
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
    TemplateG2pTask::start(const LangCore::NO<LangCore::TaskInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.g2pInference && impl.enableOnnxG2p)
                return LangCore::Error(LangCore::Error::SessionError, "TemplateG2pTask: g2p inference not initialized");
        }

        if (!input)
            return LangCore::Error(LangCore::Error::InvalidArgument, "g2p input is nullptr");

        const auto g2pInput = input.as<LangCore::G2pInputV1>();
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
                    const auto lstmInput = LangCore::NO<LangCore::G2pInputV1>::create();
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
                    if (const auto g2pResult = result.as<LangCore::G2pResultV1>()) {
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
        auto g2pResult = LangCore::NO<LangCore::G2pResultV1>::create();
        g2pResult->g2pResult = res;

        impl.result = g2pResult;
        return g2pResult;
    }
} // namespace LangPlugins::TemplateG2p::V1
