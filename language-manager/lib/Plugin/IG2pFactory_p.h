#ifndef IG2PFACTORYPRIVATE_H
#define IG2PFACTORYPRIVATE_H

#include "IG2pFactory.h"

namespace LangMgr
{
    class IG2pFactoryPrivate {
    public:
        IG2pFactoryPrivate();
        ~IG2pFactoryPrivate();

        static void init();

        IG2pFactory *q_ptr;

        std::string id;
        bool base = true;
        std::string displayName;
        std::string categroy;
        std::string author;
        std::string description;
    };

} // namespace LangMgr

#endif // IG2PFACTORYPRIVATE_H
