#ifndef LANGPLUGINS_INFERUTIL_TENSORHELPER_H
#define LANGPLUGINS_INFERUTIL_TENSORHELPER_H

#include <LangMgr/Support/Expected.h>
#include <LangPlugins/Core/Tensor.h>
#include <cstdint>
#include <vector>

namespace LangPlugins::inferUtil
{
    template <typename T>
    class TensorHelper {
    public:
        static LangMgr::Expected<TensorHelper> createFor1DArray(size_t size) {
            TensorHelper helper;
            const std::vector shape{1, static_cast<int64_t>(size)};
            auto exp = Tensor::create(tensor_traits<T>::data_type, shape);
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

        bool write(T value) {
            if (_current >= _end) {
                return false;
            }
            *_current++ = value;
            return true;
        }

        void writeUnchecked(T value) { *_current++ = value; }

        bool isComplete() const { return _current == _end; }

        LangMgr::NO<Tensor> &value() { return _tensor; }

        LangMgr::NO<Tensor> &&take() { return std::move(_tensor); }

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

        LangMgr::NO<Tensor> _tensor;
        T *_current;
        const T *_end;
    };
} // namespace LangPlugins::inferUtil
#endif // LANGPLUGINS_INFERUTIL_TENSORHELPER_H
