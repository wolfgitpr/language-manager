#ifndef LANGMGR_INFERENCECONTRIB_H
#define LANGMGR_INFERENCECONTRIB_H

#include <LangMgr/Core/Module.h>
#include <LangMgr/Task/Task.h>

namespace LangMgr
{
    class G2pTask;
    class G2pCategory;

    class LANGMGR_EXPORT G2pDefinition : public ModuleDefinition {
    public:
        ~G2pDefinition() override;

    protected:
        class Impl;
        G2pDefinition();

        friend class G2pCategory;
    };

    class InferenceDriver;

    class LANGMGR_EXPORT G2pCategory : public ModuleCategory {
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
