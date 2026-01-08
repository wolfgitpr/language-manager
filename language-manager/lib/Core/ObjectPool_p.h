#ifndef LANGMGR_OBJECTPOOL_P_H
#define LANGMGR_OBJECTPOOL_P_H


#include <map>

#include <stdcorelib/linked_map.h>

#include <LangMgr/Core/NamedObject.h>
#include <LangMgr/Core/ObjectPool.h>

#include "NamedObject_p.h"

namespace LangMgr
{

    class ObjectPool::Impl : public NamedObject::Impl {
    public:
        explicit Impl(ObjectPool *decl) : NamedObject::Impl(decl) {}
        ~Impl() override;

        std::map<std::string, stdc::linked_map<const NamedObject *, NO<NamedObject>>, std::less<>> objects;
    };

} // namespace LangMgr

#endif // LANGMGR_OBJECTPOOL_P_H
