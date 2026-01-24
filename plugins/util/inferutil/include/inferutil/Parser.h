#ifndef LANGPLUGINS_INFERUTIL_PARSER_H
#define LANGPLUGINS_INFERUTIL_PARSER_H

#include <string>
#include <vector>

#include <inferutil/ErrorCollector.h>

#include <LangPlugins/Api/G2ps/TemplateG2p/1/TemplateG2pL1.h>
#include <LangPlugins/Api/Taggers/RegexTagger/1/RegexTaggerL1.h>

#include "LangCore/Module/Module.h"

namespace LangPlugins::inferUtil
{
    enum class ParameterType {
        Variance,
        Transition,
        All,
    };

    class ConfigurationParser {
    public:
        ConfigurationParser(const LangCore::ModuleSpec *spec_, ErrorCollector *ec_) : spec(spec_), ec(ec_) {
            pConfig = &spec->manifestConfiguration();
        }

        inline void parse_bool_optional(bool &out, const std::string &fieldName);
        inline void parse_int_optional(int &out, const std::string &fieldName);
        inline void parse_positive_int_optional(int &out, const std::string &fieldName);
        inline void parse_double_optional(double &out, const std::string &fieldName);
        inline void parse_positive_double_optional(double &out, const std::string &fieldName);
        inline void parse_string_required(std::string &out, const std::string &fieldName);
        inline void parse_path_required(std::filesystem::path &out, const std::string &fieldName);
        inline void parse_phonemes(std::map<std::string, int> &out, const std::string &fieldName);
        inline void parse_verify_required(std::vector<VerifyEntry> &out, const std::string &fieldName);
        inline void parse_tagger_required(std::vector<Api::RegexTagger::L1::TaggerRegexEntry> &out,
                                          const std::string &fieldName);

        template <typename T>
        void collectError(T &&msg) {
            if (ec) {
                ec->collectError(std::forward<T>(msg));
            }
        }

    private:
        bool loadIdMapping(const std::string &fieldName, const std::filesystem::path &path,
                           std::map<std::string, int> &out);

        const LangCore::ModuleSpec *spec;
        ErrorCollector *ec;
        const LangCore::JsonObject *pConfig;
    };
} // namespace LangPlugins::inferUtil

#include "detail/Parser_impl.h"

#endif // LANGPLUGINS_INFERUTIL_PARSER_H
