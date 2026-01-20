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

        Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
        Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

        std::vector<std::string> defaultTaggerOrder() const;
        void setDefaultOrder(const std::vector<std::string> &order);
        std::vector<std::string> split(const std::string &input,
                                       const std::vector<std::string> &priorityLanguages = {});

        std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);

        std::vector<TaggerRes> tag(const std::vector<std::string> &input, bool split = false,
                                   const std::vector<std::string> &priorityLanguages = {});

    protected:
        class Impl;
        friend class Package;
        friend class ModuleCategory;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_H
