#include "Inference.h"

#include <stdcorelib/pimpl.h>

#include "ITask_p.h"
#include "InferenceContrib.h"

namespace LangMgr
{

    class Inference::Impl : public ITask::Impl {
    public:
        Impl(Inference *decl, const InferenceSpec *spec) : ITask::Impl(decl), spec(spec) {}

        const InferenceSpec *spec;
    };

    Inference::Inference(const InferenceSpec *spec) : ITask(*new Impl(this, spec)) {}

    Inference::~Inference() = default;

    const InferenceSpec *Inference::spec() const {
        __stdc_impl_t;
        return impl.spec;
    }

    LanguageManager *Inference::SU() const {
        __stdc_impl_t;
        return impl.spec->SU();
    }

} // namespace LangMgr
