#ifndef LANGMGR_INFERENCECONTRIB_H
#define LANGMGR_INFERENCECONTRIB_H

#include <LangMgr/Core/Manager.h>
#include <LangMgr/Module/Module.h>

namespace LangMgr
{
    class G2pTask;
    class G2pCategory;

    class G2pDefinition : public ModuleDefinition {
    public:
        ~G2pDefinition() override;

    protected:
        class Impl;
        G2pDefinition();

        friend class G2pCategory;
    };

    class G2pCategory : public ModuleCategory {
    public:
        ~G2pCategory() override;

    protected:
        std::string key() const override;
        std::string category() const override;

        class Impl;
        explicit G2pCategory(Manager *env);

        friend class Manager;
        friend class ModuleCategoryRegistrar<G2pCategory>;
    };

} // namespace LangMgr

#endif // LANGMGR_INFERENCECONTRIB_H
