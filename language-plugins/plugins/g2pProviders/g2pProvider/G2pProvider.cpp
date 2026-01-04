#include "G2pProvider.h"

#include <stdcorelib/path.h>

#include <LangPlugins/Api/G2ps/G2p/1/G2pProviderApiL1.h>

namespace LangPlugins
{

    namespace G2p = Api::G2pProvider::L1;

    static std::string formatErrorMessage(const std::string &msgPrefix, const std::vector<std::string> &errorList);

    G2pProvider::G2pProvider() = default;

    G2pProvider::~G2pProvider() = default;

    int G2pProvider::apiLevel() const { return G2p::API_LEVEL; }

    LangMgr::Expected<LangMgr::NO<LangMgr::G2pConfiguration>>
    G2pProvider::createConfiguration(const LangMgr::G2pSpec *spec) const {
        if (!spec) {
            // fatal error: null pointer, return immediately
            return LangMgr::Error{
                LangMgr::Error::InvalidArgument,
                "fatal in createConfiguration: G2pSpec is nullptr",
            };
        }

        const auto &config = spec->manifestConfiguration();
        auto result = LangMgr::NO<G2p::G2pProviderConfiguration>::create();

        // Collect all the errors and return to user
        bool hasErrors = false;
        std::vector<std::string> errorList;

        // [REQUIRED] dict, path (JSON value is string)
        {
            auto collectError = [&](auto &&msg)
            {
                hasErrors = true;
                errorList.emplace_back(std::forward<decltype(msg)>(msg));
            };
            static_assert(std::is_same_v<decltype(result->dict), std::filesystem::path>);
            if (const auto it = config.find("dict"); it != config.end()) {
                if (!it->second.isString()) {
                    collectError(R"(string field "dict" type mismatch)");
                } else {
                    result->dict =
                        stdc::path::clean_path(spec->path() / stdc::path::from_utf8(it->second.toStringView()));
                }
            } else {
                collectError(R"(string field "dict" is missing)");
            }
        } // dict

        if (hasErrors) {
            return LangMgr::Error{
                LangMgr::Error::InvalidFormat,
                formatErrorMessage("error parsing G2p configuration", errorList),
            };
        }
        return result;
    }

    static std::string formatErrorMessage(const std::string &msgPrefix, const std::vector<std::string> &errorList) {
        const std::string middlePart = " (";
        const std::string countSuffix = " errors found):\n";

        size_t totalLength =
            msgPrefix.size() + middlePart.size() + std::to_string(errorList.size()).size() + countSuffix.size();

        for (size_t i = 0; i < errorList.size(); ++i) {
            totalLength += std::to_string(i + 1).size() + 2; // index + ". "
            totalLength += errorList[i].size();
            if (i != errorList.size() - 1) {
                totalLength += 2; // "; "
            }
        }

        std::string result;
        result.reserve(totalLength);

        result.append(msgPrefix);
        result.append(middlePart);
        result.append(std::to_string(errorList.size()));
        result.append(countSuffix);

        for (size_t i = 0; i < errorList.size(); ++i) {
            result.append(std::to_string(i + 1));
            result.append(". ");
            result.append(errorList[i]);
            if (i != errorList.size() - 1) {
                result.append(";\n");
            }
        }

        return result;
    }

} // namespace LangPlugins
