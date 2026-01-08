#ifndef LANGMGR_ENGINEFACTORYPLUGIN_H
#define LANGMGR_ENGINEFACTORYPLUGIN_H

#include <LangMgr/Modules/EngineFactory.h>
#include <LangMgr/Plugin/Plugin.h>

namespace LangMgr
{

    class EngineFactoryPlugin : public Plugin {
    public:
        EngineFactoryPlugin() = default;
        ~EngineFactoryPlugin() override;

        const char *iid() const override { return "org.openvpi.EngineFactory"; }

        virtual NO<EngineFactory> create() = 0;

        STDCORELIB_DISABLE_COPY(EngineFactoryPlugin)
    };
    inline EngineFactoryPlugin::~EngineFactoryPlugin() = default;

} // namespace LangMgr

#endif // LANGMGR_ENGINEFACTORYPLUGIN_H
