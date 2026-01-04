#include "LstmG2pInterpreter.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "LstmG2pInference.h"

namespace LangPlugins
{
    LstmG2pInterpreter::LstmG2pInterpreter() = default;

    LstmG2pInterpreter::~LstmG2pInterpreter() = default;

    int LstmG2pInterpreter::apiLevel() const { return 1; }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceSchema>>
    LstmG2pInterpreter::createSchema(const LangMgr::InferenceSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createSchema: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Lstm::LstmG2pSchema>::create();

        // Collect all the errors and return to user

        if (const inferUtil::ErrorCollector ec; ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration schema"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceConfiguration>>
    LstmG2pInterpreter::createConfiguration(const LangMgr::InferenceSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangMgr::NO<Lstm::LstmG2pConfiguration>::create();

        // Collect all the errors and return to user
        inferUtil::ErrorCollector ec;
        inferUtil::ConfigurationParser parser(spec, &ec);

        // [REQUIRED] encoder, path (JSON value is string)
        {
            static_assert(std::is_same_v<decltype(result->encoder), std::filesystem::path>);
            parser.parse_path_required(result->encoder, "encoder");
        } // encoder

        // [REQUIRED] predictor, path (JSON value is string)
        {
            static_assert(std::is_same_v<decltype(result->decoder), std::filesystem::path>);
            parser.parse_path_required(result->decoder, "decoder");
        } // predictor

        // chars, load file (JSON value is string of file path)
        {
            static_assert(std::is_same_v<decltype(result->charVocab), std::map<std::string, int>>);
            parser.parse_phonemes(result->charVocab, "charVocab");
        } // chars

        // phonemes, load file (JSON value is string of file path)
        {
            static_assert(std::is_same_v<decltype(result->phonemeVocab), std::map<std::string, int>>);
            parser.parse_phonemes(result->phonemeVocab, "phonemeVocab");
        } // phonemes

        // idx to phoneme mapping
        for (const auto &[phoneme, index] : result->phonemeVocab)
            result->idx_to_phoneme[index] = phoneme;

        if (ec.hasErrors()) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                ec.getErrorMessage("error parsing duration configuration"),
            };
        }
        return result;
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::InferenceImportOptions>>
    LstmG2pInterpreter::createImportOptions(const LangMgr::InferenceSpec *spec,
                                            const LangMgr::JsonValue &options) const {
        return LangMgr::NO<Lstm::LstmG2pImportOptions>::create();
    }

    LangMgr::Expected<LangMgr::NO<LangMgr::Inference>>
    LstmG2pInterpreter::createInference(const LangMgr::InferenceSpec *spec,
                                        const LangMgr::NO<LangMgr::InferenceImportOptions> &importOptions,
                                        const LangMgr::NO<LangMgr::InferenceRuntimeOptions> &runtimeOptions) {
        return LangMgr::NO<LstmG2pInference>::create(spec);
    }

} // namespace LangPlugins
