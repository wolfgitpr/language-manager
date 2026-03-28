#ifndef LANGMGR_TAGGERUTIL_H
#define LANGMGR_TAGGERUTIL_H

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <re2/re2.h>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Support/Expected.h>

namespace LangPlugins::TemplateTagger
{
    struct TaggerUtilEntry {
        std::string type;
        std::vector<std::string> value;
        std::string tag;
        bool discard = false;
    };

    class ITaggerUtil {
    public:
        explicit ITaggerUtil(TaggerUtilEntry entry, std::string language);
        virtual ~ITaggerUtil();

        virtual LangCore::Expected<void> init() = 0;
        virtual void tagger(std::vector<LangCore::TaggerRes> &input) = 0;

    protected:
        std::string m_language;
        TaggerUtilEntry m_entry;
    };

    class TaggerRegex : public ITaggerUtil {
    public:
        explicit TaggerRegex(const TaggerUtilEntry &entry, const std::string &language);
        ~TaggerRegex() override;

        LangCore::Expected<void> init() override;
        void tagger(std::vector<LangCore::TaggerRes> &input) override;

    private:
        RE2::Options RegexOptions;
        std::unique_ptr<RE2> regex_;

        static std::string mergePatterns(const std::vector<std::string> &patterns);
    };

    class TaggerArray : public ITaggerUtil {
    public:
        explicit TaggerArray(const TaggerUtilEntry &entry, const std::string &language);
        ~TaggerArray() override;

        LangCore::Expected<void> init() override;
        void tagger(std::vector<LangCore::TaggerRes> &input) override;

    protected:
        std::set<std::string> array;
    };

    class TaggerDict : public TaggerArray {
    public:
        explicit TaggerDict(const TaggerUtilEntry &entry, const std::string &language);
        ~TaggerDict() override;

        LangCore::Expected<void> init() override;

    private:
        static LangCore::Expected<std::set<std::string>> loadWordsFromTxtFiles(const std::vector<std::string> &paths);
    };

    class TaggerUtil {
    public:
        static LangCore::Expected<std::unique_ptr<TaggerUtil>> Create(const std::vector<TaggerUtilEntry> &entries,
                                                                      const std::string &language);
        ~TaggerUtil() = default;

        void tagger(std::vector<LangCore::TaggerRes> &input) const;

    private:
        TaggerUtil() = default;
        std::vector<std::unique_ptr<ITaggerUtil>> m_taggerUtils;
    };

} // namespace LangPlugins::TemplateTagger

#endif // LANGMGR_TAGGERUTIL_H
