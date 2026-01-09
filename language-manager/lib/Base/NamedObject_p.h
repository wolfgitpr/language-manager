#ifndef LANGUAGE_MANAGER_NAMEDOBJECT_P_H
#define LANGUAGE_MANAGER_NAMEDOBJECT_P_H

#include <map>

#include <LangMgr/Base/NamedObject.h>

namespace LangMgr
{

    class NamedObject::Impl {
    public:
        explicit Impl(NamedObject *decl) : _decl(decl) {}
        virtual ~Impl() = default;

        NamedObject *_decl;

        std::string name;
        std::map<std::string, std::any, std::less<>> properties;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_NAMEDOBJECT_P_H
