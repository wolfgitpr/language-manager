#include <language-manager/ILanguageFactory.h>
#include "IG2pFactory_p.h"

#include <QJsonObject>

namespace LangMgr
{

    IG2pFactoryPrivate::IG2pFactoryPrivate() {}

    IG2pFactoryPrivate::~IG2pFactoryPrivate() = default;

    void IG2pFactoryPrivate::init() {}

    IG2pFactory::IG2pFactory(const QString &id, QObject *parent) : IG2pFactory(*new IG2pFactoryPrivate(), id, parent) {}

    IG2pFactory::~IG2pFactory() = default;

    IG2pFactory::IG2pFactory(IG2pFactoryPrivate &d, const QString &id, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.id = id;
        d.categroy = id;

        d.init();
    }

    bool IG2pFactory::initialize(QString &errMsg) {
        Q_UNUSED(errMsg);
        return true;
    }

    QString IG2pFactory::id() const {
        Q_D(const IG2pFactory);
        return d->id;
    }

    QString IG2pFactory::displayName() const {
        Q_D(const IG2pFactory);
        return d->displayName;
    }

    void IG2pFactory::setDisplayName(const QString &displayName) {
        Q_D(IG2pFactory);
        d->displayName = displayName;
    }

    QString IG2pFactory::category() const {
        Q_D(const IG2pFactory);
        return d->categroy;
    }

    void IG2pFactory::setCategory(const QString &category) {
        Q_D(IG2pFactory);
        d->categroy = category;
    }

    QString IG2pFactory::author() const {
        Q_D(const IG2pFactory);
        return d->author;
    }

    void IG2pFactory::setAuthor(const QString &author) {
        Q_D(IG2pFactory);
        d->author = author;
    }

    QString IG2pFactory::description() const {
        Q_D(const IG2pFactory);
        return d->description;
    }

    void IG2pFactory::setDescription(const QString &description) {
        Q_D(IG2pFactory);
        d->description = description;
    }

    QJsonObject IG2pFactory::config() { return {}; }

    void IG2pFactory::loadConfig(const QJsonObject &config) { Q_UNUSED(config); }

    LangNote IG2pFactory::convert(const QString &input) const { return convert(QStringList() << input).at(0); }

    QList<LangNote> IG2pFactory::convert(const QStringList &input) const {
        Q_UNUSED(input);
        return {};
    }
} // namespace LangMgr
