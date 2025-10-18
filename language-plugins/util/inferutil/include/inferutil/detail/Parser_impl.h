// DO NOT include this file directly.
// Include <inferutil/Parser.h> instead.

#ifndef LANGPLUGINS_INFERUTIL_PARSER_IMPL_H
#define LANGPLUGINS_INFERUTIL_PARSER_IMPL_H

#ifndef LANGPLUGINS_INFERUTIL_PARSER_IMPL_H
#error "Parser_impl.h should only be included by Parser.h"
#endif

#include <cstddef>
#include <fstream>
#include <utility>

#include <stdcorelib/path.h>
#include <stdcorelib/str.h>
#include <synthrt/Support/JSON.h>

namespace LangPlugins::inferutil
{

    namespace detail
    {
        template <typename Container>
        void insertParamHelper(Container &container, const ParamTag &param) {
            if constexpr (std::is_same_v<Container, std::set<ParamTag>>) {
                container.insert(param);
            } else if constexpr (std::is_same_v<Container, std::vector<ParamTag>>) {
                container.push_back(param);
            } else {
                static_assert(std::is_same_v<Container, void>,
                              "insert_param only supports std::set<ParamTag> "
                              "and std::vector<ParamTag>");
            }
        }

        template <typename Container>
        inline bool tryFindAndInsertParameters(std::string_view key, Container &out) {
            return tryFindAndInsertVarianceParameters(key, out) || tryFindAndInsertTransitionParameters(key, out);
        }
    } // namespace detail

    inline void ConfigurationParser::parse_bool_optional(bool &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (it->second.isBool()) {
                out = it->second.toBool();
            } else {
                collectError("boolean field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }
    inline void ConfigurationParser::parse_int_optional(int &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (it->second.isNumber()) {
                out = static_cast<int>(it->second.toInt());
            } else {
                collectError("integer field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }

    inline void ConfigurationParser::parse_positive_int_optional(int &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (it->second.isNumber()) {
                if (const auto val = static_cast<int>(it->second.toInt()); val > 0) {
                    out = val;
                } else {
                    collectError("integer field \"" + fieldName + "\" must be positive");
                }
            } else {
                collectError("integer field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }
    inline void ConfigurationParser::parse_double_optional(double &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (it->second.isNumber()) {
                out = it->second.toDouble();
            } else {
                collectError("float field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }

    inline void ConfigurationParser::parse_positive_double_optional(double &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (it->second.isNumber()) {
                if (const auto val = it->second.toDouble(); val > 0) {
                    out = val;
                } else {
                    collectError("float field \"" + fieldName + "\" must be positive");
                }
            } else {
                collectError("float field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }

    inline void ConfigurationParser::parse_path_required(std::filesystem::path &out, const std::string &fieldName) {
        const auto &config = *pConfig;

        if (const auto it = config.find(fieldName); it != config.end()) {
            if (!it->second.isString()) {
                collectError("string field \"" + fieldName + "\" type mismatch");
            } else {
                out = stdc::path::clean_path(spec->path() / stdc::path::from_utf8(it->second.toStringView()));
            }
        } else {
            collectError("string field \"" + fieldName + "\" is missing");
        }
    }

    inline void ConfigurationParser::parse_phonemes(std::map<std::string, int> &out) {
        const auto &config = *pConfig;

        if (const auto it = config.find("phonemes"); it != config.end()) {
            if (!it->second.isString()) {
                collectError(R"(string field "phonemes" type mismatch)");
            } else {
                auto path = spec->path() / stdc::path::from_utf8(it->second.toStringView());
                loadIdMapping(it->first, path, out);
            }
        } else {
            collectError("string field \"phonemes\" is missing");
        }
    }

    inline void ConfigurationParser::parse_languages(bool useLanguageId, std::map<std::string, int> &out) {
        const auto &config = *pConfig;

        if (const auto it = config.find("languages"); it != config.end()) {
            if (!it->second.isString()) {
                collectError(R"(string field "languages" type mismatch)");
            } else {
                auto path = spec->path() / stdc::path::from_utf8(it->second.toStringView());
                loadIdMapping(it->first, path, out);
            }
        } else {
            if (useLanguageId) {
                // Missing required `languages` field.
                collectError(R"(string field "languages" is missing)"
                             R"((required when "useLanguageId" is set to true))");
            } else {
                // Nothing to do:
                // `languages` is an optional field when "useLanguageId" is set to false
            }
        }
    }

    inline void ConfigurationParser::parse_hiddenSize(bool useSpeakerEmbedding, int &out) {
        const auto &config = *pConfig;

        if (const auto it = config.find("hiddenSize"); it != config.end()) {
            if (it->second.isNumber()) {
                auto val = static_cast<int>(it->second.toInt());
                if (val <= 0) {
                    collectError(R"(integer field "hiddenSize" must be a positive integer)");
                }
                out = val;
            } else {
                collectError(R"(integer field "hiddenSize" type mismatch)");
            }
        } else {
            if (useSpeakerEmbedding) {
                // Missing required `hiddenSize` field.
                collectError(R"(integer field "hiddenSize" is missing )"
                             R"((required when "useSpeakerEmbedding" is set to true))");
            } else {
                // Nothing to do:
                // `hiddenSize` is an optional field when "useSpeakerEmbedding" is set to false
            }
        }
    }

    inline void ConfigurationParser::parse_frameWidth(double &out) {
        const auto &config = *pConfig;

        if (const auto it = config.find("frameWidth"); it != config.end()) {
            // `frameWidth` found
            if (it->second.isNumber()) {
                out = it->second.toDouble();
            } else {
                collectError(R"(float field "frameWidth" type mismatch)");
            }
        } else {
            // `frameWidth` not found, fall back to `sampleRate` and `hopSize`
            auto it_sampleRate = config.find("sampleRate");
            auto it_hopSize = config.find("hopSize");
            if (it_sampleRate != config.end() && it_hopSize != config.end()) {
                // OK: Fields exist
                if (it_sampleRate->second.isNumber() && it_hopSize->second.isNumber()) {
                    // OK: Is a number
                    auto sampleRate = it_sampleRate->second.toDouble();
                    auto hopSize = it_hopSize->second.toDouble();
                    if (sampleRate > 0 && hopSize > 0) {
                        // OK: Is positive
                        out = 1.0 * hopSize / sampleRate;
                    } else {
                        // Error: Not positive
                        collectError(R"(integer fields "hopSize" and "hopSize" must be positive)");
                    }
                } else {
                    // Error: Not a number
                    collectError(R"(integer fields "hopSize" or "hopSize" type mismatch)");
                }
            } else {
                // Error: `frameWidth` not found, and `sampleRate` `hopSize` also not found
                collectError(R"(must specify either "frameWidth" or ("sampleRate and "hopSize")");
            }
        }
    }

    inline bool ConfigurationParser::loadIdMapping(const std::string &fieldName, const std::filesystem::path &path,
                                                   std::map<std::string, int> &out) {
        std::ifstream file(path);
        if (!file.is_open()) {
            collectError(
                stdc::formatN(R"(error loading "%1": %2 file not found)", fieldName, stdc::path::to_utf8(path)));
            return false;
        }
        file.seekg(0, std::ios::end);
        auto size = file.tellg();
        std::string buffer(size, '\0');
        file.seekg(0);
        file.read(buffer.data(), size);

        std::string errString;
        auto j = srt::JsonValue::fromJson(buffer, true, &errString);
        if (!errString.empty()) {
            if (ec) {
                ec->collectError(std::move(errString));
            }
            return false;
        }

        if (!j.isObject()) {
            collectError(stdc::formatN(R"(error loading "%1": outer JSON is not an object)", fieldName));
            return false;
        }

        const auto &obj = j.toObject();
        bool flag = true;
        for (const auto &[key, value] : obj) {
            if (!value.isInt()) {
                flag = false;
                collectError(stdc::formatN(R"(error loading "%1": value of key "%2" is not int)", fieldName, key));
            } else {
                out[key] = static_cast<int>(value.toInt());
            }
        }
        return flag;
    }

    inline void SchemaParser::parse_bool_optional(bool &out, const std::string &fieldName) {
        const auto &schema = *pSchema;

        if (const auto it = schema.find(fieldName); it != schema.end()) {
            if (it->second.isBool()) {
                out = it->second.toBool();
            } else {
                collectError("boolean field \"" + fieldName + "\" type mismatch");
            }
        } else {
            // Nothing to do
        }
    }

    inline void SchemaParser::parse_string_array_optional(std::vector<std::string> &out, const std::string &fieldName) {
        const auto &schema = *pSchema;

        if (const auto it = schema.find(fieldName); it != schema.end()) {
            if (!it->second.isArray()) {
                collectError("array field \"" + fieldName + "\" type mismatch");
            } else {
                const auto &arr = it->second.toArray();
                out.reserve(arr.size());
                for (const auto &item : arr) {
                    if (!item.isString()) {
                        collectError("array field \"" + fieldName + "\" values type mismatch: string expected");
                    } else {
                        out.emplace_back(item.toString());
                    }
                }
            }
        } else {
            // nothing to do: optional field
        }
    }

    inline void ImportOptionsParser::parse_path_required(std::filesystem::path &out, const std::string &fieldName) {
        const auto &config = *pOptions;
        if (const auto it = config.find(fieldName); it != config.end()) {
            if (!it->second.isString()) {
                collectError("string field \"" + fieldName + "\" type mismatch");
            } else {
                out = stdc::path::clean_path(spec->path() / stdc::path::from_utf8(it->second.toStringView()));
            }
        } else {
            collectError("string field \"" + fieldName + "\" is missing");
        }
    }
} // namespace LangPlugins::inferutil

#endif // LANGPLUGINS_INFERUTIL_PARSER_IMPL_H
