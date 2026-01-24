#ifndef LANGCORE_NAMEDOBJECT_H
#define LANGCORE_NAMEDOBJECT_H


#include <any>
#include <memory>
#include <stdcorelib/adt/array_view.h>
#include <string>
#include <string_view>

#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{

    class LANGCORE_EXPORT NamedObject {
    public:
        NamedObject();
        explicit NamedObject(std::string name);
        virtual ~NamedObject();

        const std::string &objectName() const;
        void setObjectName(std::string name);

        const std::any &property(std::string_view name) const;
        void setProperty(std::string_view name, std::any value);

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;
        explicit NamedObject(Impl &impl);
    };

    /// NO - A shared pointer wrapper for \c NamedObject instance.
    template <class T>
    class NO : public std::shared_ptr<T> {
        static_assert(std::is_base_of_v<NamedObject, T>, "T should inherit from LangPlugins::NamedObject");

    public:
        using Base = std::shared_ptr<T>;

        template <typename... Args>
        using Constructible = std::enable_if_t<std::is_constructible_v<Base, Args...>>;

        constexpr NO() noexcept : Base() {}

        NO(const Base &RHS) noexcept : Base(RHS) {}

        template <typename U, typename = Constructible<U *>>
        explicit NO(U *p) : Base(p) {}

        template <typename U, typename Deleter, typename = Constructible<U *, Deleter>>
        NO(U *p, Deleter d) : Base(p, std::move(d)) {}

        template <typename Deleter>
        NO(std::nullptr_t p, Deleter d) : Base(p, std::move(d)) {}

        template <typename U, typename Deleter, typename Alloc, typename = Constructible<U *, Deleter, Alloc>>
        NO(U *p, Deleter d, Alloc a) : Base(p, std::move(d), std::move(a)) {}

        template <typename Deleter, typename Alloc>
        NO(std::nullptr_t p, Deleter d, Alloc a) : Base(p, std::move(d), std::move(a)) {}

        template <typename U>
        NO(const std::shared_ptr<U> &RHS, T *p) noexcept : Base(RHS, p) {}

        template <typename U, typename = Constructible<const std::shared_ptr<U> &>>
        NO(const std::shared_ptr<U> &RHS) noexcept : Base(RHS) {}

        NO(Base &&RHS) noexcept : Base(std::move(RHS)) {}

        template <typename U, typename = Constructible<std::shared_ptr<U>>>
        NO(Base &&RHS) noexcept : Base(std::move(RHS)) {}

        template <typename U, typename = Constructible<const std::weak_ptr<U> &>>
        explicit NO(const std::weak_ptr<U> &RHS) : Base(RHS) {}

        template <typename U, typename Deleter, typename = Constructible<std::unique_ptr<U, Deleter>>>
        NO(std::unique_ptr<U, Deleter> &&RHS) : Base(std::move(RHS)) {}

        constexpr NO(std::nullptr_t) noexcept : Base() {}

        template <class U>
        NO<U> as() const noexcept {
            return std::static_pointer_cast<U>(*this);
        }

        template <class... Args>
        static NO create(Args &&...args) {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }
    };

} // namespace LangCore

#endif // LANGCORE_NAMEDOBJECT_H
