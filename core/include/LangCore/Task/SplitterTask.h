#ifndef LANGCORE_SPLITTERTASK_H
#define LANGCORE_SPLITTERTASK_H

#include <filesystem>

#include <LangCore/Task/Task.h>

namespace LangCore
{
    class SplitterInputV1 : public TaskInput {
    public:
        SplitterInputV1() {}

        std::vector<std::string> splitterInput;
    };

    class SplitterResultV1 : public TaskResult {
    public:
        SplitterResultV1() {}

        std::vector<std::string> splitterResult;
    };
} // namespace LangCore
#endif
