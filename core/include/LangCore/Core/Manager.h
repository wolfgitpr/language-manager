#ifndef LANGCORE_MANAGER_H
#define LANGCORE_MANAGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Base/NamedObject.h>
#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>

#include <LangCore/LangCoreGlobal.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    class LANGCORE_EXPORT Manager : public PackageManager {
    public:
        Manager();
        ~Manager() override;

        static Manager *instance();

        bool initialize(std::string &errMsg);
        bool initialized() const;

        Expected<NO<Task>> task(const std::string &category, const std::string &id) const;
        Expected<std::vector<NO<Task>>> tasks(const std::string &category) const;

        std::vector<G2pRes> convert(const std::vector<G2pInput *> &input);

    private:
        Expected<bool> loadTasksForCategory(const std::string &category);

    protected:
        class Impl;
        friend class Package;
        friend class ModuleCategory;
    };

} // namespace LangCore

#endif // LANGCORE_MANAGER_H
