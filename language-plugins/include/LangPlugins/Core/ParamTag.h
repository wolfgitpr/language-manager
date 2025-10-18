#ifndef LANGPLUGINS_PARAMTAG_H
#define LANGPLUGINS_PARAMTAG_H

#include <string_view>
#include <type_traits>

namespace LangPlugins
{

    class ParamTag {
    public:
        constexpr ParamTag() = default;

        template <size_t N>
        explicit constexpr ParamTag(const char (&name)[N]) : _name(name, N - 1) {}

        constexpr std::string_view name() const { return _name; }

        bool operator==(const ParamTag &RHS) const { return _name == RHS._name; }

        bool operator!=(const ParamTag &RHS) const { return _name != RHS._name; }

        bool operator<(const ParamTag &RHS) const { return _name < RHS._name; }

        bool operator>(const ParamTag &RHS) const { return _name > RHS._name; }

        bool operator<=(const ParamTag &RHS) const { return _name <= RHS._name; }

        bool operator>=(const ParamTag &RHS) const { return _name >= RHS._name; }

        size_t hash() const { return std::hash<std::string_view>()(_name); }

    protected:
        const std::string_view _name;
    };

} // namespace LangPlugins

template <>
struct std::hash<LangPlugins::ParamTag> {
    size_t operator()(const LangPlugins::ParamTag &key) const noexcept { return key.hash(); }
}; // namespace std

#endif // LANGPLUGINS_PARAMTAG_H
