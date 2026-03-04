#ifndef LANGMGR_TAGGERUTIL_H
#define LANGMGR_TAGGERUTIL_H

#include <filesystem>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <re2/re2.h>

#include <LangPlugins/Api/Taggers/TemplateTagger/1/TemplateTaggerL1.h>


namespace LangPlugins::TemplateTagger
{
    class ITaggerUtil {
    public:
        explicit ITaggerUtil(Api::TemplateTagger::L1::TaggerUtilEntry entry, std::string language);
        virtual ~ITaggerUtil();
        virtual void tagger(std::vector<LangCore::TaggerRes> &input) = 0;

    protected:
        std::string m_language;
        Api::TemplateTagger::L1::TaggerUtilEntry m_entry;
    };

    class TaggerRegex : public ITaggerUtil {
    public:
        explicit TaggerRegex(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language);
        ~TaggerRegex() override;
        void tagger(std::vector<LangCore::TaggerRes> &input) override;

    private:
        RE2::Options RegexOptions;
        std::unique_ptr<RE2> regex_;

        static std::string mergePatterns(const std::vector<std::string> &patterns);
    };

    class TaggerArray : public ITaggerUtil {
    public:
        explicit TaggerArray(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language);
        ~TaggerArray() override;
        void tagger(std::vector<LangCore::TaggerRes> &input) override;

    protected:
        std::set<std::string> array;
    };

    class TaggerDict : public TaggerArray {
    public:
        explicit TaggerDict(const Api::TemplateTagger::L1::TaggerUtilEntry &entry, const std::string &language);
        ~TaggerDict() override;

    private:
        static std::set<std::string> loadWordsFromTxtFiles(const std::vector<std::string> &paths);
    };

    class TaggerUtil {
    public:
        explicit TaggerUtil(const std::vector<Api::TemplateTagger::L1::TaggerUtilEntry> &entries, std::string language);
        ~TaggerUtil() = default;

        void tagger(std::vector<LangCore::TaggerRes> &input) const;

    private:
        std::vector<std::unique_ptr<ITaggerUtil>> m_taggerUtils;
    };

} // namespace LangPlugins::TemplateTagger

#endif // LANGMGR_TAGGERUTIL_H
