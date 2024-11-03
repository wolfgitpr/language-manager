#include <language-manager/IG2pFactory.h>
#include "IG2pFactory_p.h"

#include <QJsonObject>

namespace LangMgr
{

    IG2pFactoryPrivate::IG2pFactoryPrivate() {}

    IG2pFactoryPrivate::~IG2pFactoryPrivate() = default;

    void IG2pFactoryPrivate::init() {}

    IG2pFactory::IG2pFactory(const QString &id, const QString &categroy, QObject *parent) :
        IG2pFactory(*new IG2pFactoryPrivate(), id, categroy, parent) {}

    IG2pFactory::~IG2pFactory() = default;

    IG2pFactory::IG2pFactory(IG2pFactoryPrivate &d, const QString &id, const QString &categroy, QObject *parent) :
        QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.id = id;
        d.categroy = categroy.isEmpty() ? id : categroy;

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

    QString IG2pFactory::randString() const {
        // TODO
        return m_langFactory.first()->randString();
    }

    QJsonObject IG2pFactory::defaultConfig() {
        Q_D(IG2pFactory);
        QJsonObject config;
        config.insert("id", id());
        config.insert("languageConfig", languageDefaultConfig());
        return config;
    }

    QJsonObject IG2pFactory::languageDefaultConfig() {
        Q_D(IG2pFactory);
        QJsonObject config;
        for (const auto &langfactory : m_langFactory) {
            config.insert(langfactory->id(), langfactory->exportConfig());
        }
        return config;
    }

    QJsonObject IG2pFactory::config() {
        Q_D(IG2pFactory);
        if (m_config->empty())
            return defaultConfig();
        return *m_config;
    }

    void IG2pFactory::loadG2pConfig(const QJsonObject &config) { Q_UNUSED(config); }

    QJsonObject IG2pFactory::languageConfig() const {
        QJsonObject config;
        for (const auto &langfactory : m_langFactory) {
            config.insert(langfactory->id(), langfactory->exportConfig());
        }
        return config;
    }

    void IG2pFactory::loadLanguageConfig(const QJsonObject &config) {
        Q_D(const IG2pFactory);
        QJsonObject configObj = this->config();

        for (const auto &langfactory : m_langFactory) {
            langfactory->loadConfig(config.value(langfactory->id()).toObject());
        }

        configObj.insert("languageConfig", languageConfig());
        *m_config = configObj;
    }

    QString IG2pFactory::analysis(const QString &input) const {
        for (const auto &factory : m_langFactory) {
            const auto result = factory->analysis(input);
            if (result != "unknown")
                return result;
        }
        return "unknown";
    }

    QList<LangNote> IG2pFactory::split(const QString &input) const {
        Q_D(const IG2pFactory);
        return split({LangNote(input)});
    }

    QList<LangNote> IG2pFactory::split(const QList<LangNote> &input) const {
        Q_D(const IG2pFactory);

        QList<LangNote> result = input;
        for (const auto &factory : m_langFactory) {
            result = factory->split(result, d->id);
        }
        return result;
    }

    void IG2pFactory::correct(const QList<LangNote *> &input) const {
        Q_D(const IG2pFactory);
        for (const auto &factory : m_langFactory) {
            factory->correct(input, d->id);
        }
    }

    QList<LangNote> IG2pFactory::convert(const QStringList &input) const {
        Q_UNUSED(input);
        return {};
    }
} // namespace LangMgr
