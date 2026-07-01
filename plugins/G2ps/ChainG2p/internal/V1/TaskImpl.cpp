#include "TaskImpl.h"
#include <mutex>
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Error.h>
#include <LangCore/Support/Logging.h>
#include <LangCore/Task/G2pTask.h>
#include "../Core/G2pStep.h"

namespace LangPlugins::ChainG2p::Internal::V1
{
    ChainG2pTaskImpl::ChainG2pTaskImpl(const LangCore::ModuleSpec *spec)
        : m_spec(spec) {}

    LangCore::Expected<void> ChainG2pTaskImpl::initialize()
    {
        std::unique_lock lock(m_mutex);

        auto cfg = LangCore::config(m_spec);

        // 创建管道，传递 Task 对象
        m_pipeline = std::make_unique<G2pPipeline>(m_spec, m_task);

        // 配置管道
        auto configExp = m_pipeline->configure(cfg.raw());
        if (!configExp) {
            return configExp.takeError();
        }

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    ChainG2pTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input)
    {
        if (!input) {
            return LangCore::Error(LangCore::Error::ConfigError, "g2p input is nullptr");
        }

        const auto g2pInput = input.as<LangCore::G2pInputV1>();

        // 创建上下文
        auto context = std::make_shared<G2pContext>(g2pInput->g2pInput, m_spec);

        // 执行管道
        m_pipeline->process(*context);

        // 构建结果
        auto result = LangCore::NO<LangCore::G2pResultV1>::create();
        result->g2pResult.reserve(context->words().size());

        for (const auto &word : context->words()) {
            LangCore::G2pRes res;
            res.lyric = word.lyric;
            res.g2pId = m_spec->id();
            res.pronunciation = word.pronunciation;
            res.candidates = word.candidates;
            res.mode = word.mode;
            res.errorType = word.errorType;

            // copy 模式下，如果发音为空，应该原样返回
            if (res.mode == "copy" && res.pronunciation.empty()) {
                res.pronunciation = res.lyric;
                if (res.candidates.empty()) {
                    res.candidates = {res.lyric};
                }
            }

            result->g2pResult.push_back(res);
        }

        return result;
    }

    std::string ChainG2pTaskImpl::getConfig() const
    {
        std::shared_lock lock(m_mutex);
        return m_config;
    }

} // namespace LangPlugins::ChainG2p::Internal::V1