#include "inferutil/Driver.h"

#include <stdcorelib/str.h>

#include <LangMgr/Tool/InferenceContrib.h>
#include <LangPlugins/Api/Drivers/Onnx/OnnxDriverApi.h>

namespace LangPlugins::inferUtil
{
    LangMgr::Expected<LangMgr::NO<InferenceDriver>> getInferenceDriver(const LangMgr::Inference *obj) {
        namespace Onnx = Api::Onnx;

        const auto inferenceCate = obj->spec()->SU()->category("inference");
        const auto dsdriverObject = inferenceCate->getFirstObject("g2pOnnxDriver");

        if (!dsdriverObject) {
            return LangMgr::Error(LangMgr::Error::SessionError, "could not find dsdriver");
        }

        auto onnxDriver = dsdriverObject.as<InferenceDriver>();

        const auto arch = onnxDriver->arch();
        // TODO: expectedArch
        constexpr auto expectedArch = "onnx";
        const bool isArchMatch = arch == expectedArch;

        const auto backend = onnxDriver->backend();
        constexpr auto expectedBackend = Onnx::API_NAME;

        if (const bool isBackendMatch = backend == expectedBackend; !isArchMatch || !isBackendMatch) {
            return LangMgr::Error(
                LangMgr::Error::SessionError,
                stdc::formatN(
                    R"(invalid driver: expected arch "%1", got "%2" (%3); expected backend "%4", got "%5" (%6))",
                    expectedArch, arch, isArchMatch ? "match" : "MISMATCH", expectedBackend, backend,
                    isBackendMatch ? "match" : "MISMATCH"));
        }

        return onnxDriver;
    }
} // namespace LangPlugins::inferUtil
