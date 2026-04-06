#ifndef LANGPLUGINS_CHAING2P_G2PSTEP_H
#define LANGPLUGINS_CHAING2P_G2PSTEP_H

#include <LangCore/Module/Module.h>
#include <LangCore/Support/Expected.h>
#include <LangCore/Support/JSON.h>
#include "G2pContext.h"
#include <memory>
#include <string>
#include <vector>

namespace LangPlugins::ChainG2p
{
    /// G2pStep - G2p 处理步骤基类
    ///
    /// 所有处理步骤必须继承此类并实现以下方法：
    /// - configure(): 配置步骤
    /// - handle(): 处理输入
    /// - cleanup(): 清理资源
    class G2pStep {
    public:
        virtual ~G2pStep() = default;

        /// 配置步骤
        /// @param spec 模块规范
        /// @param config 配置对象
        /// @return 成功返回 Expected<void>::success()，失败返回错误信息
        virtual LangCore::Expected<void> configure(const LangCore::ModuleSpec *spec,
                                                    const LangCore::JsonObject &config) = 0;

        /// 处理输入
        /// @param context 处理上下文
        virtual void handle(G2pContext &context) = 0;

        /// 清理资源
        virtual void cleanup() {}

        /// 获取步骤名称
        /// @return 步骤名称
        virtual std::string name() const = 0;

    protected:
        const LangCore::ModuleSpec* m_spec = nullptr;
        LangCore::PackageManager* m_mgr = nullptr;
        std::string m_config;
    };

    /// G2pStepFactory - 步骤工厂
    class G2pStepFactory {
    public:
        /// 创建步骤
        /// @param stepType 步骤类型
        /// @return 成功返回步骤对象，失败返回错误信息
        static LangCore::Expected<std::shared_ptr<G2pStep>> create(const std::string &stepType);

        /// 获取所有支持的步骤类型
        /// @return 步骤类型列表
        static std::vector<std::string> supportedTypes();

        /// 获取所有支持的步骤类型的字符串表示
        /// @return 步骤类型列表的字符串表示
        static std::string supportedTypesAsString();

    private:
        /// 连接字符串
        /// @param strings 字符串列表
        /// @param delimiter 分隔符
        /// @return 连接后的字符串
        static std::string joinStrings(const std::vector<std::string> &strings, const std::string &delimiter);
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_G2PSTEP_H