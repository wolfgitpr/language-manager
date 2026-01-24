#include <LangCore/Base/ObjectPool.h>
#include "ObjectPool_p.h"

#include <LangCore/Base/NamedObject.h>

#include <stdcorelib/pimpl.h>

namespace LangCore
{

    ObjectPool::Impl::~Impl() {}

    ObjectPool::ObjectPool() : ObjectPool(*new Impl(this)) {}

    ObjectPool::~ObjectPool() = default;

    void ObjectPool::addObject(const NO<NamedObject> &obj) { addObject({}, obj); }

    void ObjectPool::addObject(const std::string_view id, const NO<NamedObject> &obj) {
        __stdc_impl_t;

        if (!obj) {
            return;
        }

        auto it = impl.objects.find(id);
        if (it == impl.objects.end()) {
            it = impl.objects.emplace(std::string(id), stdc::linked_map<const NamedObject *, NO<NamedObject>>()).first;
        }
        it->second.append(obj.get(), obj);
    }

    void ObjectPool::removeObject(const NamedObject *obj) { removeObject({}, obj); }

    void ObjectPool::removeObject(const std::string_view id, const NamedObject *obj) {
        __stdc_impl_t;
        const auto it = impl.objects.find(id);
        if (it == impl.objects.end()) {
            return;
        }
        {
            auto &map = it->second;
            const auto it2 = map.find(obj);
            if (it2 == map.end()) {
                return;
            }
            aboutToRemoveObject(id, it2->second);
            map.erase(it2);
            if (map.empty()) {
                impl.objects.erase(it);
            }
        }
    }

    void ObjectPool::removeObjects(const std::string_view id) {
        __stdc_impl_t;
        const auto it = impl.objects.find(id);
        if (it == impl.objects.end()) {
            return;
        }
        auto &map = it->second;
        for (auto it2 = map.rbegin(); it2 != map.rend(); ++it2) {
            aboutToRemoveObject(id, it2->second);
        }
        impl.objects.erase(it);
    }

    void ObjectPool::removeAllObjects() {
        __stdc_impl_t;
        for (auto &[fst, snd] : impl.objects) {
            auto &map = snd;
            for (auto it = map.rbegin(); it != map.rend(); ++it) {
                aboutToRemoveObject(fst, it->second);
            }
        }
        impl.objects.clear();
    }

    std::vector<NO<NamedObject>> ObjectPool::allObjects() const {
        __stdc_impl_t;
        std::vector<NO<NamedObject>> res;
        for (const auto &[fst, snd] : impl.objects) {
            auto values = snd.values();
            res.insert(res.end(), values.begin(), values.end());
        }
        return res;
    }

    std::vector<NO<NamedObject>> ObjectPool::getObjects(const std::string_view id) const {
        __stdc_impl_t;
        const auto it = impl.objects.find(id);
        if (it == impl.objects.end()) {
            return {};
        }
        return it->second.values();
    }

    NO<NamedObject> ObjectPool::getFirstObject(const std::string_view id) const {
        __stdc_impl_t;
        const auto it = impl.objects.find(id);
        if (it == impl.objects.end()) {
            return {};
        }
        return it->second.begin()->second;
    }

    void ObjectPool::objectAdded(const std::string_view id, const NO<NamedObject> &obj) {
        (void)id;
        (void)obj;
    }

    void ObjectPool::aboutToRemoveObject(const std::string_view id, const NO<NamedObject> &obj) {
        (void)id;
        (void)obj;
    }

    ObjectPool::ObjectPool(Impl &impl) : NamedObject(impl) {}

} // namespace LangCore
