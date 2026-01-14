#ifndef LANGUAGE_MANAGER_H
#define LANGUAGE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <LangMgr/Base/LangCommon.h>
#include <LangMgr/Base/NamedObject.h>
#include <LangMgr/Core/PackageManager.h>
#include <LangMgr/LangMgrGlobal.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
{

    class Package;
    class Task;
    struct TaggerRes;
    class ModuleCategory;

    template <class T>
    class Expected;

    class LANGMGR_EXPORT Manager : public PackageManager {
    public:
        Manager();
        ~Manager() override;

        static Manager *instance();

        bool initialize(std::string &errMsg);
        bool initialized() const;

    public:
        Expected<NO<Task>> tagger(const std::string &id) const;
        std::vector<NO<Task>> taggers() const;

        std::vector<std::string> defaultOrder() const;
        void setDefaultOrder(const std::vector<std::string> &order);

        std::vector<TaggerRes> split(const std::string &input,
                                     const std::vector<std::string> &priorityTaggerIds = {}) const;
        void convert(const std::vector<TaggerRes *> &input) const;

        std::vector<std::string> tag(const std::vector<std::string> &input,
                                     const std::vector<std::string> &priorityTaggerIds = {},
                                     const std::vector<std::string> &reservedTokens = {}) const;

    protected:
        class Impl;
        friend class Package;
        friend class ModuleCategory;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_H
