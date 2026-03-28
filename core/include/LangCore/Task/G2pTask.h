#ifndef LANGCORE_G2PTASK_H
#define LANGCORE_G2PTASK_H

#include <filesystem>

#include <LangCore/Base/LangCommon.h>
#include <LangCore/Task/Task.h>

namespace LangCore
{
    class G2pInputV1 : public TaskInput {
    public:
        G2pInputV1() {}

        std::vector<std::string> g2pInput;
    };

    class G2pResultV1 : public TaskResult {
    public:
        G2pResultV1() {}

        std::vector<G2pRes> g2pResult;
        std::string errorMessage;
    };
} // namespace LangCore
#endif
