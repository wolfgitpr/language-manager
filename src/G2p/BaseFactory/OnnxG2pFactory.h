#ifndef LANGUAGE_MANAGER_ONNXG2PFACTORY_H
#define LANGUAGE_MANAGER_ONNXG2PFACTORY_H

#include "G2pDriver.h"
#include "G2pModel.h"
#include "language-manager/IG2pFactory.h"

namespace LangMgr
{
    class LANG_MANAGER_EXPORT OnnxG2pFactory : public IG2pFactory {
        Q_OBJECT
    public:
        explicit OnnxG2pFactory(const QString &id, QObject *parent = nullptr);
        ~OnnxG2pFactory() override;

        QList<LangNote> convert(const QStringList &input) const override;

        bool is_open() const;

        void terminate() const;

    private:
        G2pModel *m_g2p;
        G2pDriver *m_driver;
    };

} // namespace LangMgr

#endif // LANGUAGE_MANAGER_ONNXG2PFACTORY_H
