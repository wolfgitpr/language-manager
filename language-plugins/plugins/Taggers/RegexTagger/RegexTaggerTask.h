#ifndef LANG_PLUGINS_REGEXTAGGERTASK_H
#define LANG_PLUGINS_REGEXTAGGERTASK_H

#include <LangMgr/Task/Task.h>
#include <LangPlugins/Core/Tensor.h>

#include <LangPlugins/Api/Common/1/CommonApiL1.h>
#include <LangPlugins/Api/Drivers/Onnx/1/OnnxDriverApiL1.h>
#include <LangPlugins/Api/Taggers/RegexTagger/1/RegexTaggerL1.h>

namespace LangPlugins
{
    namespace Regex = Api::RegexTagger::L1;
    namespace Common = Api::Common::L1;
    namespace Onnx = Api::Onnx::L1;

    class RegexTaggerTask : public LangMgr::Task {
    public:
        explicit RegexTaggerTask(const LangMgr::ModuleSpec *spec);
        ~RegexTaggerTask() override;

        LangMgr::Expected<void> initialize(const LangMgr::NO<LangMgr::TaskInitArgs> &args) override;

        LangMgr::Expected<LangMgr::NO<LangMgr::TaskResult>>
        start(const LangMgr::NO<LangMgr::TaskStartInput> &input) override;
        LangMgr::Expected<void> startAsync(const LangMgr::NO<LangMgr::TaskStartInput> &input,
                                           const StartAsyncCallback &callback) override;
        bool stop() override;

        LangMgr::NO<LangMgr::TaskResult> result() const override;

    protected:
        class Impl;
        std::unique_ptr<Impl> _impl;

    private:
        std::vector<std::string> lookup(const std::string &key) const;
    };

} // namespace LangPlugins

#endif //  LANG_PLUGINS_REGEXTAGGERTASK_H
