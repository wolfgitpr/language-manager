#ifndef LANGCORE_OBJECTPOOL_H
#define LANGCORE_OBJECTPOOL_H

#include <string_view>
#include <vector>

#include <LangCore/Base/NamedObject.h>
#include <LangCore/LangCoreGlobal.h>

namespace LangCore
{

    class LANGCORE_EXPORT ObjectPool : public NamedObject {
    public:
        explicit ObjectPool();
        ~ObjectPool() override;

        void addObject(const NO<NamedObject> &obj);
        void addObject(std::string_view id, const NO<NamedObject> &obj);
        void addObjects(const std::string_view id, const stdc::array_view<NO<NamedObject>> objs) {
            for (const auto &obj : objs) {
                addObject(id, obj);
            }
        }
        void removeObject(const NamedObject *obj);
        void removeObject(std::string_view id, const NamedObject *obj);
        void removeObjects(std::string_view id);
        void removeAllObjects();

        std::vector<NO<NamedObject>> allObjects() const;
        std::vector<NO<NamedObject>> getObjects(std::string_view id) const;
        NO<NamedObject> getFirstObject(std::string_view id) const;

    protected:
        virtual void objectAdded(std::string_view id, const NO<NamedObject> &obj);
        virtual void aboutToRemoveObject(std::string_view id, const NO<NamedObject> &obj);

        class Impl;
        explicit ObjectPool(Impl &impl);
    };
} // namespace LangCore
#endif // LANGCORE_OBJECTPOOL_H
