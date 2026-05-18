#ifndef LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H

#include <PinyinG2pTaskImplBase.h>
#include <memory>

#include <cpp-pinyin/Pinyin.h>

namespace LangPlugins::MandarinG2p::Internal::V1
{
    class MandarinG2pTaskImpl final : public LangPlugins::Common::PinyinG2pTaskImplBase {
    public:
        explicit MandarinG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~MandarinG2pTaskImpl() override = default;

    protected:
        LangCore::Expected<void> onInitializeEngine() override;
        bool isEngineInitialized() const override;
        std::vector<Pinyin::PinyinRes> doHanziToPinyin(const std::vector<std::string> &input) override;

    private:
        std::unique_ptr<Pinyin::Pinyin> m_engine;
    };

} // namespace LangPlugins::MandarinG2p::Internal::V1

#endif // LANGPLUGINS_MANDARING2P_INTERNAL_V1_TASKIMPL_H