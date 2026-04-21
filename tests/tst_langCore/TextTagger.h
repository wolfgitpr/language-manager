#ifndef TST_LANGCORE_TEXTTAGGER_H
#define TST_LANGCORE_TEXTTAGGER_H

#include <filesystem>
#include <string>
#include <vector>

#include <LangCore/Base/LangCommon.h>

namespace TestUtils
{
    /// 从 JSON 配置目录加载所有 tagger 配置并初始化
    /// @param configDir   tagger 配置目录（包含 cmn.json, eng.json 等）
    /// @param dictRootDir 字典文件的根搜索目录（递归查找）
    /// @return true 成功
    bool initTaggers(const std::filesystem::path &configDir, const std::filesystem::path &dictRootDir);

    /// 对分割后的文本段执行语言标注
    /// @param input             分割后的文本段
    /// @param discard           是否过滤 discard=true 的段
    /// @param priorityLanguages 优先标注的语言列表
    /// @return 标注结果
    std::vector<LangCore::TaggerRes> tag(const std::vector<std::string> &input,
                                          bool discard = false,
                                          const std::vector<std::string> &priorityLanguages = {});

} // namespace TestUtils

#endif // TST_LANGCORE_TEXTTAGGER_H
