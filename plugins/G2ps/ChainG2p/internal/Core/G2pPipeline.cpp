#include "G2pPipeline.h"
#include <LangCore/Support/Error.h>

namespace LangPlugins::ChainG2p
{
    LangCore::Expected<void> G2pPipeline::configure(const LangCore::JsonObject &config)
    {
        // 检查是否是责任链格式（有 steps 字段）
        auto stepsIt = config.find("steps");
        if (stepsIt != config.end() && stepsIt->second.isArray()) {
            // 责任链格式
            const auto &stepsArray = stepsIt->second.toArray();
            m_steps.reserve(stepsArray.size());

            for (const auto &stepItem : stepsArray) {
                if (!stepItem.isObject()) {
                    return LangCore::Error(LangCore::Error::ConfigError, "Step must be an object");
                }

                const auto &stepObj = stepItem.toObject();

                // 获取步骤类型
                auto stepTypeIt = stepObj.find("step");
                if (stepTypeIt == stepObj.end() || !stepTypeIt->second.isString()) {
                    return LangCore::Error(LangCore::Error::ConfigError, "Missing required field: step");
                }
                auto stepType = stepTypeIt->second.toString();

                // 检查是否禁用
                bool enabled = true;
                auto enabledIt = stepObj.find("enabled");
                if (enabledIt != stepObj.end() && enabledIt->second.isBool()) {
                    enabled = enabledIt->second.toBool();
                }
                if (!enabled) {
                    continue;
                }

                // 创建步骤
                auto stepExp = G2pStepFactory::create(stepType);
                if (!stepExp) {
                    return stepExp.takeError();
                }
                auto step = stepExp.take();

                // 配置步骤
                auto paramsIt = stepObj.find("params");
                if (paramsIt != stepObj.end() && paramsIt->second.isObject()) {
                    auto configExp = step->configure(m_spec, paramsIt->second.toObject());
                    if (!configExp) {
                        return configExp.takeError();
                    }
                } else {
                    // 没有 params，使用默认配置
                    auto configExp = step->configure(m_spec, LangCore::JsonObject());
                    if (!configExp) {
                        return configExp.takeError();
                    }
                }

                // 添加到管道
                m_steps.push_back(step);
            }
        } else {
            // TemplateG2p 格式（向后兼容）
            // 自动转换为责任链步骤

            // 1. tagAndValidate 步骤
            auto verifyIt = config.find("verify");
            if (verifyIt != config.end() && verifyIt->second.isArray()) {
                auto stepExp = G2pStepFactory::create("tagAndValidate");
                if (!stepExp) {
                    return stepExp.takeError();
                }
                auto step = stepExp.take();
                auto configExp = step->configure(m_spec, config);
                if (!configExp) {
                    return configExp.takeError();
                }
                m_steps.push_back(step);
            }

            // 2. dict 步骤（检查是否有 file 字段）
            auto fileIt = config.find("file");
            if (fileIt != config.end() && fileIt->second.isString()) {
                auto stepExp = G2pStepFactory::create("dict");
                if (!stepExp) {
                    return stepExp.takeError();
                }
                auto step = stepExp.take();
                auto configExp = step->configure(m_spec, config);
                if (!configExp) {
                    return configExp.takeError();
                }
                m_steps.push_back(step);
            }

            // 3. model 步骤（检查是否有 id 字段）
            auto idIt = config.find("id");
            if (idIt != config.end() && idIt->second.isString()) {
                auto stepExp = G2pStepFactory::create("model");
                if (!stepExp) {
                    return stepExp.takeError();
                }
                auto step = stepExp.take();
                auto configExp = step->configure(m_spec, config);
                if (!configExp) {
                    return configExp.takeError();
                }
                m_steps.push_back(step);
            }

            // 4. fallback 步骤（检查是否有 useOriginal 字段）
            auto useOriginalIt = config.find("useOriginal");
            if (useOriginalIt != config.end() && useOriginalIt->second.isBool()) {
                auto stepExp = G2pStepFactory::create("fallback");
                if (!stepExp) {
                    return stepExp.takeError();
                }
                auto step = stepExp.take();
                auto configExp = step->configure(m_spec, config);
                if (!configExp) {
                    return configExp.takeError();
                }
                m_steps.push_back(step);
            }
        }

        return {};
    }

    void G2pPipeline::process(G2pContext &context)
    {
        for (auto &step : m_steps) {
            step->handle(context);
            if (context.isStopProcessing()) {
                break;
            }
        }
    }

    void G2pPipeline::cleanup()
    {
        for (auto &step : m_steps) {
            if (step) {
                step->cleanup();
            }
        }
        m_steps.clear();
    }

} // namespace LangPlugins::ChainG2p