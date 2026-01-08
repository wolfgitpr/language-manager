#ifndef LANGMGR_ENGINEFACTORY_H
#define LANGMGR_ENGINEFACTORY_H

#include <LangMgr/Core/Module.h>
#include <LangMgr/Support/Expected.h>

namespace LangMgr
{

    class EngineFactory : public NamedObject {
    public:
        /// The highest inference API version currently supported by this interpreter.
        virtual int apiLevel() const = 0;

        /// Called when \c InferenceDefinition loads.
        virtual Expected<NO<TaskConfiguration>> createConfiguration(const ModuleDefinition *definition) const = 0;

        /// Called when it's about to execute an inference.
        virtual Expected<NO<Task>> createTask(const ModuleDefinition *definition,
                                              const NO<TaskRuntimeOptions> &runtimeOptions) = 0;
    };

} // namespace LangMgr

#endif // LANGMGR_ENGINEFACTORY_H
