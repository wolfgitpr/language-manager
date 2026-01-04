#ifndef LANGPLUGINS_INFERUTIL_DRIVER_H
#define LANGPLUGINS_INFERUTIL_DRIVER_H

#include <LangMgr/Support/Expected.h>
#include <LangMgr/Tool/Inference.h>
#include <LangPlugins/Inference/InferenceDriver.h>

namespace LangPlugins::inferUtil
{
    LangMgr::Expected<LangMgr::NO<LangMgr::NamedObject>> getInferenceObject(const LangMgr::Inference *obj,
                                                                            const std::string &id);
    LangMgr::Expected<LangMgr::NO<InferenceDriver>> getInferenceDriver(const LangMgr::Inference *obj);
} // namespace LangPlugins::inferUtil

#endif // LANGPLUGINS_INFERUTIL_DRIVER_H
