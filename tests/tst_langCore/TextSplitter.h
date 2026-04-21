#ifndef TST_LANGCORE_TEXTSPLITTER_H
#define TST_LANGCORE_TEXTSPLITTER_H

#include <filesystem>
#include <string>
#include <vector>

namespace TestUtils
{
    /// 从 JSON 配置目录加载所有 splitter 配置并初始化
    /// @param configDir  splitter 配置目录（包含 cmn.json, eng.json 等）
    /// @return true 成功
    bool initSplitters(const std::filesystem::path &configDir);

    /// 对输入文本执行所有 splitter 分割
    std::vector<std::string> split(const std::string &input);

    /// 对输入文本数组执行所有 splitter 分割
    std::vector<std::string> split(const std::vector<std::string> &input);

} // namespace TestUtils

#endif // TST_LANGCORE_TEXTSPLITTER_H
