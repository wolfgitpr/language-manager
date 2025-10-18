#ifndef LANGMGR_INFERENCE_H
#define LANGMGR_INFERENCE_H

#include <LangMgr/Task/ITask.h>

namespace LangMgr
{

    class InferenceSpec;

    class LanguageManager;

    class InferenceInitArgs : public TaskInitArgs {
    public:
        InferenceInitArgs(std::string name) : TaskInitArgs(std::move(name)) {}

        /// The intermediate output can be stored here in the form of an \c NamedObject for later
        /// use.
        NO<ObjectPool> intermediateObjects;
    };

    class LANGMGR_EXPORT Inference : public ITask {
    public:
        explicit Inference(const InferenceSpec *spec);
        ~Inference();

    public:
        const InferenceSpec *spec() const;
        LanguageManager *SU() const;

    protected:
        class Impl;
    };

} // namespace LangMgr

#endif // LANGMGR_INFERENCE_H
