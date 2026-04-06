#include "G2pStep.h"
#include "Steps/TagAndValidateStep.h"
#include "Steps/DictStep.h"
#include "Steps/ModelStep.h"
#include "Steps/FormatStep.h"
#include "Steps/FallbackStep.h"
#include <LangCore/Support/Error.h>
#include <algorithm>

namespace LangPlugins::ChainG2p
{

    LangCore::Expected<std::shared_ptr<G2pStep>> G2pStepFactory::create(const std::string &stepType)
    {
        std::shared_ptr<G2pStep> step;

        if (stepType == "tagAndValidate") {
            step = std::make_shared<TagAndValidateStep>();
        } else if (stepType == "dict") {
            step = std::make_shared<DictStep>();
        } else if (stepType == "model") {
            step = std::make_shared<ModelStep>();
        } else if (stepType == "format") {
            step = std::make_shared<FormatStep>();
        } else if (stepType == "fallback") {
            step = std::make_shared<FallbackStep>();
        } else {
            return LangCore::Error(LangCore::Error::ConfigError,
                                 "Unknown step type: " + stepType +
                                 ", supported types: " + joinStrings(supportedTypes(), ", "));
        }

        return step;
    }

    std::vector<std::string> G2pStepFactory::supportedTypes()
    {
        return {"tagAndValidate", "dict", "model", "format", "fallback"};
    }

    std::string G2pStepFactory::supportedTypesAsString()
    {
        return joinStrings(supportedTypes(), ", ");
    }

    std::string G2pStepFactory::joinStrings(const std::vector<std::string> &strings, const std::string &delimiter)
    {
        if (strings.empty()) {
            return "";
        }

        std::string result = strings[0];
        for (size_t i = 1; i < strings.size(); ++i) {
            result += delimiter + strings[i];
        }
        return result;
    }

} // namespace LangPlugins::ChainG2p