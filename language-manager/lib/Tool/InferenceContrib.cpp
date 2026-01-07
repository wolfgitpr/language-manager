#include "InferenceContrib.h"

#include <fstream>

#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>
#include <stdcorelib/str.h>

#include "Contribute_p.h"
#include "Inference.h"
#include "InferenceInterpreter.h"
#include "InferenceInterpreterPlugin.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    class InferenceSpec::Impl : public ContribSpec::Impl {
    public:
        Impl() : ContribSpec::Impl("inference") {}

        Expected<void> read(const std::filesystem::path &basePath, const JsonObject &obj) override;

        std::filesystem::path path;

        std::string className;

        DisplayText name;
        int apiLevel = 0;

        JsonObject manifestConfiguration;
        NO<InferenceConfiguration> configuration;

        NO<InferenceInterpreter> interp = nullptr;
    };

    Expected<void> InferenceSpec::Impl::read(const std::filesystem::path &basePath, const JsonObject &obj) {
        fs::path configPath;
        stdc::VersionNumber fmtVersion_;
        std::string id_;
        std::string className_;

        DisplayText name_;
        int apiLevel_;

        JsonObject configuration_;

        // Parse desc
        {
            // id
            auto it = obj.find("id");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "id" field in inference contribute field)",
                };
            }
            id_ = it->second.toString();
            if (!ContribLocator::isValidLocator(id_)) {
                return Error{
                    Error::InvalidFormat,
                    R"("id" field has invalid value in inference contribute field)",
                };
            }

            // class
            it = obj.find("class");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "class" field in inference contribute field)",
                };
            }
            className_ = it->second.toString();
            if (className_.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("class" field has invalid value in inference contribute field)",
                };
            }

            // configuration
            it = obj.find("configuration");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "configuration" field in inference contribute field)",
                };
            }

            std::string configPathString = it->second.toString();
            if (configPathString.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("configuration" field has invalid value in inference contribute field)",
                };
            }

            configPath = stdc::path::from_utf8(configPathString);
            if (auto configPathExtension = stdc::to_lower(configPath.extension().string());
                configPathExtension != ".json") {
                configPath += ".json";
            }
            if (configPath.is_relative()) {
                configPath = basePath / configPath;
            }
        }

        // Read configuration
        JsonObject configObj;
        {
            std::ifstream file(configPath);
            if (!file.is_open()) {
                return Error{
                    Error::FileNotOpen,
                    stdc::formatN(R"(%1: failed to open inference manifest)", configPath),
                };
            }

            std::stringstream ss;
            ss << file.rdbuf();

            std::string error2;
            auto root = JsonValue::fromJson(ss.str(), true, &error2);
            if (!error2.empty()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid inference manifest format: %2)", configPath, error2),
                };
            }
            if (!root.isObject()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid inference manifest format)", configPath),
                };
            }
            configObj = root.toObject();
        }

        // Get attributes
        // $version
        {
            if (auto it = configObj.find("$version"); it == configObj.end()) {
                fmtVersion_ = stdc::VersionNumber(1);
            } else {
                fmtVersion_ = stdc::VersionNumber::fromString(it->second.toString());
                if (fmtVersion_ > stdc::VersionNumber(1)) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(R"(%1: format version "%2" is not supported)", configPath,
                                      fmtVersion_.toString()),
                    };
                }
            }
        }
        // name
        {
            if (auto it = configObj.find("name"); it != configObj.end()) {
                name_ = it->second;
            }
            if (name_.isEmpty()) {
                name_ = id_;
            }
        }
        // level
        {
            auto it = configObj.find("level");
            if (it == configObj.end()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: missing "level" field)", configPath),
                };
            }
            apiLevel_ = it->second.toInt();
            if (apiLevel_ == 0) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: "level" field has invalid value)", configPath),
                };
            }
        }

        // configuration
        {
            if (auto it = configObj.find("configuration"); it != configObj.end()) {
                if (!it->second.isObject()) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(%1: "configuration" field has invalid value)", configPath),
                    };
                }
                configuration_ = it->second.toObject();
            }
        }

        path = fs::canonical(configPath).parent_path();
        fmtVersion = fmtVersion_;
        id = std::move(id_);
        className = std::move(className_);
        name = name_;
        apiLevel = apiLevel_;
        manifestConfiguration = std::move(configuration_);
        return {};
    }

    class InferenceCategory::Impl : public ContribCategory::Impl {
    public:
        explicit Impl(InferenceCategory *decl, LanguageManager *su) : ContribCategory::Impl(decl, "inference", su) {}

        ~Impl() override = default;

        std::map<std::string, NO<InferenceInterpreter>> interpreters;
    };

    InferenceSpec::~InferenceSpec() = default;

    const std::string &InferenceSpec::className() const {
        __stdc_impl_t;
        return impl.className;
    }

    DisplayText InferenceSpec::name() const {
        __stdc_impl_t;
        return impl.name;
    }

    int InferenceSpec::apiLevel() const {
        __stdc_impl_t;
        return impl.apiLevel;
    }

    const JsonObject &InferenceSpec::manifestConfiguration() const {
        __stdc_impl_t;
        return impl.manifestConfiguration;
    }

    NO<InferenceConfiguration> InferenceSpec::configuration() const {
        __stdc_impl_t;
        return impl.configuration;
    }

    const std::filesystem::path &InferenceSpec::path() const {
        __stdc_impl_t;
        return impl.path;
    }

    Expected<NO<Inference>> InferenceSpec::createInference(const NO<InferenceRuntimeOptions> &runtimeOptions) const {
        __stdc_impl_t;
        return impl.interp->createInference(this, runtimeOptions);
    }

    InferenceSpec::InferenceSpec() : ContribSpec(*new Impl()) {}

    InferenceCategory::~InferenceCategory() = default;

    std::vector<InferenceSpec *> InferenceCategory::findInferences(const ContribLocator &identifier) const {
        __stdc_impl_t;
        std::vector<InferenceSpec *> res;
        auto temp = impl.findContributes(identifier);
        res.reserve(res.size());
        for (const auto &item : std::as_const(temp)) {
            res.push_back(static_cast<InferenceSpec *>(item));
        }
        return res;
    }

    std::vector<InferenceSpec *> InferenceCategory::inferences() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx());
        std::vector<InferenceSpec *> res;
        res.reserve(impl.contributes.size());
        for (const auto &item : impl.contributes) {
            res.push_back(static_cast<InferenceSpec *>(item));
        }
        return res;
    }

    std::string InferenceCategory::key() const { return "inferences"; }

    Expected<ContribSpec *> InferenceCategory::parseSpec(const std::filesystem::path &basePath,
                                                         const JsonValue &config) const {
        __stdc_impl_t;
        if (!config.isObject()) {
            return Error{
                Error::InvalidFormat,
                R"(invalid inference specification)",
            };
        }
        auto spec = new InferenceSpec();
        if (const auto exp = spec->_impl->read(basePath, config.toObject()); !exp) {
            delete spec;
            return exp.error();
        }
        return spec;
    }

    Expected<void> InferenceCategory::loadSpec(ContribSpec *spec, const ContribSpec::State state) {
        __stdc_impl_t;
        switch (state) {
        case ContribSpec::Initialized:
            {
                const auto infSpec = static_cast<InferenceSpec *>(spec);
                const auto spec_impl = static_cast<InferenceSpec::Impl *>(infSpec->_impl.get());

                const auto &key = infSpec->className();
                NO<InferenceInterpreter> interp;

                // Search interpreter cache
                if (const auto it = impl.interpreters.find(key); it != impl.interpreters.end()) {
                    interp = it->second;
                } else {
                    // Search interpreter
                    const auto plugin = Mgr()->plugin<InferenceInterpreterPlugin>(infSpec->className().c_str());
                    if (!plugin) {
                        return Error{
                            Error::FeatureNotSupported,
                            stdc::formatN(R"(required interpreter "%1" of inference "%2" not found)",
                                          infSpec->className(), infSpec->id()),
                        };
                    }
                    interp = plugin->create();
                    impl.interpreters[key] = interp;
                }

                // Check api level
                if (interp->apiLevel() < infSpec->apiLevel()) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(
                            R"(required interpreter "%1" of api level %2 doesn't support inference "%3" of api level %4)",
                            infSpec->className(), interp->apiLevel(), infSpec->id(), infSpec->apiLevel()),
                    };
                }

                auto config = interp->createConfiguration(infSpec);
                if (!config) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(failed to parse inference configuration of "%1": %2)", infSpec->id(),
                                      config.error().message()),
                    };
                }
                spec_impl->configuration = config.get();
                spec_impl->interp = interp;
                return ContribCategory::loadSpec(spec, state);
            }

        case ContribSpec::Ready:
        case ContribSpec::Finished:
            return {};

        case ContribSpec::Deleted:
            return ContribCategory::loadSpec(spec, state);

        default:
            break;
        }
        return {};
    }

    InferenceCategory::InferenceCategory(LanguageManager *env) : ContribCategory(*new Impl(this, env)) {}

    static ContribCategoryRegistrar<InferenceCategory> registrar;

} // namespace LangMgr
