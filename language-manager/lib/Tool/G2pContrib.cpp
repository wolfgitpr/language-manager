#include "G2pContrib.h"

#include <cstdlib>
#include <fstream>

#include <stdcorelib/3rdparty/llvm/smallvector.h>
#include <stdcorelib/path.h>
#include <stdcorelib/pimpl.h>

#include "Contribute_p.h"
#include "G2pProviderPlugin.h"
#include "InferenceContrib.h"
#include "PackageRef.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    static constexpr int kNumG2pImportFields = 5;

    class G2pImportData {
    public:
        ContribLocator inferenceLocator;
        InferenceSpec *inference;
        JsonValue manifestOptions;
        NO<InferenceImportOptions> options;
    };

    class G2pSpec::Impl : public ContribSpec::Impl {
    public:
        Impl() : ContribSpec::Impl("g2p") {}

    public:
        Expected<void> read(const std::filesystem::path &basePath, const JsonObject &obj) override;

        std::filesystem::path path;

        std::string arch;

        DisplayText name;
        int apiLevel = 0;

        std::filesystem::path avatar;
        std::filesystem::path background;
        std::filesystem::path demoAudio;

        llvm::SmallVector<G2pImportData, kNumG2pImportFields> importDataList;
        llvm::SmallVector<G2pImport, kNumG2pImportFields> importList; // wrapper of importDataList

        JsonObject manifestConfiguration;
        NO<G2pConfiguration> configuration;

        NO<G2pProvider> prov = nullptr;
    };

    static bool readG2pImport(const JsonValue &val, G2pImportData *out, std::string *errorMessage) {
        if (val.isString()) {
            auto inference = ContribLocator::fromString(val.toString());
            if (inference.id().empty()) {
                *errorMessage = R"(invalid id)";
                return false;
            }
            G2pImportData res;
            res.inferenceLocator = inference;
            *out = std::move(res);
            return true;
        }
        if (!val.isObject()) {
            *errorMessage = R"(invalid data type)";
            return false;
        }
        auto obj = val.toObject();
        auto it = obj.find("id");
        if (it == obj.end()) {
            *errorMessage = R"(missing "id" field)";
            return false;
        }
        auto inference = ContribLocator::fromString(it->second.toString());
        G2pImportData res;
        res.inferenceLocator = inference;

        // options
        it = obj.find("options");
        if (it != obj.end()) {
            res.manifestOptions = it->second;
        }
        *out = std::move(res);
        return true;
    }

    Expected<void> G2pSpec::Impl::read(const std::filesystem::path &basePath, const JsonObject &obj) {
        fs::path configPath;
        std::string id_;
        std::string arch_;

        DisplayText name_;
        int apiLevel_;

        fs::path avatar_;
        fs::path background_;
        fs::path demoAudio_;

        llvm::SmallVector<G2pImportData, kNumG2pImportFields> imports_;
        JsonObject configuration_;

        // Parse desc
        {
            // id
            auto it = obj.find("id");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "id" field in g2p contribute field)",
                };
            }
            id_ = it->second.toString();
            if (!ContribLocator::isValidLocator(id_)) {
                return Error{
                    Error::InvalidFormat,
                    R"("id" field has invalid value in g2p contribute field)",
                };
            }

            // arch
            it = obj.find("arch");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "arch" field in g2p contribute field)",
                };
            }
            arch_ = it->second.toString();
            if (arch_.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("arch" field has invalid value in g2p contribute field)",
                };
            }

            // path
            it = obj.find("path");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    R"(missing "path" field in g2p contribute field)",
                };
            }

            std::string configPathString = it->second.toString();
            if (configPathString.empty()) {
                return Error{
                    Error::InvalidFormat,
                    R"("path" field has invalid value in g2p contribute field)",
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
                    stdc::formatN(R"(%1: failed to open g2p manifest)", configPath),
                };
            }

            std::stringstream ss;
            ss << file.rdbuf();

            std::string error2;
            auto root = JsonValue::fromJson(ss.str(), true, &error2);
            if (!error2.empty()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid g2p manifest format: %2)", configPath, error2),
                };
            }
            if (!root.isObject()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid g2p manifest format)", configPath),
                };
            }
            configObj = root.toObject();
        }

        // Get attributes
        // $version
        {
            stdc::VersionNumber fmtVersion_;
            if (auto it = configObj.find("$version"); it == configObj.end()) {
                fmtVersion_ = stdc::VersionNumber(1);
            } else {
                fmtVersion_ = stdc::VersionNumber::fromString(it->second.toString());
                if (fmtVersion_ > stdc::VersionNumber(1)) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(R"(%1: format version "%1" is not supported)", fmtVersion_.toString()),
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
        // avatar
        {
            if (auto it = configObj.find("avatar"); it != configObj.end()) {
                avatar_ = stdc::path::from_utf8(it->second.toString());
            }
        }
        // background
        {
            if (auto it = configObj.find("background"); it != configObj.end()) {
                background_ = stdc::path::from_utf8(it->second.toString());
            }
        }
        // demoAudio
        {
            if (auto it = configObj.find("demoAudio"); it != configObj.end()) {
                demoAudio_ = stdc::path::from_utf8(it->second.toString());
            }
        }
        // imports
        {
            if (auto it = configObj.find("imports"); it != configObj.end()) {
                if (!it->second.isArray()) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(%1: "imports" field has invalid value)", configPath),
                    };
                }

                for (const auto &item : it->second.toArray()) {
                    G2pImportData g2pImport;
                    std::string errorMessage;
                    if (!readG2pImport(item, &g2pImport, &errorMessage)) {
                        return Error{
                            Error::InvalidFormat,
                            stdc::formatN(R"(%1: invalid "imports" field entry %2: %3)", configPath,
                                          imports_.size() + 1, errorMessage),
                        };
                    }
                    imports_.push_back(g2pImport);
                }
            }
        }
        // misc
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
        id = std::move(id_);
        arch = std::move(arch_);
        name = std::move(name_);
        apiLevel = apiLevel_;
        avatar = std::move(avatar_);
        background = std::move(background_);
        demoAudio = std::move(demoAudio_);
        importDataList = std::move(imports_);
        manifestConfiguration = std::move(configuration_);
        return Expected<void>();
    }

    static G2pImportData *staticEmptyG2pImportData() {
        static G2pImportData empty;
        return &empty;
    }

    G2pInfoBase::~G2pInfoBase() {}
    G2pImport::G2pImport() : _data(staticEmptyG2pImportData()) {}

    G2pImport::~G2pImport() = default;

    bool G2pImport::isNull() const { return _data == staticEmptyG2pImportData(); }

    const ContribLocator &G2pImport::inferenceLocator() const { return _data->inferenceLocator; }

    InferenceSpec *G2pImport::inference() const { return _data->inference; }

    JsonValue G2pImport::manifestOptions() const { return _data->manifestOptions; }

    NO<InferenceImportOptions> G2pImport::options() const { return _data->options; }

    G2pImport::G2pImport(const G2pImportData *data) : _data(data) {}

    G2pSpec::~G2pSpec() = default;

    const std::string &G2pSpec::arch() const {
        __stdc_impl_t;
        return impl.arch;
    }

    DisplayText G2pSpec::name() const {
        __stdc_impl_t;
        return impl.name;
    }

    int G2pSpec::apiLevel() const {
        __stdc_impl_t;
        return impl.apiLevel;
    }

    const std::filesystem::path &G2pSpec::avatar() const {
        __stdc_impl_t;
        return impl.avatar;
    }

    const std::filesystem::path &G2pSpec::background() const {
        __stdc_impl_t;
        return impl.background;
    }

    const std::filesystem::path &G2pSpec::demoAudio() const {
        __stdc_impl_t;
        return impl.demoAudio;
    }

    stdc::array_view<G2pImport> G2pSpec::imports() const {
        __stdc_impl_t;
        return impl.importList;
    }

    const JsonObject &G2pSpec::manifestConfiguration() const {
        __stdc_impl_t;
        return impl.manifestConfiguration;
    }

    NO<G2pConfiguration> G2pSpec::configuration() const {
        __stdc_impl_t;
        return impl.configuration;
    }

    const std::filesystem::path &G2pSpec::path() const {
        __stdc_impl_t;
        return impl.path;
    }

    G2pSpec::G2pSpec() : ContribSpec(*new Impl()) {}


    class G2pCategory::Impl : public ContribCategory::Impl {
    public:
        explicit Impl(G2pCategory *decl, LanguageManager *mgr) : ContribCategory::Impl(decl, "g2p", mgr) {}

        std::map<std::string, NO<G2pProvider>> providers;
    };

    G2pCategory::~G2pCategory() = default;

    std::vector<G2pSpec *> G2pCategory::findG2pSpecs(const ContribLocator &locator) const {
        __stdc_impl_t;
        std::vector<G2pSpec *> res;
        auto temp = impl.findContributes(locator);
        res.reserve(res.size());
        for (const auto &item : std::as_const(temp)) {
            res.push_back(static_cast<G2pSpec *>(item));
        }
        return res;
    }

    std::vector<G2pSpec *> G2pCategory::g2pSpecs() const {
        __stdc_impl_t;
        std::shared_lock lock(impl.su_mtx());
        std::vector<G2pSpec *> res;
        res.reserve(impl.contributes.size());
        for (const auto &item : impl.contributes) {
            res.push_back(static_cast<G2pSpec *>(item));
        }
        return res;
    }

    std::string G2pCategory::key() const { return "g2ps"; }

    Expected<ContribSpec *> G2pCategory::parseSpec(const std::filesystem::path &basePath,
                                                   const JsonValue &config) const {
        if (!config.isObject()) {
            return Error{
                Error::InvalidFormat,
                R"(invalid inference specification)",
            };
        }
        auto spec = new G2pSpec();
        if (const auto exp = spec->_impl->read(basePath, config.toObject()); !exp) {
            delete spec;
            return exp.error();
        }
        return spec;
    }

    Expected<void> G2pCategory::loadSpec(ContribSpec *spec, const ContribSpec::State state) {
        __stdc_impl_t;
        switch (state) {
        case ContribSpec::Initialized:
            {
                const auto g2pSpec = static_cast<G2pSpec *>(spec);
                const auto spec_impl = static_cast<G2pSpec::Impl *>(g2pSpec->_impl.get());

                const auto &key = g2pSpec->arch();
                NO<G2pProvider> prov;

                // Search provider cache
                if (const auto it = impl.providers.find(key); it != impl.providers.end()) {
                    prov = it->second;
                } else {
                    // Search provider
                    const auto plugin = Mgr()->plugin<G2pProviderPlugin>(g2pSpec->arch().c_str());
                    if (!plugin) {
                        return Error{
                            Error::FeatureNotSupported,
                            stdc::formatN(R"(required arch "%1" of g2p "%2" not found)", g2pSpec->arch(),
                                          g2pSpec->id()),
                        };
                    }
                    prov = plugin->create();
                    impl.providers[key] = prov;
                }

                // Check api level
                if (prov->apiLevel() < g2pSpec->apiLevel()) {
                    return Error{
                        Error::FeatureNotSupported,
                        stdc::formatN(R"(required arch "%1" of api level %2 doesn't support g2p "%3" of api level %4)",
                                      g2pSpec->arch(), prov->apiLevel(), g2pSpec->id(), g2pSpec->apiLevel()),
                    };
                }

                // Create configuration
                auto config = prov->createConfiguration(g2pSpec);
                if (!config) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(failed to parse inference configuration of "%1": %2)", g2pSpec->id(),
                                      config.error().message()),
                    };
                }
                spec_impl->configuration = config.get();
                spec_impl->prov = prov;

                // Fix imports
                for (auto &imp : spec_impl->importDataList) {
                    auto &loc = imp.inferenceLocator;
                    const ContribLocator newLoc(loc.package().empty() ? spec->parent().id() : loc.package(),
                                                loc.version().isEmpty() ? spec->parent().version() : loc.version(),
                                                loc.id());
                    loc = newLoc;
                }
                return ContribCategory::loadSpec(spec, state);
            }

        case ContribSpec::Ready:
            {
                // Check inferences
                const auto spec1 = static_cast<G2pSpec *>(spec);
                const auto spec_impl = static_cast<G2pSpec::Impl *>(spec1->_impl.get());
                const auto inferenceReg = impl.mgr->category("inference")->as<InferenceCategory>();
                auto &importDataList = spec_impl->importDataList;
                for (auto &imp : importDataList) {
                    // Find inference
                    auto inferences = inferenceReg->findInferences(imp.inferenceLocator);
                    if (inferences.empty()) {
                        return Error{
                            Error::FeatureNotSupported,
                            stdc::formatN(R"(required inference "%1" of g2p "%2" not found)",
                                          imp.inferenceLocator.toString(), spec1->id()),
                        };
                    }

                    // Create options
                    const auto inference = inferences.front();
                    auto options = inference->createImportOptions(imp.manifestOptions);
                    if (!options) {
                        return Error{
                            Error::InvalidFormat,
                            stdc::formatN(R"(failed to parse options of inference "%1" imported by g2p "%2": %3)",
                                          imp.inferenceLocator.toString(), spec1->id(), options.error().message()),
                        };
                    }
                    imp.inference = inference;
                    imp.options = options.get();
                }

                llvm::SmallVector<G2pImport, kNumG2pImportFields> imports;
                imports.reserve(importDataList.size());
                for (const auto &imp : std::as_const(importDataList)) {
                    imports.push_back(G2pImport(&imp));
                }
                spec_impl->importList = std::move(imports);
                return Expected<void>();
            }

        case ContribSpec::Finished:
            {
                return Expected<void>();
            }

        case ContribSpec::Deleted:
            {
                return ContribCategory::loadSpec(spec, state);
            }
        default:
            break;
        }
        std::abort();
        return Expected<void>();
    }

    G2pCategory::G2pCategory(LanguageManager *mgr) : ContribCategory(*new Impl(this, mgr)) {}

    static ContribCategoryRegistrar<G2pCategory> registrar;

} // namespace LangMgr
