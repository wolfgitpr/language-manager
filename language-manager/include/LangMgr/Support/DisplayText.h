#ifndef LANGUAGE_MANAGER_DISPLAYTEXT_H
#define LANGUAGE_MANAGER_DISPLAYTEXT_H

#include <map>
#include <memory>
#include <string>

#include <LangMgr/Support/JSON.h>

namespace LangMgr
{

    /// DisplayText - Represents a text with multiple translations.
    class LANGMGR_EXPORT DisplayText {
    public:
        /// Constructs an empty display text object.
        DisplayText();

        /// Constructs with a default text.
        DisplayText(std::string text);

        /// Constructs with a default text and a map, where the key is the locale code and the value
        /// is the corresponding text.
        DisplayText(std::string defaultText, const std::map<std::string, std::string> &texts);

        /// Constructs with a JSON value.
        /// \note The JSON value must be a string-mapping object, with the key being the locale code
        /// and the value being the corresponding text. If the \c _ property exists, use it as the
        /// default text; otherwise, search for the property of \c en_XX and try to use the value as
        /// the default text.
        explicit DisplayText(const JsonValue &value);

        ~DisplayText();

        DisplayText &operator=(std::string text);
        DisplayText &operator=(const JsonValue &value);

        inline void swap(DisplayText &RHS) noexcept { _impl.swap(RHS._impl); }

    public:
        const std::string &text() const;
        const std::string &text(std::string_view locale) const;

        bool isEmpty() const;

    protected:
        class Impl;
        std::shared_ptr<Impl> _impl;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_DISPLAYTEXT_H
