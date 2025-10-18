#ifndef ILANGUAGEFACTORY_P_H
#define ILANGUAGEFACTORY_P_H

#include "IG2pFactory.h"

namespace LangMgr
{

    class ILanguageFactoryPrivate {
    public:
        ILanguageFactoryPrivate();
        ~ILanguageFactoryPrivate();

        static void init();

        ILanguageFactory *q_ptr;

        std::string id;
        std::string displayName;

        bool enabled = true;
        bool discardResult = false;
    };

} // namespace LangMgr

#endif // ILANGUAGEFACTORY_P_H
