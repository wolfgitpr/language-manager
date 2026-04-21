#include "TaskImpl.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace LangPlugins::DsDict::Internal::V1
{
    // ============================================================
    // Dictionary Implementation
    // ============================================================

    void Dictionary::addEntry(const std::string& key, const std::string& value, const std::string& metadata) {
        std::lock_guard<std::mutex> lock(_mutex);
        _entries[key] = DictEntry(key, value, metadata);
    }

    void Dictionary::addEntries(const std::vector<DictEntry>& entries) {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& entry : entries) {
            _entries[entry.key] = entry;
        }
    }

    bool Dictionary::lookup(const std::string& key, std::string& value, std::string& metadata) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _entries.find(key);
        if (it != _entries.end()) {
            value = it->second.value;
            metadata = it->second.metadata;
            return true;
        }
        return false;
    }

    bool Dictionary::contains(const std::string& key) const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _entries.find(key) != _entries.end();
    }

    std::vector<DictEntry> Dictionary::getAllEntries() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::vector<DictEntry> result;
        result.reserve(_entries.size());
        for (const auto& pair : _entries) {
            result.push_back(pair.second);
        }
        return result;
    }

    void Dictionary::clear() {
        std::lock_guard<std::mutex> lock(_mutex);
        _entries.clear();
    }

    size_t Dictionary::hash() const {
        std::lock_guard<std::mutex> lock(_mutex);
        size_t h = 0;
        std::hash<std::string> hasher;
        for (const auto& pair : _entries) {
            h ^= hasher(pair.first) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= hasher(pair.second.value) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }

    // ============================================================
    // DsDictTaskImpl Implementation
    // ============================================================

    DsDictTaskImpl::DsDictTaskImpl(const LangCore::ModuleSpec *spec)
        : _spec(spec) {
    }

    LangCore::Expected<void> DsDictTaskImpl::initialize() {
        auto cfg = LangCore::config(_spec);

        // Load dictionaries from configuration
        auto dictPaths = cfg.getObject("dictionaries");

        if (!dictPaths.empty()) {
            for (const auto& pair : dictPaths) {
                const std::string& dictId = pair.first;
                const auto& dictConfig = pair.second;

                if (dictConfig.isString()) {
                    // Simple path configuration
                    auto path = dictConfig.toString();
                    auto result = loadDictionary(dictId, path);
                    if (!result) {
                        auto err = result.takeError();
                        LOG_WARNING("Failed to load dictionary '{}': {}", dictId, err.message());
                    } else {
                        LOG_INFO("Loaded dictionary '{}' from '{}'", dictId, path);
                    }
                } else if (dictConfig.isObject()) {
                    // Advanced configuration with options
                    const auto& configObj = dictConfig.toObject();
                    auto pathIt = configObj.find("path");
                    if (pathIt != configObj.end() && pathIt->second.isString()) {
                        auto path = pathIt->second.toString();
                        auto result = loadDictionary(dictId, path);
                        if (!result) {
                            auto err = result.takeError();
                            LOG_WARNING("Failed to load dictionary '{}': {}", dictId, err.message());
                        } else {
                            LOG_INFO("Loaded dictionary '{}' from '{}'", dictId, path);
                        }
                    }
                }
            }
        }

        LOG_INFO("DsDictTaskImpl initialized with {} dictionaries", _dictionaries.size());

        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    DsDictTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        auto dictInput = LangCore::dynamic_pointer_cast<LangCore::DictInputV1>(input);
        if (!dictInput) {
            return LangCore::Error(LangCore::Error::InvalidArgument,
                                   "Invalid input type, expected DictInputV1");
        }

        return processQuery(*dictInput);
    }

    std::string DsDictTaskImpl::getConfig() const {
        return LangCore::Task::defaultConfig(_spec);
    }

    LangCore::Expected<void> DsDictTaskImpl::loadDictionary(const std::string& dictId, const std::filesystem::path& path) {
        // Check if dictionary already exists
        {
            std::lock_guard<std::mutex> lock(_dictMutex);
            if (_dictionaries.find(dictId) != _dictionaries.end()) {
                return LangCore::Error(LangCore::Error::RuntimeError,
                                       "Dictionary '" + dictId + "' already loaded");
            }
        }

        if (!std::filesystem::exists(path)) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                   "Dictionary file not found: " + path.string());
        }

        // Create new dictionary
        auto dict = std::make_shared<Dictionary>(dictId);

        // Load entries from file (assuming simple line-based format: key\tvalue)
        std::ifstream file(path);
        if (!file.is_open()) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                   "Failed to open dictionary file: " + path.string());
        }

        std::string line;
        int lineNum = 0;
        while (std::getline(file, line)) {
            lineNum++;

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#') {
                continue;
            }

            // Parse line: key\tvalue
            size_t tabPos = line.find('\t');
            if (tabPos == std::string::npos) {
                LOG_WARNING("Invalid format at line {} in '{}', skipping", lineNum, path.string());
                continue;
            }

            std::string key = line.substr(0, tabPos);
            std::string value = line.substr(tabPos + 1);

            dict->addEntry(key, value);
        }

        file.close();

        // Add to dictionaries map
        {
            std::lock_guard<std::mutex> lock(_dictMutex);
            _dictionaries[dictId] = dict;
        }

        LOG_INFO("Loaded {} entries from '{}' into dictionary '{}'", dict->size(), path.string(), dictId);

        return {};
    }

    std::shared_ptr<Dictionary> DsDictTaskImpl::getDictionary(const std::string& dictId) {
        std::lock_guard<std::mutex> lock(_dictMutex);
        auto it = _dictionaries.find(dictId);
        if (it != _dictionaries.end()) {
            return it->second;
        }
        return nullptr;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    DsDictTaskImpl::processQuery(const LangCore::DictInputV1& input) {
        auto result = LangCore::NO<LangCore::DictResV1>::create();
        result->found = true;
        result->foundCount = 0;

        auto dict = getDictionary(input.dictId);
        if (!dict) {
            LOG_WARNING("Dictionary '{}' not found", input.dictId);
            return result;
        }

        for (const auto& key : input.keys) {
            std::string value, metadata;
            if (dict->lookup(key, value, metadata)) {
                result->foundCount++;
                result->values.push_back(value);
                LOG_DEBUG("Dictionary lookup: dict='{}', key='{}', value='{}'",
                          input.dictId, key, value);
            } else {
                // Return default value if provided
                if (!input.defaultValue.empty()) {
                    result->foundCount++;
                    result->values.push_back(input.defaultValue);
                } else {
                    result->values.push_back("");
                }
                LOG_DEBUG("Dictionary lookup failed: dict='{}', key='{}'",
                          input.dictId, key);
            }
        }

        // Set found flag based on whether all keys were found
        result->found = (result->foundCount == input.keys.size());

        return result;
    }

} // namespace LangPlugins::DsDict::Internal::V1