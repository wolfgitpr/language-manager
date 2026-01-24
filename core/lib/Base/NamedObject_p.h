#ifndef LANGCORE_NAMEDOBJECT_P_H
#define LANGCORE_NAMEDOBJECT_P_H

#include <map>

#include <LangCore/Base/NamedObject.h>

namespace LangCore
{

    class NamedObject::Impl {
    public:
        explicit Impl(NamedObject *decl) : _decl(decl) {}
        virtual ~Impl() = default;

        NamedObject *_decl;

        std::string name;
        std::map<std::string, std::any, std::less<>> properties;
    };

} // namespace LangCore

#endif // LANGCORE_NAMEDOBJECT_P_H
