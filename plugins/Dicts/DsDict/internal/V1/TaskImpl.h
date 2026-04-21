#ifndef LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H
#define LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H

#include <LangCore/Task/VersionedTaskImplBase.h>
#include <LangCore/Task/DictTask.h>
#include <LangCore/Module/Module.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <mutex>
#include <functional>

namespace LangPlugins::DsDict::Internal::V1
{
    /// Dictionary entry with metadata
    struct DictEntry {
        std::string key;
        std::string value;
        std::string metadata;

        DictEntry() = default;
        DictEntry(std::string k, std::string v, std::string m = "")
            : key(std::move(k)), value(std::move(v)), metadata(std::move(m)) {}
    };

    /// Dictionary container with hash-based lookup
    class Dictionary {
    public:
        Dictionary() = default;
        explicit Dictionary(std::string id) : _id(std::move(id)) {}

        /// Add an entry to the dictionary
        void addEntry(const std::string& key, const std::string& value, const std::string& metadata = "");

        /// Batch add entries
        void addEntries(const std::vector<DictEntry>& entries);

        /// Lookup a key in the dictionary
        bool lookup(const std::string& key, std::string& value, std::string& metadata) const;

        /// Check if key exists
        bool contains(const std::string& key) const;

        /// Get all entries
        std::vector<DictEntry> getAllEntries() const;

        /// Clear all entries
        void clear();

        /// Get dictionary ID
        const std::string& id() const { return _id; }

        /// Get entry count
        size_t size() const { return _entries.size(); }

        /// Calculate hash of the dictionary state
        size_t hash() const;

    private:
        std::string _id;
        std::unordered_map<std::string, DictEntry> _entries;
        mutable std::mutex _mutex;
    };

    /// DsDictTaskImpl - V1 implementation of DsDictTask
    class DsDictTaskImpl final : public LangCore::VersionedTaskImplBase {
    public:
        explicit DsDictTaskImpl(const LangCore::ModuleSpec *spec);
        ~DsDictTaskImpl() = default;

        LangCore::Expected<void> initialize();

        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        start(const LangCore::NO<LangCore::TaskInput> &input);

        std::string getConfig() const;

    private:
        const LangCore::ModuleSpec *_spec;

        /// Dictionary storage: dictId -> Dictionary
        std::unordered_map<std::string, std::shared_ptr<Dictionary>> _dictionaries;
        mutable std::mutex _dictMutex;

        /// Load dictionary from file
        LangCore::Expected<void> loadDictionary(const std::string& dictId, const std::filesystem::path& path);

        /// Create or get dictionary
        std::shared_ptr<Dictionary> getDictionary(const std::string& dictId);

        /// Process query input
        LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
        processQuery(const LangCore::DictInputV1& input);
    };

} // namespace LangPlugins::DsDict::Internal::V1

#endif // LANGPLUGINSDSDICT_INTERNAL_V1_TASKIMPL_H