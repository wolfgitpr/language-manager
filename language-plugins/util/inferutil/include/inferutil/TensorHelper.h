#ifndef LANGPLUGINS_INFERUTIL_TENSORHELPER_H
#define LANGPLUGINS_INFERUTIL_TENSORHELPER_H

#include <LangMgr/Support/Expected.h>
#include <LangPlugins/Core/Tensor.h>
#include <cstdint>
#include <vector>

namespace LangPlugins::inferutil
{
    template <typename T>
    class TensorHelper {
    public:
        static LangMgr::Expected<TensorHelper> createFor1DArray(size_t size) {
            TensorHelper helper;
            const std::vector shape{1, static_cast<int64_t>(size)};
            auto exp = LangPlugins::Tensor::create(LangPlugins::tensor_traits<T>::data_type, shape);
            if (!exp) {
                return exp.takeError();
            }
            helper._tensor = exp.take();
            auto dataPtr = helper._tensor->mutableData<T>();
            if (STDCORELIB_UNLIKELY(dataPtr == nullptr)) {
                return LangMgr::Error(LangMgr::Error::SessionError, "failed to create tensor");
            }
            helper._current = dataPtr;
            helper._end = dataPtr + size;
            return helper;
        }

        inline bool write(T value) {
            if (_current >= _end) {
                return false;
            }
            *_current++ = value;
            return true;
        }

        inline void writeUnchecked(T value) { *_current++ = value; }

        inline bool isComplete() const { return _current == _end; }

        inline LangMgr::NO<LangPlugins::Tensor> &value() { return _tensor; }

        inline LangMgr::NO<LangPlugins::Tensor> &&take() { return std::move(_tensor); }

        STDCORELIB_DISABLE_COPY(TensorHelper)

        TensorHelper(TensorHelper &&other) noexcept :
            _tensor(std::move(other._tensor)), _current(other._current), _end(other._end) {
            other._current = nullptr;
            other._end = nullptr;
        }

        TensorHelper &operator=(TensorHelper &&other) noexcept {
            if (this != &other) {
                _tensor = std::move(other._tensor);
                _current = other._current;
                _end = other._end;

                other._current = nullptr;
                other._end = nullptr;
            }
            return *this;
        }

    private:
        TensorHelper() : _current(nullptr), _end(nullptr) {};

        LangMgr::NO<LangPlugins::Tensor> _tensor;
        T *_current;
        const T *_end;
    };
} // namespace LangPlugins::inferutil
#endif // LANGPLUGINS_INFERUTIL_TENSORHELPER_H
