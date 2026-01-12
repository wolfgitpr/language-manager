#include <../../include/LangMgr/Module/Module.h>
#include <../../include/LangMgr/Package/Package.h>

#include <fstream>
#include <set>

#include <stdcorelib/path.h>
#include <stdcorelib/stlextra/algorithms.h>

#include "Manager_p.h"
#include "Package_p.h"

namespace fs = std::filesystem;

namespace LangMgr
{

    PackageData::~PackageData() {
        for (const auto &[fst, snd] : std::as_const(moduleSpecs)) {
            for (const auto &[fst2, snd2] : snd) {
                delete snd2;
            }
        }
    }

    Expected<void> PackageData::parse(const std::filesystem::path &dir,
                                      const std::map<std::string, ModuleCategory *, std::less<>> &categories,
                                      llvm::SmallVectorImpl<ModuleDefinition *> *outModules) {
        std::string id_;
        stdc::VersionNumber version_;
        stdc::VersionNumber compatVersion_;
        DisplayText vendor_;
        DisplayText copyright_;
        DisplayText description_;
        fs::path readme_;
        std::string url_;
        llvm::SmallVector<ModuleDefinition *> modules_;

        auto canonicalDir = fs::canonical(dir);
        const auto &descPath = canonicalDir / _TSTR("package.json");

        // Read desc
        JsonObject obj;
        if (auto exp = readDesc(descPath); !exp) {
            return exp.error();
        } else {
            obj = exp.take();
        }

        // id
        {
            auto it = obj.find("packageId");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: missing "packageId" field)", descPath),
                };
            }
            id_ = it->second.toString();
            if (!ModuleLocator::isValidLocator(id_)) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: "id" field has invalid value)", descPath),
                };
            }
        }
        // version
        {
            auto it = obj.find("version");
            if (it == obj.end()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: missing "version" field)", descPath),
                };
            }
            version_ = stdc::VersionNumber::fromString(it->second.toString());
            if (version_.isEmpty()) {
                return Error{
                    Error::InvalidFormat,
                    stdc::formatN(R"(%1: invalid version)", descPath),
                };
            }
        }
        // compatVersion
        {
            if (auto it = obj.find("compatVersion"); it != obj.end()) {
                compatVersion_ = stdc::VersionNumber::fromString(it->second.toString());
                if (compatVersion_ > version_) {
                    return Error{
                        Error::InvalidFormat,
                        stdc::formatN(R"(%1: invalid compat version)", descPath),
                    };
                }
            } else {
                compatVersion_ = version_;
            }
        }
        // vendor
        {
            if (auto it = obj.find("vendor"); it != obj.end()) {
                vendor_ = it->second;
            }
        }
        // copyright
        {
            if (auto it = obj.find("copyright"); it != obj.end()) {
                copyright_ = it->second;
            }
        }
        // description
        {
            if (auto it = obj.find("description"); it != obj.end()) {
                description_ = it->second;
            }
        }
        // readme
        {
            if (auto it = obj.find("readme"); it != obj.end()) {
                readme_ = stdc::path::from_utf8(it->second.toString());
            }
        }
        // url
        {
            if (auto it = obj.find("url"); it != obj.end()) {
                url_ = it->second.toString();
            }
        }
        // modules
        {
            auto it = obj.find("modules");
            if (it != obj.end()) {
                if (!it->second.isObject()) {
                    return Error{
                        Error::InvalidFormat,
                        R"("modules" field has invalid value in package manifest)",
                    };
                }
            }

            do {
                Error error1;
                for (const auto &[fst, snd] : it->second.toObject()) {
                    const auto &moduleKey = fst;
                    auto it2 = categories.find(moduleKey);
                    if (it2 == categories.end()) {
                        error1 = {
                            Error::FeatureNotSupported,
                            stdc::formatN(R"(unknown module "%1")", moduleKey),
                        };
                        goto out_failed;
                    }

                    const auto &cc = it2->second;
                    if (!snd.isArray()) {
                        error1 = {
                            Error::InvalidFormat,
                            stdc::formatN(R"(module "%1" field has invalid value in package manifest)", moduleKey),
                        };
                        goto out_failed;
                    }

                    std::set<std::string_view> idSet;
                    for (const auto &item : snd.toArray()) {
                        auto module = cc->parseDefinition(canonicalDir, item);
                        if (!module) {
                            error1 = module.error();
                            goto out_failed;
                        }
                        modules_.push_back(module.get());

                        // Check id
                        const auto &moduleId = module.get()->id();
                        if (idSet.count(moduleId)) {
                            error1 = {
                                Error::InvalidFormat,
                                stdc::formatN(R"(module "%1" object has duplicated id "%2")", fst, moduleId),
                            };
                            goto out_failed;
                        }
                        idSet.emplace(moduleId);
                    }
                }

                break;

            out_failed:
                stdc::delete_all(modules_);
                return error1;
            }
            while (false);
        }

        path = canonicalDir;
        id = std::move(id_);
        version = version_;
        compatVersion = compatVersion_;
        vendor = std::move(vendor_);
        copyright = std::move(copyright_);
        description = std::move(description_);
        readme = std::move(readme_);
        url = std::move(url_);
        *outModules = std::move(modules_);
        return Expected<void>();
    }

    Expected<JsonObject> PackageData::readDesc(const std::filesystem::path &descPath) {
        const std::ifstream file(descPath);
        if (!file.is_open()) {
            return Error{
                Error::FileNotOpen,
                stdc::formatN(R"("%1": failed to open package manifest)", descPath),
            };
        }

        std::stringstream ss;
        ss << file.rdbuf();

        std::string error2;
        const auto root = JsonValue::fromJson(ss.str(), true, &error2);
        if (!error2.empty()) {
            return Error{
                Error::InvalidFormat,
                stdc::formatN(R"("%1": invalid package manifest format: %2)", descPath, error2),
            };
        }
        if (!root.isObject()) {
            return Error{
                Error::InvalidFormat,
                stdc::formatN(R"("%1": invalid package manifest format: not an object)", descPath),
            };
        }
        return root.toObject();
    }

    static PackageData &staticEmptyPackageData() {
        static PackageData empty(nullptr);
        return empty;
    }

    static bool parseDependencyId(std::string_view token, std::string *outId, stdc::VersionNumber *outVersion) {
        if (const size_t openBracket = token.find('['); openBracket != std::string::npos) {
            if (token.back() != ']') {
                return false;
            }
            const auto package = token.substr(0, openBracket);
            if (!ModuleLocator::isValidLocator(package)) {
                return false;
            }
            *outId = package;
            *outVersion =
                stdc::VersionNumber::fromString(token.substr(openBracket + 1, token.size() - openBracket - 1));
            return true;
        }
        return false;
    }

    Package::Package() : _data(&staticEmptyPackageData()) {}

    Package::~Package() = default;

    bool Package::close() {
        if (!_data->mgr) {
            return true;
        }
        if (!static_cast<Manager::Impl *>(_data->mgr->_impl.get())->close(_data)) {
            return false;
        }
        _data = &staticEmptyPackageData();
        return true;
    }

    const std::string &Package::id() const { return _data->id; }

    stdc::VersionNumber Package::version() const { return _data->version; }

    stdc::VersionNumber Package::compatVersion() const { return _data->compatVersion; }

    DisplayText Package::description() const { return _data->description; }

    DisplayText Package::vendor() const { return _data->vendor; }

    DisplayText Package::copyright() const { return _data->copyright; }

    const std::filesystem::path &Package::readme() const { return _data->readme; }

    const std::string &Package::url() const { return _data->url; }

    std::vector<ModuleDefinition *> Package::moduleSpecs(const std::string_view &category) const {
        auto &modules = _data->moduleSpecs;
        const auto it = modules.find(category);
        if (it == modules.end()) {
            return {};
        }

        std::vector<ModuleDefinition *> res;
        const auto &map2 = it->second;
        res.reserve(map2.size());
        for (const auto &[fst, snd] : std::as_const(map2)) {
            res.push_back(snd);
        }
        return res;
    }

    ModuleDefinition *Package::moduleSpec(const std::string_view &category, const std::string_view &id) const {
        auto &modules = _data->moduleSpecs;
        const auto it = modules.find(category);
        if (it == modules.end()) {
            return nullptr;
        }

        const auto &map2 = it->second;
        const auto it2 = map2.find(id);
        if (it2 == map2.end()) {
            return nullptr;
        }
        return it2->second;
    }

    const std::filesystem::path &Package::path() const { return _data->path; }

    Error Package::error() const { return _data->err; }

    bool Package::isLoaded() const { return _data->loaded; }

    Manager *Package::Mgr() const { return _data->mgr; }

    void ScopedPackageRef::forceClose() {
        if (!close()) {
            _data = &staticEmptyPackageData();
        }
    }

} // namespace LangMgr
