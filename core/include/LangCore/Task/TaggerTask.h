#ifndef LANGCORE_TAGGERTASK_H
#define LANGCORE_TAGGERTASK_H

#include <vector>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    class TaggerInputV1 : public TaskInput {
    public:
        TaggerInputV1() {}

        std::vector<TaggerRes> taggerInput;
    };

    class TaggerResultV1 : public TaskResult {
    public:
        TaggerResultV1() {}

        std::vector<TaggerRes> taggerResult;
        std::string errorMessage;
    };
} // namespace LangCore
#endif // LANGCORE_TAGGERTASK_H
