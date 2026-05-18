#ifndef LANGPLUGINS_CANTONESEG2P_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINS_CANTONESEG2P_INTERNAL_V1_TASKIMPL_H

#include <PinyinG2pTaskImplBase.h>
#include <memory>

#include <cpp-pinyin/Jyutping.h>

namespace LangPlugins::CantoneseG2p::Internal::V1
{
    class CantoneseG2pTaskImpl final : public LangPlugins::Common::PinyinG2pTaskImplBase {
    public:
        explicit CantoneseG2pTaskImpl(const LangCore::ModuleSpec *spec);
        ~CantoneseG2pTaskImpl() override = default;

    protected:
        LangCore::Expected<void> onInitializeEngine() override;
        bool isEngineInitialized() const override;
        std::vector<Pinyin::PinyinRes> doHanziToPinyin(const std::vector<std::string> &input) override;

    private:
        std::unique_ptr<Pinyin::Jyutping> m_engine;
    };

} // namespace LangPlugins::CantoneseG2p::Internal::V1

#endif // LANGPLUGINS_CANTONESEG2P_INTERNAL_V1_TASKIMPL_H