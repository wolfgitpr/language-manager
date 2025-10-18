#ifndef LANGUAGE_MANAGER_NAMEDOBJECT_P_H
#define LANGUAGE_MANAGER_NAMEDOBJECT_P_H

#include <map>

#include <stdcorelib/linked_map.h>

#include "NamedObject.h"

namespace LangMgr {

    class NamedObject::Impl {
    public:
        explicit Impl(NamedObject *decl) : _decl(decl) {
        }
        virtual ~Impl() = default;

        NamedObject *_decl;

        std::string name;
        std::map<std::string, std::any, std::less<>> properties;
    };

    class ObjectPool::Impl : public NamedObject::Impl {
    public:
        explicit Impl(ObjectPool *decl) : NamedObject::Impl(decl) {
        }
        virtual ~Impl();

        std::map<std::string, stdc::linked_map<const NamedObject *, NO<NamedObject>>, std::less<>>
            objects;
    };

}

#endif // LANGUAGE_MANAGER_NAMEDOBJECT_P_H