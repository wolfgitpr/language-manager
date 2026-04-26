#ifndef LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <LangCore/Task/DictTask.h>
#include <LangCore/Module/Module.h>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace LangPlugins::DsDict::Internal::V1
{
    /// Dictionary entry with value (phoneme string etc.)
    struct DictEntry {
        std::string value;

        DictEntry() = default;
        explicit DictEntry(std::string v) : value(std::move(v)) {}
    };

    /// Dictionary container with hash-based lookup and shared_mutex for concurrent reads.
    class Dictionary {
    public:
        Dictionary() = default;
        explicit Dictionary(std::string id, std::string canonicalPath)
            : _id(std::move(id)), _canonicalPath(std::move(canonicalPath)) {}

        /// Lookup a key in the dictionary
        bool lookup(const std::string &key, std::string &value) const;

        /// Check if key exists
        bool contains(const std::string &key) const;

        /// Get dictionary ID
        const std::string &id() const { return _id; }

        /// Get the canonical file path this dictionary was loaded from
        const std::string &canonicalPath() const { return _canonicalPath; }

        /// Get entry count
        size_t size() const;

    private:
        std::string _id;
        std::string _canonicalPath;
        std::unordered_map<std::string, DictEntry> _entries;
        mutable std::shared_mutex _mutex;

        friend class DsDictTaskImpl;
    };

    /// DsDictTaskImpl - V1 implementation of DsDictTask
    class DsDictTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit DsDictTaskImpl(const LangCore::ModuleSpec *spec);
        ~DsDictTaskImpl() override = default;

        LangCore::Expected<void> initialize() override;

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input) override;

        std::string getConfig() const override;

    private:
        const LangCore::ModuleSpec *_spec;

        /// Dictionary storage: dictId -> Dictionary
        std::unordered_map<std::string, std::shared_ptr<Dictionary>> _dictionaries;
        mutable std::shared_mutex _dictMutex;

        /// Dedup: canonical file path -> already-loaded Dictionary (avoids re-parsing
        /// the same file when multiple dictIds reference the same physical file).
        static std::unordered_map<std::string, std::weak_ptr<Dictionary>> s_loadedFiles;
        static std::mutex s_loadedFilesMutex;

        /// Load dictionary from file; deduplicates by canonical path.
        LangCore::Expected<void> loadDictionary(const std::string &dictId,
                                                 const std::filesystem::path &path);

        /// Get dictionary by id (thread-safe read)
        std::shared_ptr<Dictionary> getDictionary(const std::string &dictId) const;

        /// Process query input
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        processQuery(const LangCore::DictInputV1 &input) const;
    };

} // namespace LangPlugins::DsDict::Internal::V1

#endif // LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H