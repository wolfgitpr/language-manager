#ifndef LANGMGR_INFERENCECONTRIB_H
#define LANGMGR_INFERENCECONTRIB_H

#include <LangMgr/Core/Contribute.h>
#include <LangMgr/Support/DisplayText.h>

namespace LangMgr
{
    /// InferenceInfoBase - The base class storing inference information which should be created
    /// by a specific inference interpreter.
    class InferenceInfoBase : public NamedObject {
    public:
        InferenceInfoBase(std::string name, std::string className, const int apiLevel) :
            NamedObject(std::move(name)), _className(std::move(className)), _apiLevel(apiLevel) {}
        ~InferenceInfoBase() override = default;

        /// Related interpreter information.
        const std::string &className() const { return _className; }
        int apiLevel() const { return _apiLevel; }

    protected:
        std::string _className;
        int _apiLevel;
    };

    class InferenceConfiguration : public InferenceInfoBase {
    public:
        InferenceConfiguration(std::string name, std::string iid, const int apiLevel) :
            InferenceInfoBase(std::move(name), std::move(iid), apiLevel) {}
    };

    class InferenceRuntimeOptions : public InferenceInfoBase {
    public:
        InferenceRuntimeOptions(std::string name, std::string iid, const int apiLevel) :
            InferenceInfoBase(std::move(name), std::move(iid), apiLevel) {}
    };

    class Inference;

    class InferenceCategory;

    class LANGMGR_EXPORT InferenceSpec : public ContribSpec {
    public:
        ~InferenceSpec() override;

        const std::string &className() const;
        DisplayText name() const;
        int apiLevel() const;

        const JsonObject &manifestConfiguration() const;
        NO<InferenceConfiguration> configuration() const;

        const std::filesystem::path &path() const;

        /// Creates an inference interface with the given options.
        Expected<NO<Inference>> createInference(const NO<InferenceRuntimeOptions> &runtimeOptions) const;

    protected:
        class Impl;
        InferenceSpec();

        friend class InferenceCategory;
    };

    class InferenceDriver;

    class LANGMGR_EXPORT InferenceCategory : public ContribCategory {
    public:
        ~InferenceCategory() override;

        std::vector<InferenceSpec *> findInferences(const ContribLocator &identifier) const;
        std::vector<InferenceSpec *> inferences() const;

    protected:
        std::string key() const override;
        Expected<ContribSpec *> parseSpec(const std::filesystem::path &basePath,
                                          const JsonValue &config) const override;
        Expected<void> loadSpec(ContribSpec *spec, ContribSpec::State state) override;

        class Impl;
        explicit InferenceCategory(LanguageManager *env);

        friend class LanguageManager;
        friend class ContribCategoryRegistrar<InferenceCategory>;
    };

} // namespace LangMgr

#endif // LANGMGR_INFERENCECONTRIB_H
