#ifndef LANGUAGE_MANAGER_U32STR_H
#define LANGUAGE_MANAGER_U32STR_H

#include <string>

#include <LangMgr/LangMgrGlobal.h>

namespace LangMgr
{
    LANGMGR_EXPORT std::string u32strToUtf8str(const char32_t &ch32);
    LANGMGR_EXPORT std::string u32strToUtf8str(const std::u32string &u32str);
    LANGMGR_EXPORT std::u32string utf8strToU32str(const std::string &utf8str);

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_U32STR_H
