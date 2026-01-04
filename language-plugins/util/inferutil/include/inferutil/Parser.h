#ifndef LANGPLUGINS_INFERUTIL_PARSER_H
#define LANGPLUGINS_INFERUTIL_PARSER_H

#include <string>
#include <vector>

#include <LangMgr/Tool/InferenceContrib.h>
#include <inferutil/ErrorCollector.h>

namespace LangPlugins::inferUtil
{
    enum class ParameterType {
        Variance,
        Transition,
        All,
    };

    class ConfigurationParser {
    public:
        ConfigurationParser(const LangMgr::InferenceSpec *spec_, ErrorCollector *ec_) : spec(spec_), ec(ec_) {
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

        template <typename T>
        void collectError(T &&msg) {
            if (ec) {
                ec->collectError(std::forward<T>(msg));
            }
        }

    private:
        bool loadIdMapping(const std::string &fieldName, const std::filesystem::path &path,
                           std::map<std::string, int> &out);

        const LangMgr::InferenceSpec *spec;
        ErrorCollector *ec;
        const LangMgr::JsonObject *pConfig;
    };

    class SchemaParser {
    public:
        SchemaParser(const LangMgr::InferenceSpec *spec_, ErrorCollector *ec_) : spec(spec_), ec(ec_) {
            pSchema = &spec->manifestSchema();
        }

        inline void parse_bool_optional(bool &out, const std::string &fieldName);
        inline void parse_string_array_optional(std::vector<std::string> &out, const std::string &fieldName);

        template <typename T>
        void collectError(T &&msg) {
            if (ec) {
                ec->collectError(std::forward<T>(msg));
            }
        }

    private:
        bool loadIdMapping(const std::string &fieldName, const std::filesystem::path &path,
                           std::map<std::string, int> &out);

        const LangMgr::InferenceSpec *spec;
        ErrorCollector *ec;
        const LangMgr::JsonObject *pSchema;
    };

    class ImportOptionsParser {
    public:
        ImportOptionsParser(const LangMgr::InferenceSpec *spec_, ErrorCollector *ec_,
                            const LangMgr::JsonObject &options_) : spec(spec_), ec(ec_), pOptions(&options_) {}

        inline void parse_path_required(std::filesystem::path &out, const std::string &fieldName);

        template <typename T>
        void collectError(T &&msg) {
            if (ec) {
                ec->collectError(std::forward<T>(msg));
            }
        }

    private:
        const LangMgr::InferenceSpec *spec;
        ErrorCollector *ec;
        const LangMgr::JsonObject *pOptions;
    };
} // namespace LangPlugins::inferUtil

#include "detail/Parser_impl.h"

#endif // LANGPLUGINS_INFERUTIL_PARSER_H
