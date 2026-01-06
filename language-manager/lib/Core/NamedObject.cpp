#include "NamedObject.h"
#include "NamedObject_p.h"

#include <stdcorelib/pimpl.h>

namespace LangMgr
{

    NamedObject::NamedObject() : NamedObject(*new Impl(this)) {}

    NamedObject::NamedObject(std::string name) : NamedObject() { setObjectName(std::move(name)); }

    NamedObject::~NamedObject() = default;

    const std::string &NamedObject::objectName() const {
        __stdc_impl_t;
        return impl.name;
    }

    void NamedObject::setObjectName(std::string name) {
        __stdc_impl_t;
        impl.name = std::move(name);
    }

    static std::any &staticEmptyObjectProperty() {
        static std::any empty;
        return empty;
    }

    const std::any &NamedObject::property(const std::string_view name) const {
        __stdc_impl_t;
        const auto it = impl.properties.find(name);
        if (it == impl.properties.end()) {
            return staticEmptyObjectProperty();
        }
        return it->second;
    }

    void NamedObject::setProperty(const std::string_view name, std::any value) {
        __stdc_impl_t;
        if (const auto it = impl.properties.find(name); it == impl.properties.end()) {
            impl.properties[std::string(name)] = std::move(value);
        } else {
            it->second = std::move(value);
        }
    }

    NamedObject::NamedObject(Impl &impl) : _impl(&impl) {}

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

} // namespace LangMgr
