#ifndef LANGPLUGINS_INFERUTIL_DRIVER_H
#define LANGPLUGINS_INFERUTIL_DRIVER_H

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Tool/Inference.h>
#include <LangPlugins/Inference/InferenceDriver.h>

namespace LangPlugins::inferUtil
{
    LangMgr::Expected<LangMgr::NO<InferenceDriver>> getInferenceDriver(const LangMgr::Inference *obj);
}

#endif // LANGPLUGINS_INFERUTIL_DRIVER_H
