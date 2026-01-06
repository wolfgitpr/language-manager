#ifndef LANGUAGE_MANAGER_CONTRIBUTE_H
#define LANGUAGE_MANAGER_CONTRIBUTE_H

#include <filesystem>

#include <stdcorelib/support/versionnumber.h>

#include <LangMgr/Core/LanguageManager.h>
#include <LangMgr/Core/NamedObject.h>
#include <LangMgr/Support/Expected.h>
#include <LangMgr/Support/JSON.h>

namespace LangMgr
{

    /// Package contribution locator.
    ///
    /// Syntax:
    ///  - <package>[<version>]/<contrib>:  e.g. \c foo[1.0]/bar
    ///  - <package>/<id>:                  e.g. \c foo/bar
    ///  - <contrib>:                       e.g. \c bar
    class ContribLocator {
    public:
        ContribLocator(std::string package, stdc::VersionNumber version, std::string id) :
            _package(std::move(package)), _version(std::move(version)), _id(std::move(id)) {}
        ContribLocator(std::string package, stdc::VersionNumber version) :
            _package(std::move(package)), _version(std::move(version)) {}
        ContribLocator(std::string package, std::string id) : _package(std::move(package)), _id(std::move(id)) {}
        ContribLocator(std::string id) : _id(std::move(id)) {}

        ContribLocator() = default;

        /// Returns the package name.
        const std::string &package() const { return _package; }

        /// Returns the package version.
        stdc::VersionNumber version() const { return _version; }

        /// Returns the contribution ID.
        const std::string &id() const { return _id; }

        bool isEmpty() const { return _id.empty(); }

        std::string toString() const;

        /// Parses the contribution locator from the given string.
        static ContribLocator fromString(const std::string_view &token);

        static bool isValidLocator(const std::string_view &token);

        bool operator==(const ContribLocator &other) const {
            return _package == other._package && _version == other._version && _id == other._id;
        }

        bool operator!=(const ContribLocator &other) const { return !(*this == other); }

    protected:
        std::string _package;
        stdc::VersionNumber _version;
        std::string _id;
    };

    class LanguageManager;

    class PackageData;

    class PackageRef;

    class ContribCategory;

    class LANGMGR_EXPORT ContribSpec {
    public:
        enum State {
            Invalid,
            Initialized,
            Ready,
            Finished,
            Deleted,
        };

        virtual ~ContribSpec();

        const std::string &category() const;
        const std::string &id() const;

        /// Load state. Internal use only.
        State state() const;
        /// Related package.
        PackageRef parent() const;
        /// Related \c LanguageManager instance.
        LanguageManager *Mgr() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit ContribSpec(Impl &impl);
        explicit ContribSpec(std::string category);

        friend class ContribCategory;
        friend class LanguageManager;
    };

    template <class T>
    constexpr T *ContribSpec::as() {
        static_assert(std::is_base_of_v<ContribSpec, T>, "T should inherit from LangMgr::ContribSpec");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ContribSpec::as() const {
        static_assert(std::is_base_of_v<ContribSpec, T>, "T should inherit from LangMgr::ContribSpec");
        return static_cast<const T *>(this);
    }

    class ContribCategory : public ObjectPool {
    public:
        ~ContribCategory() override;

        const std::string &name() const;

        /// Returns the related \c LanguageManager instance.
        LanguageManager *Mgr() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        /// Used to identify the sub-specifications of this category within the properties of
        /// \c contributes object in the manifest file.
        virtual std::string key() const = 0;

        /// Parses the contribution specification from the given JSON configuration.
        /// \param basePath The path of the configuration directory.
        /// \param config
        /// \return The uninitialized \c ContribSpec instance.
        virtual Expected<ContribSpec *> parseSpec(const std::filesystem::path &basePath,
                                                  const JsonValue &config) const = 0;

        /// Initializes the \c ContribSpec instance in the given state.
        virtual Expected<void> loadSpec(ContribSpec *spec, ContribSpec::State state);

        std::vector<ContribSpec *> find(const ContribLocator &loc) const;

        class Impl;
        explicit ContribCategory(Impl &impl);
        ContribCategory(std::string name, LanguageManager *su);

        friend class LanguageManager;
        friend class PackageRef;
        friend class PackageData;
    };

    template <class T>
    constexpr T *ContribCategory::as() {
        static_assert(std::is_base_of_v<ContribCategory, T>, "T should inherit from LangMgr::ContribCategory");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ContribCategory::as() const {
        static_assert(std::is_base_of_v<ContribCategory, T>, "T should inherit from LangMgr::ContribCategory");
        return static_cast<const T *>(this);
    }

    template <class T>
    class ContribCategoryRegistrar {
        static_assert(std::is_base_of_v<ContribCategory, T>, "T should inherit from LangMgr::ContribCategory");

    public:
        ContribCategoryRegistrar(ContribCategory *(*fac)(LanguageManager *)) {
            LanguageManager::registerCategoryFactory(fac);
        }

        ContribCategoryRegistrar() {
            LanguageManager::registerCategoryFactory(
                [](LanguageManager *mgr) -> ContribCategory *
                {
                    return new T(mgr); //
                });
        }
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_CONTRIBUTE_H
