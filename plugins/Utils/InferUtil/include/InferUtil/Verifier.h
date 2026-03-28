#ifndef LANGMGR_VERIFIER_H
#define LANGMGR_VERIFIER_H

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <LangCore/Support/Expected.h>
#include <re2/re2.h>

namespace LangPlugins::InferUtil
{
    struct VerifyEntry {
        std::string type;
        std::vector<std::string> value;
        std::string mode;
    };

    struct VerifyRes {
        std::string lyric;
        std::string mode = "copy";
        bool error = true;
    };

    class IVerify {
    public:
        explicit IVerify(VerifyEntry entry);
        virtual ~IVerify();

        virtual LangCore::Expected<void> init() = 0;
        virtual void verify(std::vector<VerifyRes> &input) = 0;

    protected:
        VerifyEntry entry_;
    };

    class VerifyRegex : public IVerify {
    public:
        explicit VerifyRegex(const VerifyEntry &entry);
        ~VerifyRegex() override;

        LangCore::Expected<void> init() override;
        void verify(std::vector<VerifyRes> &input) override;

    private:
        RE2::Options regexOptions;
        std::unique_ptr<RE2> regex_;

        static std::string mergePatterns(const std::vector<std::string> &patterns);
    };

    class VerifyArray : public IVerify {
    public:
        explicit VerifyArray(const VerifyEntry &entry);
        ~VerifyArray() override;

        LangCore::Expected<void> init() override { return {}; }
        void verify(std::vector<VerifyRes> &input) override;

    protected:
        std::set<std::string> array;
    };

    class VerifyDict : public VerifyArray {
    public:
        explicit VerifyDict(const VerifyEntry &entry);
        ~VerifyDict() override;

        LangCore::Expected<void> init() override;

    private:
        static LangCore::Expected<std::set<std::string>> loadWordsFromTxtFiles(const std::vector<std::string> &paths);
    };

    class Verifier {
    public:
        static LangCore::Expected<std::unique_ptr<Verifier>> Create(const std::vector<VerifyEntry> &entries);
        ~Verifier() = default;

        std::vector<VerifyRes> verify(const std::vector<std::string> &input) const;

    private:
        Verifier() = default;
        std::vector<std::unique_ptr<IVerify>> verifiers_;
    };

} // namespace LangPlugins::InferUtil

#endif // LANGMGR_VERIFIER_H
