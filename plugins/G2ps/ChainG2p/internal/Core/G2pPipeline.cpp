#include "G2pPipeline.h"
#include <LangCore/Support/Error.h>
#include <stdcorelib/str.h>

namespace LangPlugins::ChainG2p
{
    /// 格式化步骤错误消息
    /// @param stepIndex 步骤索引
    /// @param stepType 步骤类型
    /// @param message 错误消息
    /// @param suggestion 建议（可选）
    /// @return 格式化后的错误
    static LangCore::Error formatStepError(size_t stepIndex, const std::string &stepType,
                                          const std::string &message, const std::string &suggestion = "") {
        std::string formattedMessage = stdc::formatN("Step #%1 (%2): %3", stepIndex, stepType, message);
        return LangCore::Error(LangCore::Error::ConfigError, formattedMessage, suggestion);
    }

    /// 配置步骤
    /// @param step 步骤对象
    /// @param spec 模块规范
    /// @param params 配置参数
    /// @param stepIndex 步骤索引
    /// @param stepType 步骤类型
    /// @param useDefault 是否使用默认配置
    /// @return 配置结果
    static LangCore::Expected<void> configureStep(std::shared_ptr<G2pStep> step,
                                                  const LangCore::ModuleSpec *spec,
                                                  const LangCore::JsonObject &params,
                                                  size_t stepIndex, const std::string &stepType,
                                                  bool useDefault = false) {
        auto configExp = step->configure(spec, params);
        if (!configExp) {
            std::string suggestion = useDefault ?
                "Check the step's default configuration" :
                "Check the 'params' field in your configuration";
            return formatStepError(stepIndex, stepType, "Configuration failed: " + configExp.error().message(), suggestion);
        }
        return {};
    }
    LangCore::Expected<void> G2pPipeline::configure(const LangCore::JsonObject &config)
    {
        // 检查是否是责任链格式（有 steps 字段）
        auto stepsIt = config.find("steps");
        if (stepsIt != config.end() && stepsIt->second.isArray()) {
            // 责任链格式
            const auto &stepsArray = stepsIt->second.toArray();

            // 安全性检查：限制步骤数量
            const size_t maxSteps = 50;  // 最大步骤数量限制
            if (stepsArray.size() > maxSteps) {
                return LangCore::Error(LangCore::Error::ConfigError,
                                     stdc::formatN("Too many steps: %1 (maximum allowed: %2)", stepsArray.size(), maxSteps),
                                     "Reduce the number of steps in your configuration");
            }

            m_steps.reserve(stepsArray.size());

            for (size_t stepIndex = 0; stepIndex < stepsArray.size(); ++stepIndex) {
                const auto &stepItem = stepsArray[stepIndex];
                if (!stepItem.isObject()) {
                    return LangCore::Error(LangCore::Error::ConfigError,
                                         stdc::formatN("Step #%1 must be an object", stepIndex));
                }

                const auto &stepObj = stepItem.toObject();

                // 获取步骤类型
                auto stepTypeIt = stepObj.find("step");
                if (stepTypeIt == stepObj.end() || !stepTypeIt->second.isString()) {
                    return LangCore::Error(LangCore::Error::ConfigError,
                                         stdc::formatN("Step #%1: Missing required field: step", stepIndex),
                                         "Add the 'step' field to specify the step type");
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
                    return LangCore::Error(stepExp.error().type(),
                                         stdc::formatN("Step #%1: Failed to create step type '%2': %3", stepIndex, stepType, stepExp.error().message()),
                                         stdc::formatN("Check if step type '%1' is supported. Supported types: %2", stepType,
                                                     G2pStepFactory::supportedTypesAsString()));
                }
                auto step = stepExp.take();

                // 设置 Task
                step->setTask(m_task);

                // 配置步骤
                auto paramsIt = stepObj.find("params");
                if (paramsIt != stepObj.end() && paramsIt->second.isObject()) {
                    auto configResult = configureStep(step, m_spec, paramsIt->second.toObject(), stepIndex, stepType, false);
                    if (!configResult) {
                        return configResult.takeError();
                    }
                } else {
                    // 没有 params，使用默认配置
                    auto configResult = configureStep(step, m_spec, LangCore::JsonObject(), stepIndex, stepType, true);
                    if (!configResult) {
                        return configResult.takeError();
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