#ifndef LANGCORE_OBJECTPOOL_P_H
#define LANGCORE_OBJECTPOOL_P_H

#include <map>

#include <stdcorelib/linked_map.h>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/Base/ObjectPool.h>

#include "NamedObject_p.h"

namespace LangCore
{

    class ObjectPool::Impl : public NamedObject::Impl {
    public:
        explicit Impl(ObjectPool *decl) : NamedObject::Impl(decl) {}
        ~Impl() override;

        std::map<std::string, stdc::linked_map<const NamedObject *, NO<NamedObject>>, std::less<>> objects;
    };

} // namespace LangCore

#endif // LANGCORE_OBJECTPOOL_P_H
