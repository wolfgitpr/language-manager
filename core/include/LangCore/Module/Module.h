#ifndef LANGCORE_MODULE_H
#define LANGCORE_MODULE_H

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Base/ObjectPool.h>
#include <LangCore/Support/ContextUtils.h>
#include <LangCore/Support/DisplayText.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>
#include <LangCore/Task/Task.h>
#include <stdcorelib/support/versionnumber.h>

namespace LangCore
{
    class ModuleLocator {
    public:
        ModuleLocator(std::string package, const stdc::VersionNumber version, std::string id) :
            _package(std::move(package)), _version(version), _id(std::move(id)) {}
        ModuleLocator(std::string package, const stdc::VersionNumber version) :
            _package(std::move(package)), _version(version) {}
        ModuleLocator(std::string package, std::string id) : _package(std::move(package)), _id(std::move(id)) {}
        explicit ModuleLocator(std::string id) : _id(std::move(id)) {}

        ModuleLocator() = default;

        const std::string &package() const { return _package; }
        stdc::VersionNumber version() const { return _version; }
        const std::string &id() const { return _id; }
        bool isEmpty() const { return _id.empty(); }

        std::string toString() const;
        static ModuleLocator fromString(const std::string_view &token);
        static bool isValidLocator(const std::string_view &token);

        bool operator==(const ModuleLocator &other) const {
            return _package == other._package && _version == other._version && _id == other._id;
        }

        bool operator!=(const ModuleLocator &other) const { return !(*this == other); }

    private:
        std::string _package;
        stdc::VersionNumber _version;
        std::string _id;
    };

    class PackageData;
    class Package;
    class PackageManager;

    class LANGCORE_EXPORT ModuleSpec {
    public:
        enum State {
            Invalid,
            Initialized,
            Ready,
            Finished,
            Deleted,
        };

        virtual ~ModuleSpec();

        const std::string &id() const;
        const std::string &category() const;
        const std::string &className() const;
        
        /// 获取名称（自动返回当前语言的本地化文本，无感调用）
        std::string name() const;

        int apiLevel() const;

        const JsonObject &manifestConfiguration() const;
        NO<TaskConfiguration> configuration() const;
        const std::filesystem::path &path() const;

        /// 获取配置键的显示名称（自动返回当前语言的本地化文本，无感调用）
        std::string configurationDisplayName(const std::string &configKey) const;

        State state() const;
        Package parent() const;
        PackageManager *Mgr() const;

        /// 返回此模块所属的 ContextKey。
        /// 默认 context 模块返回 ContextKey()（空 context + 空 version）。
        /// 声库私有模块返回 ContextKey(singerId, packageVersion)。
        /// 注：contextKey 在 createModuleTask 阶段注入，parseSpec/loadSpec 阶段为默认值。
        ContextKey contextKey() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit ModuleSpec(Impl &impl);
        explicit ModuleSpec(std::string category);

        friend class ModuleCategory;
        friend class PackageManager;
    };

    template <class T>
    constexpr T *ModuleSpec::as() {
        static_assert(std::is_base_of_v<ModuleSpec, T>, "T must inherit from LangCore::ModuleSpec");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleSpec::as() const {
        static_assert(std::is_base_of_v<ModuleSpec, T>, "T must inherit from LangCore::ModuleSpec");
        return static_cast<const T *>(this);
    }

    class LANGCORE_EXPORT ModuleCategory : public ObjectPool {
    public:
        ~ModuleCategory() override;

        const std::string &name() const;
        PackageManager *Mgr() const;

        std::vector<ModuleSpec *> findSpec(const ModuleLocator &identifier) const;
        std::vector<ModuleSpec *> specs() const;

        template <class T>
        constexpr T *as();

        template <class T>
        constexpr const T *as() const;

    protected:
        virtual std::string key() const = 0;
        virtual std::string category() const = 0;

        Expected<ModuleSpec *> parseSpec(const std::filesystem::path &basePath, const JsonValue &config) const;
        Expected<void> loadSpecBase(ModuleSpec *spec, ModuleSpec::State state);
        Expected<void> loadSpec(ModuleSpec *spec, ModuleSpec::State state);

        std::vector<ModuleSpec *> find(const ModuleLocator &loc) const;

        class Impl;
        explicit ModuleCategory(Impl &impl);
        ModuleCategory(std::string name, PackageManager *mgr);

        friend class PackageManager;
        friend class Package;
        friend class PackageData;
    };

    template <class T>
    constexpr T *ModuleCategory::as() {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangCore::ModuleCategory");
        return static_cast<T *>(this);
    }

    template <class T>
    constexpr const T *ModuleCategory::as() const {
        static_assert(std::is_base_of_v<ModuleCategory, T>, "T must inherit from LangCore::ModuleCategory");
        return static_cast<const T *>(this);
    }

} // namespace LangCore

// ============================================================================
// 简化的模块类别注册宏
// ============================================================================

/// LANGCORE_DECLARE_MODULE_CATEGORY - 在头文件中声明模块类别
/// 使用示例（在 .h 文件中）：
///   namespace LangCore {
///       class MyTask;
///       LANGCORE_DECLARE_MODULE_CATEGORY(My, "my-category")
///   }
#define LANGCORE_DECLARE_MODULE_CATEGORY(ClassName, CategoryKey) \
    class ClassName##Spec : public ModuleSpec { \
    public: \
        ~ClassName##Spec() override; \
    protected: \
        class Impl; \
        ClassName##Spec(); \
        friend class ClassName##Category; \
    }; \
    class ClassName##Category : public ModuleCategory { \
    public: \
        ~ClassName##Category() override; \
    protected: \
        std::string key() const override; \
        std::string category() const override; \
        class Impl; \
        explicit ClassName##Category(PackageManager *env); \
        friend class PackageManager; \
        friend class ModuleCategoryRegistrar<ClassName##Category>; \
    };

/// LANGCORE_DEFINE_MODULE_CATEGORY - 在实现文件中定义模块类别
/// 使用示例（在 .cpp 文件中）：
///   LANGCORE_DEFINE_MODULE_CATEGORY(My, "my-category")
#define LANGCORE_DEFINE_MODULE_CATEGORY(ClassName, CategoryKey) \
    namespace LangCore { \
        class ClassName##Spec::Impl : public ModuleSpec::Impl { \
        public: \
            Impl(const std::string &category) : ModuleSpec::Impl(category) {} \
        }; \
        ClassName##Spec::ClassName##Spec() : ModuleSpec(*new Impl(CategoryKey)) {} \
        ClassName##Spec::~ClassName##Spec() = default; \
        class ClassName##Category::Impl : public ModuleCategory::Impl { \
        public: \
            Impl(ClassName##Category *decl, const std::string &category, PackageManager *mgr) : \
                ModuleCategory::Impl(decl, category, mgr) {} \
            ~Impl() override = default; \
        }; \
        ClassName##Category::ClassName##Category(PackageManager *env) : \
            ModuleCategory(CategoryKey, env) {} \
        ClassName##Category::~ClassName##Category() = default; \
        std::string ClassName##Category::key() const { return CategoryKey; } \
        std::string ClassName##Category::category() const { return CategoryKey; } \
        static ModuleCategoryRegistrar<ClassName##Category> g_##ClassName##Registrar; \
    }

// 注意：使用 LANGCORE_DEFINE_MODULE_CATEGORY 宏的 .cpp 文件需要包含：
// #include "Module_p.h"
// #include "PackageManager_p.h"

/// LANGCORE_REGISTER_MODULE_CATEGORY - 便捷宏，同时在头文件和实现文件中使用
/// 使用示例（在 .h 文件中）：
///   LANGCORE_REGISTER_MODULE_CATEGORY_DECLARE(My, "my-category")
/// 使用示例（在 .cpp 文件中）：
///   LANGCORE_REGISTER_MODULE_CATEGORY_DEFINE(My, "my-category")
#define LANGCORE_REGISTER_MODULE_CATEGORY_DECLARE(ClassName, CategoryKey) \
    LANGCORE_DECLARE_MODULE_CATEGORY(ClassName, CategoryKey)

#define LANGCORE_REGISTER_MODULE_CATEGORY_DEFINE(ClassName, CategoryKey) \
    LANGCORE_DEFINE_MODULE_CATEGORY(ClassName, CategoryKey)

#endif // LANGCORE_MODULE_H
