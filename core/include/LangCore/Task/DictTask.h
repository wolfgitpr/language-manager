#ifndef LANGCORE_DICTTASK_H
#define LANGCORE_DICTTASK_H

#include <string>
#include <vector>

#include <LangCore/Task/Task.h>

namespace LangCore
{
    /// DictInputV1 - Dictionary query input for V1 API
    class DictInputV1 : public TaskInput {
    public:
        DictInputV1() {}

        /// Dictionary ID to query
        std::string dictId;

        /// Keys to look up in the dictionary (can be single key or multiple keys)
        std::vector<std::string> keys;

        /// Optional default value if key not found
        std::string defaultValue;

        /// Optional flags for query behavior
        uint32_t flags = 0;
    };

    /// DictResV1 - Dictionary query result for V1 API
    class DictResV1 : public TaskResult {
    public:
        DictResV1() {}

        /// Query result values (for each key in input)
        std::vector<std::string> values;

        /// Whether all keys were found
        bool found = false;

        /// Number of keys found
        size_t foundCount = 0;
    };

} // namespace LangCore

#endif // LANGCORE_DICTTASK_H

