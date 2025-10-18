#ifndef LANGPLUGINS_INFERUTIL_PARSER_H
#define LANGPLUGINS_INFERUTIL_PARSER_H

#include <set>
#include <string>
#include <vector>

#include <LangMgr/Tool/InferenceContrib.h>
#include <LangPlugins/Core/ParamTag.h>
#include <inferutil/ErrorCollector.h>

namespace LangPlugins::inferutil
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
        inline void parse_path_required(std::filesystem::path &out, const std::string &fieldName);

        inline void parse_phonemes(std::map<std::string, int> &out);
        inline void parse_languages(bool useLanguageId, std::map<std::string, int> &out);
        inline void parse_hiddenSize(bool useSpeakerEmbedding, int &out);
        inline void parse_speakers_and_load_emb(bool useSpeakerEmbedding, int hiddenSize,
                                                std::map<std::string, std::vector<float>> &out);

        /// First, try parsing `frameWidth`.
        ///
        /// If not found, try parsing `sampleRate` and `hopSize`,
        /// calculate frameWidth = hopSize / sampleRate
        ///
        /// If all those parameters not found, collect an error.
        inline void parse_frameWidth(double &out);

        template <ParameterType PT>
        inline void parse_parameters(std::set<ParamTag> &out, const std::string &fieldName);

        template <ParameterType PT>
        inline void parse_parameters(std::vector<ParamTag> &out, const std::string &fieldName);

        template <typename T>
        inline void collectError(T &&msg) {
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

        template <ParameterType PT>
        inline void parse_parameters(std::set<ParamTag> &out, const std::string &fieldName);

        template <ParameterType PT>
        inline void parse_parameters(std::vector<ParamTag> &out, const std::string &fieldName);

        template <typename T>
        inline void collectError(T &&msg) {
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
        inline void collectError(T &&msg) {
            if (ec) {
                ec->collectError(std::forward<T>(msg));
            }
        }

    private:
        const LangMgr::InferenceSpec *spec;
        ErrorCollector *ec;
        const LangMgr::JsonObject *pOptions;
    };
} // namespace LangPlugins::inferutil

#include "detail/Parser_impl.h"

#endif // LANGPLUGINS_INFERUTIL_PARSER_H
