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
} // namespace LangMgr
