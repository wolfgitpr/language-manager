#include "TaskImpl.h"
#include <LangCore/Support/ConfigAccessor.h>
#include <LangCore/Support/Logging.h>
#include <fstream>
#include <filesystem>

namespace LangPlugins::DsDict::Internal::V1
{
    static LangCore::LogCategory Log("DsDict");

    // Static dedup map: canonical-path -> weak_ptr<Dictionary>
    std::unordered_map<std::string, std::weak_ptr<Dictionary>> DsDictTaskImpl::s_loadedFiles;
    std::mutex DsDictTaskImpl::s_loadedFilesMutex;

    // ============================================================
    // Dictionary Implementation
    // ============================================================

    bool Dictionary::lookup(const std::string &key, std::string &value) const {
        std::shared_lock lock(_mutex);
        auto it = _entries.find(key);
        if (it != _entries.end()) {
            value = it->second.value;
            return true;
        }
        return false;
    }

    bool Dictionary::contains(const std::string &key) const {
        std::shared_lock lock(_mutex);
        return _entries.find(key) != _entries.end();
    }

    size_t Dictionary::size() const {
        std::shared_lock lock(_mutex);
        return _entries.size();
    }

    // ============================================================
    // DsDictTaskImpl Implementation
    // ============================================================

    DsDictTaskImpl::DsDictTaskImpl(const LangCore::ModuleSpec *spec)
        : _spec(spec) {
    }

    LangCore::Expected<void> DsDictTaskImpl::initialize() {
        auto cfg = LangCore::config(_spec);

        // Read the raw config JSON to iterate dictionary entries.
        // Config format:
        //   "dictionaries": {
        //     "my-dict": "relative/path/to/dict.txt",
        //     "other": { "path": "another.txt" }
        //   }
        const auto &raw = cfg.raw();
        auto dictIt = raw.find("dictionaries");
        if (dictIt == raw.end() || !dictIt->second.isObject()) {
            Log.langCoreInfo("DsDictTaskImpl: no dictionaries configured, skipping.");
            return {};
        }

        const auto &dictObj = dictIt->second.toObject();
        for (const auto &[dictId, dictConfig] : dictObj) {
            std::string pathStr;

            if (dictConfig.isString()) {
                pathStr = dictConfig.toString();
            } else if (dictConfig.isObject()) {
                const auto &obj = dictConfig.toObject();
                auto pathIt2 = obj.find("path");
                if (pathIt2 != obj.end() && pathIt2->second.isString()) {
                    pathStr = pathIt2->second.toString();
                } else {
                    Log.langCoreWarning("DsDict: dictionary '%1' has no 'path' field, skipping", dictId);
                    continue;
                }
            } else {
                Log.langCoreWarning("DsDict: dictionary '%1' has invalid config type, skipping", dictId);
                continue;
            }

            // Resolve relative to the module's base path
            auto resolvedPath = cfg.basePath() / pathStr;
            auto result = loadDictionary(dictId, resolvedPath);
            if (!result) {
                Log.langCoreWarning("DsDict: failed to load dictionary '%1': %2",
                                    dictId, result.error().message());
            }
        }

        Log.langCoreInfo("DsDictTaskImpl initialized with %1 dictionaries", _dictionaries.size());
        return {};
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    DsDictTaskImpl::start(const LangCore::NO<LangCore::TaskInput> &input) {
        auto dictInput = std::dynamic_pointer_cast<LangCore::DictInputV1>(
            static_cast<const std::shared_ptr<LangCore::TaskInput> &>(input));
        if (!dictInput) {
            return LangCore::Error(LangCore::Error::ValidationError,
                                   "Invalid input type, expected DictInputV1");
        }

        return processQuery(*dictInput);
    }

    std::string DsDictTaskImpl::getConfig() const {
        // Delegate to the Task base loadConfig mechanism via the spec.
        // Return empty string if no config is available.
        return {};
    }

    LangCore::Expected<void> DsDictTaskImpl::loadDictionary(const std::string &dictId,
                                                             const std::filesystem::path &path) {
        // Check for duplicate dictId
        {
            std::shared_lock lock(_dictMutex);
            if (_dictionaries.find(dictId) != _dictionaries.end()) {
                return LangCore::Error(LangCore::Error::RuntimeError,
                                       "Dictionary '" + dictId + "' already loaded");
            }
        }

        // Resolve canonical path for deduplication
        std::error_code ec;
        auto canonical = std::filesystem::canonical(path, ec);
        if (ec) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                   "Dictionary file not found: " + path.string());
        }
        std::string canonicalStr = canonical.string();

        // Try to reuse an already-loaded dictionary for the same file
        {
            std::lock_guard fileLock(s_loadedFilesMutex);
            auto it = s_loadedFiles.find(canonicalStr);
            if (it != s_loadedFiles.end()) {
                if (auto existing = it->second.lock()) {
                    // Reuse: same physical file already parsed
                    std::unique_lock lock(_dictMutex);
                    _dictionaries[dictId] = existing;
                    Log.langCoreInfo("DsDict: reusing already-loaded file '%1' for dict '%2' (dedup)",
                                     canonicalStr, dictId);
                    return {};
                }
                // Weak pointer expired, remove stale entry
                s_loadedFiles.erase(it);
            }
        }

        // Load from file
        std::ifstream file(canonical, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                   "Failed to open dictionary file: " + canonical.string());
        }

        auto dict = std::make_shared<Dictionary>(dictId, canonicalStr);

        // Read the entire file into memory and parse
        file.seekg(0, std::ios::end);
        const auto fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        std::string buf(static_cast<size_t>(fileSize), '\0');
        if (!file.read(buf.data(), fileSize)) {
            return LangCore::Error(LangCore::Error::FileSystemError,
                                   "Failed to read dictionary file: " + canonical.string());
        }

        // Pre-allocate: estimate line count for large files
        if (fileSize > 1024 * 1024) {
            size_t lineCount = std::count(buf.begin(), buf.end(), '\n') + 1;
            dict->_entries.reserve(lineCount);
        }

        // Parse lines: key\tvalue
        size_t pos = 0;
        int lineNum = 0;
        while (pos < buf.size()) {
            lineNum++;

            // Find end of line
            size_t eol = buf.find_first_of("\r\n", pos);
            if (eol == std::string::npos)
                eol = buf.size();

            // Skip empty lines and comments
            if (eol == pos || buf[pos] == '#') {
                pos = (eol < buf.size() && buf[eol] == '\r' && eol + 1 < buf.size() && buf[eol + 1] == '\n')
                          ? eol + 2
                          : eol + 1;
                continue;
            }

            // Find tab separator
            size_t tabPos = buf.find('\t', pos);
            if (tabPos == std::string::npos || tabPos >= eol) {
                Log.langCoreWarning("DsDict: invalid format at line %1, skipping", lineNum);
                pos = (eol < buf.size() && buf[eol] == '\r' && eol + 1 < buf.size() && buf[eol + 1] == '\n')
                          ? eol + 2
                          : eol + 1;
                continue;
            }

            std::string key = buf.substr(pos, tabPos - pos);
            std::string value = buf.substr(tabPos + 1, eol - tabPos - 1);

            dict->_entries.emplace(std::move(key), DictEntry(std::move(value)));

            pos = (eol < buf.size() && buf[eol] == '\r' && eol + 1 < buf.size() && buf[eol + 1] == '\n')
                      ? eol + 2
                      : eol + 1;
        }

        // Register in instance map and global dedup map
        {
            std::unique_lock lock(_dictMutex);
            _dictionaries[dictId] = dict;
        }
        {
            std::lock_guard fileLock(s_loadedFilesMutex);
            s_loadedFiles[canonicalStr] = dict;
        }

        Log.langCoreInfo("DsDict: loaded %1 entries from '%2' as dict '%3'",
                         dict->size(), canonical.filename().string(), dictId);
        return {};
    }

    std::shared_ptr<Dictionary> DsDictTaskImpl::getDictionary(const std::string &dictId) const {
        std::shared_lock lock(_dictMutex);
        auto it = _dictionaries.find(dictId);
        if (it != _dictionaries.end()) {
            return it->second;
        }
        return nullptr;
    }

    LangCore::Expected<LangCore::NO<LangCore::TaskResult>>
    DsDictTaskImpl::processQuery(const LangCore::DictInputV1 &input) const {
        auto result = LangCore::NO<LangCore::DictResV1>::create();
        result->found = false;
        result->foundCount = 0;

        auto dict = getDictionary(input.dictId);
        if (!dict) {
            Log.langCoreWarning("DsDict: dictionary '%1' not found", input.dictId);
            // Return empty results for all keys
            result->values.resize(input.keys.size());
            return result;
        }

        result->values.reserve(input.keys.size());
        for (const auto &key : input.keys) {
            std::string value;
            if (dict->lookup(key, value)) {
                result->foundCount++;
                result->values.push_back(std::move(value));
            } else if (!input.defaultValue.empty()) {
                result->values.push_back(input.defaultValue);
            } else {
                result->values.emplace_back();
            }
        }

        result->found = (result->foundCount == input.keys.size());
        return result;
    }

} // namespace LangPlugins::DsDict::Internal::V1
