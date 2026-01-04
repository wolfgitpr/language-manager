#ifndef LANGMGR_G2PCONTRIB_H
#define LANGMGR_G2PCONTRIB_H

#include <LangMgr/Core/Contribute.h>
#include <LangMgr/Support/DisplayText.h>
#include <LangMgr/Tool/InferenceContrib.h>

namespace LangMgr
{

    class G2pSpec;
    class G2pCategory;
    class G2pImportData;

    /// G2pInfoBase - The base class storing g2p information.
    class LANGMGR_EXPORT G2pInfoBase : public NamedObject {
    public:
        G2pInfoBase(std::string name, const int apiLevel) : NamedObject(std::move(name)), _apiLevel(apiLevel) {}
        ~G2pInfoBase() override;

        int apiLevel() const { return _apiLevel; }

    protected:
        int _apiLevel;
    };

    class G2pConfiguration : public G2pInfoBase {
    public:
        G2pConfiguration(std::string model, const int apiLevel) : G2pInfoBase(std::move(model), apiLevel) {}
    };

    class LANGMGR_EXPORT G2pImport {
    public:
        G2pImport();
        ~G2pImport();

        bool isNull() const;

        /// The locator of the imported inference.
        const ContribLocator &inferenceLocator() const;

        /// The related \c InferenceSpec instance.
        InferenceSpec *inference() const;

        /// The format of options is determined by the g2p model and inference kind.
        JsonValue manifestOptions() const;

        /// The options of the related inference module.
        NO<InferenceImportOptions> options() const;

    protected:
        explicit G2pImport(const G2pImportData *data);

        const G2pImportData *_data;

        friend class G2pSpec;
        friend class G2pCategory;
    };

    class LANGMGR_EXPORT G2pSpec : public ContribSpec {
    public:
        ~G2pSpec() override;

    public:
        /// Indicates the engine to which the g2p's model library belongs
        const std::string &arch() const;
        DisplayText name() const;
        int apiLevel() const;

        const std::filesystem::path &avatar() const;
        const std::filesystem::path &background() const;
        const std::filesystem::path &demoAudio() const;

        stdc::array_view<G2pImport> imports() const;

        const JsonObject &manifestConfiguration() const;
        NO<G2pConfiguration> configuration() const;

        const std::filesystem::path &path() const;

    protected:
        class Impl;
        G2pSpec();

        friend class G2pCategory;
    };

    class LANGMGR_EXPORT G2pCategory : public ContribCategory {
    public:
        ~G2pCategory() override;

    public:
        std::vector<G2pSpec *> findG2pSpecs(const ContribLocator &locator) const;
        std::vector<G2pSpec *> g2pSpecs() const;

    protected:
        std::string key() const override;
        Expected<ContribSpec *> parseSpec(const std::filesystem::path &basePath,
                                          const JsonValue &config) const override;
        Expected<void> loadSpec(ContribSpec *spec, ContribSpec::State state) override;

    protected:
        class Impl;
        explicit G2pCategory(LanguageManager *mgr);

        friend class SynthUnit;
        friend class ContribCategoryRegistrar<G2pCategory>;
    };

} // namespace LangMgr

#endif // LANGMGR_G2PCONTRIB_H
