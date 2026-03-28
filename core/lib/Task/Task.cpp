#include "LangCore/Task/Task.h"
#include "Task_p.h"

#include <stdcorelib/pimpl.h>

#include <LangCore/Core/PackageManager.h>
#include <LangCore/Module/Module.h>

namespace LangCore
{

    Task::Task() : Task(*new Impl(this)) {}
    Task::Task(const ModuleSpec *spec) : Task(*new Impl(this)) {
        __stdc_impl_t;
        impl.spec_ = spec;
    }

    Task::~Task() = default;

    Task::Task(Impl &impl) : NamedObject(impl) {}

    const ModuleSpec *Task::spec() const {
        __stdc_impl_t;
        return impl.spec_;
    }

    PackageManager *Task::Mgr() const {
        __stdc_impl_t;
        return impl.spec_->Mgr();
    }

    Expected<NO<NamedObject>> Task::getObject(const std::string &category, const std::string &id) const {
        const auto inferenceCate = this->Mgr()->category(category);
        if (!inferenceCate)
            return Error(Error::SessionError, "could not find category: " + category);

        const auto inferenceObject = inferenceCate->getFirstObject(id);
        if (!inferenceObject)
            return Error(Error::SessionError, "could not find id: " + id);

        return inferenceObject;
    }


} // namespace LangCore
