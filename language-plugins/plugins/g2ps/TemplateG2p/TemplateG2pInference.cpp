#include "TemplateG2pInference.h"

#include <fstream>
#include <mutex>
#include <numeric>
#include <shared_mutex>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include <LangPlugins/Core/Tensor.h>
#include <LangPlugins/Inference/InferenceSession.h>

#include <inferutil/Driver.h>
#include <re2/re2.h>
#include <stdcorelib/console.h>

#include "LangPlugins/Api/Inferences/LstmG2p/1/LstmG2pL1.h"
#include "LangPlugins/Support/PhonemeDict.h"

namespace LangPlugins
{
    namespace fs = std::filesystem;

    static LangMgr::Expected<LangMgr::NO<Template::TemplateG2pConfiguration>>
    getConfig(const LangMgr::InferenceSpec *spec) {
        const auto genericConfig = spec->configuration();
        if (!genericConfig)
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "TemplateG2p configuration is nullptr");
        if (!(genericConfig->className() == Template::API_CLASS && genericConfig->objectName() == Template::API_NAME))
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "invalid TemplateG2p configuration");
        return genericConfig.as<Template::TemplateG2pConfiguration>();
    }

    class VerifyBase {
    public:
        explicit VerifyBase(Api::TemplateG2p::L1::VerifyEntry entry) : entry_(std::move(entry)) {}
        virtual ~VerifyBase() = default;
        virtual void verify(std::vector<Common::G2pRes> &input) {}

    protected:
        Api::TemplateG2p::L1::VerifyEntry entry_;
    };

    class VerifyRegex : public VerifyBase {
    public:
        explicit VerifyRegex(const Api::TemplateG2p::L1::VerifyEntry &entry) : VerifyBase(entry) {
            RegexOptions.set_encoding(RE2::Options::EncodingUTF8);
            RegexOptions.set_log_errors(true);
            RegexOptions.set_max_mem(8 << 20); // 8MB

            regex_ = std::make_unique<RE2>(mergePatterns(entry_.value), RegexOptions);
            if (!regex_->ok())
                throw std::runtime_error("Invalid regex: " + regex_->error());
        }
        ~VerifyRegex() override = default;

        void verify(std::vector<Common::G2pRes> &input) override {
            std::string pattern = regex_->pattern();
            for (auto &it : input) {
                if (!it.error)
                    continue;
                it.mode = entry_.mode;
                it.error = !RE2::FullMatch(it.lyric, *regex_);
            }
        }

    private:
        RE2::Options RegexOptions;
        std::unique_ptr<RE2> regex_;

        static std::string mergePatterns(const std::vector<std::string> &patterns) {
            if (patterns.empty())
                return "";

            std::ostringstream oss;
            oss << "(?:" << patterns[0] << ")";

            for (size_t i = 1; i < patterns.size(); ++i)
                oss << "|(?:" << patterns[i] << ")";

            return oss.str();
        }
    };

    class VerifyArray : public VerifyBase {
    public:
        explicit VerifyArray(const Api::TemplateG2p::L1::VerifyEntry &entry) : VerifyBase(entry) {
            array = std::set<std::string>({entry_.value.begin(), entry_.value.end()});
        }
        ~VerifyArray() override = default;

        void verify(std::vector<Common::G2pRes> &input) override {
            for (auto &it : input) {
                if (!it.error)
                    continue;
                it.mode = entry_.mode == "convert";
                it.error = array.find(it.lyric) == array.end();
            }
        }

    protected:
        std::set<std::string> array;
    };

    class VerifyDict : public VerifyArray {
    public:
        explicit VerifyDict(const Api::TemplateG2p::L1::VerifyEntry &entry) : VerifyArray(entry) {
            array = loadWordsFromTxtFiles({entry_.value.rbegin(), entry_.value.rend()});
        }
        ~VerifyDict() override = default;

    private:
        static std::set<std::string> loadWordsFromTxtFiles(const std::vector<std::string> &paths) {
            std::set<std::string> words;

            for (const auto &path : paths) {
                if (!fs::exists(path)) {
                    std::cerr << "warning: file not exist - " << path << std::endl;
                    continue;
                }

                std::ifstream file(path);
                if (!file.is_open()) {
                    std::cerr << "warning: fail to open file - " << path << std::endl;
                    continue;
                }

                std::string line;
                size_t line_number = 0;

                while (std::getline(file, line)) {
                    line_number++;

                    if (line.empty())
                        continue;

                    if (const size_t tab_pos = line.find('\t'); tab_pos != std::string::npos) {
                        if (std::string word = line.substr(0, tab_pos); !word.empty())
                            words.insert(word);
                    }
                }
                file.close();
                std::cout << "from " << path << " load " << line_number << " lines" << std::endl;
            }
            return words;
        }
    };

    class TemplateG2pInference::Impl {
    public:
        LangMgr::NO<Template::TemplateG2pResult> result;
        LangMgr::NO<Inference> g2pInference;
        std::vector<std::unique_ptr<VerifyBase>> verifiers;
        PhonemeDict phonemeDict;
        mutable std::shared_mutex mutex;
    };

    TemplateG2pInference::TemplateG2pInference(const LangMgr::InferenceSpec *spec) :
        Inference(spec), _impl(std::make_unique<Impl>()) {}

    TemplateG2pInference::~TemplateG2pInference() = default;

    LangMgr::Expected<void> TemplateG2pInference::initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) {
        __stdc_impl_t;
        // Currently, no args to process. But we still need to enforce callers to pass the correct
        // args type.
        if (!args) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "TemplateG2p task init args is nullptr");
        }
        if (auto name = args->objectName(); name != Template::API_NAME) {
            return LangMgr::Error(LangMgr::Error::InvalidArgument,
                                  stdc::formatN(R"(invalid TemplateG2p task init args name: expected "%1", got "%2")",
                                                Template::API_NAME, name));
        }
        std::unique_lock lock(impl.mutex);

        // If there are existing result, they will be cleared.
        impl.result.reset();

        if (auto res = inferUtil::getInferenceObject(this, "lstmG2pInference"); res) {
            impl.g2pInference = res.take().as<Inference>();
        } else {
            setState(Failed);
            return res.takeError();
        }

        // Get TemplateG2p config
        auto expConfig = getConfig(spec());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        for (auto entry : config->verifyEntry) {
            if (entry.type == "regex")
                impl.verifiers.push_back(std::make_unique<VerifyRegex>(entry));
            else if (entry.type == "array")
                impl.verifiers.push_back(std::make_unique<VerifyArray>(entry));
            else if (entry.type == "dict")
                impl.verifiers.push_back(std::make_unique<VerifyDict>(entry));
            else {
                setState(Failed);
                throw std::errc::invalid_argument;
            }
        }

        // Load phoneme dict
        if (std::error_code ec; !impl.phonemeDict.load(config->dictPath, &ec))
            std::cout << "Failed to read dictionary " << config->dictPath << ":" << ec.value();

        // Initialize inference state
        setState(Idle);

        // return success
        return {};
    }

    std::vector<std::string> TemplateG2pInference::lookup(const std::string &key) const {
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
    TemplateG2pInference::start(const LangMgr::NO<LangMgr::TaskStartInput> &input) {
        __stdc_impl_t;
        {
            std::shared_lock lock(impl.mutex);
            if (!impl.g2pInference) {
                setState(Failed);
                return LangMgr::Error(LangMgr::Error::SessionError, "onnx g2p inference not initialized");
            }
        }

        setState(Running);

        // Get configuration
        auto expConfig = getConfig(spec());
        if (!expConfig) {
            setState(Failed);
            return expConfig.takeError();
        }
        const auto config = expConfig.take();

        if (!input) {
            setState(Failed);
            return LangMgr::Error(LangMgr::Error::InvalidArgument, "g2p input is nullptr");
        }

        if (const auto &name = input->objectName(); name != Template::API_NAME) {
            setState(Failed);
            return LangMgr::Error(
                LangMgr::Error::InvalidArgument,
                stdc::formatN(R"(invalid g2p task init args name: expected "%1", got "%2")", Template::API_NAME, name));
        }

        const auto g2pInput = input.as<Template::TemplateG2pStartInput>();
        std::vector<Common::G2pRes> res;
        for (const auto &[lyric, g2pid] : g2pInput->g2pInput)
            res.push_back(Common::G2pRes(lyric, g2pid, "", {}, "copy", true));

        for (const auto &verifier : impl.verifiers)
            verifier->verify(res);

        for (auto &it : res) {
            if (it.mode == "copy") {
                it.pronunciation = it.lyric;
                it.candidates = {it.pronunciation};
            } else if (it.mode == "convert") {
                if (const auto findResult = lookup(it.lyric); findResult.empty()) {
                    const auto lstmInput = LangMgr::NO<Api::LstmG2p::L1::LstmG2pStartInput>::create();
                    lstmInput->words.push_back(Api::LstmG2p::L1::G2pWord{it.lyric});
                    lstmInput->returnDetailedInfo = true;

                    std::cout << "Dict not contains word: " << it.lyric << "; Starting lstmG2p inference..."
                              << std::endl;
                    auto resultExp = impl.g2pInference->start(lstmInput);
                    if (!resultExp)
                        std::cerr << "lstmG2p inference failed: " << resultExp.error().message() << std::endl;

                    auto result = resultExp.take();
                    if (const auto g2pResult = result.as<Api::LstmG2p::L1::LstmG2pResult>()) {
                        const auto phonemes = g2pResult->phonemes;
                        it.pronunciation = std::accumulate(phonemes.begin(), phonemes.end(), std::string(),
                                                           [](const std::string &a, const std::string &b)
                                                           { return a.empty() ? b : a + " " + b; });

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
        auto g2pResult = LangMgr::NO<Template::TemplateG2pResult>::create();
        g2pResult->g2pResult = res;

        impl.result = g2pResult;
        setState(Idle);
        return g2pResult;
    }

    LangMgr::Expected<void> TemplateG2pInference::startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                                             const StartAsyncCallback &callback) {
        // TODO:
        return LangMgr::Error(LangMgr::Error::NotImplemented);
    }

    bool TemplateG2pInference::stop() {
        __stdc_impl_t;
        setState(Terminated);
        return true;
    }

    LangMgr::NO<LangMgr::TaskResult> TemplateG2pInference::result() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.mutex);
        return impl.result;
    }
} // namespace LangPlugins
