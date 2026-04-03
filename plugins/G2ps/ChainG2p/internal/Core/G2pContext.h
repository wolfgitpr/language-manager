#ifndef LANGPLUGINS_CHAING2P_G2PCONTEXT_H
#define LANGPLUGINS_CHAING2P_G2PCONTEXT_H

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Module/Module.h>
#include <any>
#include <map>
#include <string>
#include <vector>

namespace LangPlugins::ChainG2p
{
    /// G2pContext - G2p 处理上下文
    ///
    /// 负责在处理步骤之间传递数据和状态
    class G2pContext {
    public:
        /// WordInfo - 单词信息
        struct WordInfo {
            // 原始信息
            std::string lyric;                    // 原词
            std::string cleanedLyric;             // 清洗后的词

            // 标记信息（来自 Tag 插件）
            std::string tag;                      // 标记类型
            std::string language;                 // 语言类型
            bool discard = false;                 // 是否丢弃

            // 处理模式
            std::string mode;                     // 处理模式（copy/convert/skip）

            // 结果信息
            std::string pronunciation;            // 发音结果
            std::vector<std::string> candidates;  // 候选发音
            LangCore::G2pErrorType errorType = LangCore::NoError;     // 错误类型

            // 来源标记
            bool fromDict = false;                // 是否来自字典
            bool fromModel = false;               // 是否来自模型
            bool fromFallback = false;            // 是否来自回退

            // 元数据
            std::map<std::string, std::any> metadata;

            // 构造函数
            explicit WordInfo(std::string lyric) : lyric(std::move(lyric)) {}
        };

        /// 构造函数
        /// @param input 输入字符串数组
        explicit G2pContext(const std::vector<std::string> &input, const LangCore::ModuleSpec *spec)
            : m_spec(spec)
        {
            m_words.reserve(input.size());
            for (const auto &lyric : input) {
                m_words.emplace_back(WordInfo(lyric));
            }
        }

        /// 访问方法
        std::vector<WordInfo>& words() { return m_words; }
        const std::vector<WordInfo>& words() const { return m_words; }

        const LangCore::ModuleSpec* spec() const { return m_spec; }
        LangCore::PackageManager* mgr() const { return m_spec->Mgr(); }

        /// 控制方法
        bool isStopProcessing() const { return m_stopProcessing; }
        void setStopProcessing(bool stop) { m_stopProcessing = stop; }

        /// 元数据访问
        void setMetadata(const std::string &key, const std::any &value) {
            m_metadata[key] = value;
        }

        std::any getMetadata(const std::string &key) const {
            auto it = m_metadata.find(key);
            if (it != m_metadata.end()) {
                return it->second;
            }
            return std::any();
        }

        template<typename T>
        T getMetadata(const std::string &key, const T &defaultValue) const {
            auto it = m_metadata.find(key);
            if (it != m_metadata.end()) {
                try {
                    return std::any_cast<T>(it->second);
                } catch (const std::bad_any_cast &) {
                    return defaultValue;
                }
            }
            return defaultValue;
        }

        /// 统计信息
        size_t getDiscardCount() const {
            return std::count_if(m_words.begin(), m_words.end(),
                                 [](const WordInfo &w) { return w.discard; });
        }

        size_t getCopyCount() const {
            return std::count_if(m_words.begin(), m_words.end(),
                                 [](const WordInfo &w) { return w.mode == "copy"; });
        }

        size_t getConvertCount() const {
            return std::count_if(m_words.begin(), m_words.end(),
                                 [](const WordInfo &w) { return w.mode == "convert"; });
        }

        size_t getErrorCount() const {
            return std::count_if(m_words.begin(), m_words.end(),
                                 [](const WordInfo &w) { return w.errorType != LangCore::NoError; });
        }

    private:
        std::vector<WordInfo> m_words;
        const LangCore::ModuleSpec* m_spec;
        bool m_stopProcessing = false;
        std::map<std::string, std::any> m_metadata;
    };

} // namespace LangPlugins::ChainG2p

#endif // LANGPLUGINS_CHAING2P_G2PCONTEXT_H