#ifndef LANGCORE_CONTEXTUTILS_H
#define LANGCORE_CONTEXTUTILS_H

#include <string>
#include <string_view>

#include <stdcorelib/support/versionnumber.h>

#include <LangCore/Support/Expected.h>

namespace LangCore
{
    /// ContextKey — composite key for context namespace: (name, version).
    /// Empty context + empty version = default context.
    /// Non-empty context + empty version = unversioned context.
    /// Non-empty context + non-empty version = versioned context.
    struct ContextKey {
        std::string context;            ///< Voice bank name ("" = default context)
        stdc::VersionNumber version;    ///< Voice bank version (isEmpty() = unversioned)

        ContextKey() = default;
        explicit ContextKey(std::string context, stdc::VersionNumber version = {})
            : context(std::move(context)), version(std::move(version)) {}

        bool operator<(const ContextKey &o) const {
            if (context != o.context)
                return context < o.context;
            return version < o.version;
        }

        bool operator==(const ContextKey &o) const {
            return context == o.context && version == o.version;
        }

        bool operator!=(const ContextKey &o) const { return !(*this == o); }

        /// Whether this context has a version
        bool isVersioned() const { return !version.isEmpty(); }

        /// Whether this is the default context (empty name, no version)
        bool isDefault() const { return context.empty() && version.isEmpty(); }

        /// Human-readable string:
        ///   "" → "(default)"
        ///   "SingerA" → "SingerA"
        ///   "SingerA" + 2.0.0 → "SingerA@2.0.0"
        std::string toString() const {
            if (context.empty() && version.isEmpty())
                return "(default)";
            if (version.isEmpty())
                return context;
            return context + "@" + version.toString();
        }
    };

    struct FqidParseResult {
        std::string context;
        stdc::VersionNumber version;    ///< Parsed from "context@version:moduleId"
        std::string moduleId;
    };

    class ContextUtils {
    public:
        /// Parse FQID string into context + version + moduleId.
        /// "SingerA@2.0.0:g2p-cmn" → {"SingerA", 2.0.0, "g2p-cmn"}
        /// "SingerA:g2p-cmn" → {"SingerA", {}, "g2p-cmn"}
        /// "g2p-cmn" → {"", {}, "g2p-cmn"}
        /// ":g2p-cmn" → {"", {}, "g2p-cmn"}
        /// "A:B:C" → {"A", {}, "B:C"} (first colon separates)
        static FqidParseResult parseFqid(const std::string_view &fqid) {
            FqidParseResult result;
            auto colonPos = fqid.find(':');
            if (colonPos == std::string_view::npos) {
                // No colon: plain moduleId
                result.moduleId = std::string(fqid);
            } else {
                auto contextPart = fqid.substr(0, colonPos);
                result.moduleId = std::string(fqid.substr(colonPos + 1));

                // Check for '@' in context part → version
                auto atPos = contextPart.find('@');
                if (atPos == std::string_view::npos) {
                    result.context = std::string(contextPart);
                } else {
                    result.context = std::string(contextPart.substr(0, atPos));
                    result.version =
                        stdc::VersionNumber::fromString(std::string(contextPart.substr(atPos + 1)));
                }
            }
            return result;
        }

        /// Format ContextKey + moduleId into FQID string.
        /// ({"SingerA", 2.0.0}, "g2p-cmn") → "SingerA@2.0.0:g2p-cmn"
        /// ({"SingerA", {}}, "g2p-cmn") → "SingerA:g2p-cmn"
        /// ({"", {}}, "g2p-cmn") → "g2p-cmn"
        static std::string formatFqid(const ContextKey &ctxKey, const std::string_view &moduleId) {
            if (ctxKey.isDefault())
                return std::string(moduleId);
            return ctxKey.toString() + ":" + std::string(moduleId);
        }

        /// Legacy overload: format with plain context string (no version).
        static std::string formatFqid(const std::string_view &context, const std::string_view &moduleId) {
            return formatFqid(ContextKey(std::string(context)), moduleId);
        }

        /// Validate context name. Empty string is valid (default context).
        /// Allowed chars: [A-Za-z0-9_.-]
        /// Max length: 128
        /// Returns Error on invalid.
        static Expected<void> validateContextName(const std::string_view &context) {
            if (context.empty())
                return {}; // default context is always valid

            if (context.size() > 128) {
                return Error(Error::ValidationError,
                             "Context name '" + std::string(context.substr(0, 20)) +
                                 "...' exceeds maximum length (128)");
            }

            for (size_t i = 0; i < context.size(); ++i) {
                char ch = context[i];
                bool valid = (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') ||
                    ch == '_' || ch == '.' || ch == '-';
                if (!valid) {
                    return Error(Error::ValidationError,
                                 "Invalid context name '" + std::string(context) +
                                     "': contains forbidden character '" + std::string(1, ch) +
                                     "'. Allowed: [A-Za-z0-9_.-]");
                }
            }
            return {};
        }

        /// Validate that a moduleId does not contain ':' (reserved for FQID separation).
        static Expected<void> validateModuleId(const std::string_view &moduleId) {
            if (moduleId.find(':') != std::string_view::npos) {
                return Error(
                    Error::ValidationError,
                    "Module ID '" + std::string(moduleId) + "' contains ':' which is reserved for context separation");
            }
            return {};
        }
    };

} // namespace LangCore

#endif // LANGCORE_CONTEXTUTILS_H
