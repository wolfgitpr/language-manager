#include "LstmG2pEngineFactory.h"

#include <stdcorelib/str.h>

#include <inferutil/Parser.h>

#include "LstmG2pTask.h"

namespace LangPlugins::LstmG2p
{
    LstmG2pEngineFactory::LstmG2pEngineFactory() = default;

    LstmG2pEngineFactory::~LstmG2pEngineFactory() = default;

    int LstmG2pEngineFactory::apiLevel() const { return 1; }

    LangCore::Expected<LangCore::NO<LangCore::TaskConfiguration>>
    LstmG2pEngineFactory::createConfiguration(const LangCore::ModuleSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangCore::Error{
                LangCore::Error::InvalidArgument,
                "fatal in createConfiguration: InferenceSpec is nullptr",
            };
        }

        auto result = LangCore::NO<Lstm::LstmG2pConfiguration>::create();

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
            return LangCore::Error{
                LangCore::Error::InvalidFormat,
                ec.getErrorMessage("error parsing LstmG2p configuration"),
            };
        }
        return result;
    }

    LangCore::Expected<LangCore::NO<LangCore::Task>>
    LstmG2pEngineFactory::createTask(const LangCore::ModuleSpec *spec,
                                     const LangCore::NO<LangCore::TaskRuntimeOptions> &runtimeOptions) {
        return LangCore::NO<LstmG2pTask>::create(spec);
    }

} // namespace LangPlugins
