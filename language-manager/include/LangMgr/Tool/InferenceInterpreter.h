#ifndef LANGMGR_INFERENCEINTERPRETER_H
#define LANGMGR_INFERENCEINTERPRETER_H

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Tool/InferenceContrib.h>

namespace LangMgr {

    class InferenceInterpreter : public NamedObject {
    public:
        /// The highest inference API version currently supported by this interpreter.
        virtual int apiLevel() const = 0;

        /// Called when \c InferenceSpec loads.
        virtual Expected<NO<InferenceSchema>> createSchema(const InferenceSpec *spec) const = 0;

        /// Called when \c InferenceSpec loads.
        virtual Expected<NO<InferenceConfiguration>>
            createConfiguration(const InferenceSpec *spec) const = 0;

        /// Called when \c SingerSpec loads.
        virtual Expected<NO<InferenceImportOptions>>
            createImportOptions(const InferenceSpec *spec, const JsonValue &options) const = 0;

        /// Called when it's about to execute an inference.
        virtual Expected<NO<Inference>>
            createInference(const InferenceSpec *spec,
                            const NO<InferenceImportOptions> &importOptions,
                            const NO<InferenceRuntimeOptions> &runtimeOptions) = 0;
    };

}

#endif // LANGMGR_INFERENCEINTERPRETER_H