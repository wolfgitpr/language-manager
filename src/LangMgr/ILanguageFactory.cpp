#include <language-manager/ILanguageFactory.h>
#include "ILanguageFactory_p.h"

#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <memory>

#include <language-manager/IG2pManager.h>

namespace LangMgr
{

    ILanguageFactoryPrivate::ILanguageFactoryPrivate() {}

    ILanguageFactoryPrivate::~ILanguageFactoryPrivate() = default;

    void ILanguageFactoryPrivate::init() {}

    ILanguageFactory::ILanguageFactory(const QString &id, QObject *parent) :
        ILanguageFactory(*new ILanguageFactoryPrivate(), id, parent) {}

    ILanguageFactory::~ILanguageFactory() = default;

    bool ILanguageFactory::initialize(QString &errMsg) {
        Q_UNUSED(errMsg);
        return true;
    }

    ILanguageFactory::ILanguageFactory(ILanguageFactoryPrivate &d, const QString &id, QObject *parent) :
        QObject(parent), d_ptr(&d) {
        d.q_ptr = this;
        d.id = id;
        d.m_selectedG2p = id;
        d.categroy = id;

        d.init();
    }

    QString ILanguageFactory::id() const {
        Q_D(const ILanguageFactory);
        return d->id;
    }

    QString ILanguageFactory::displayName() const {
        Q_D(const ILanguageFactory);
        return d->displayName;
    }

    void ILanguageFactory::setDisplayName(const QString &name) {
        Q_D(ILanguageFactory);
        d->displayName = name;
    }

    QString ILanguageFactory::category() const {
        Q_D(const ILanguageFactory);
        return d->categroy;
    }

    void ILanguageFactory::setCategory(const QString &category) {
        Q_D(ILanguageFactory);
        d->categroy = category;
    }

    QString ILanguageFactory::selectedG2p() const {
        Q_D(const ILanguageFactory);
        return d->m_selectedG2p;
    }

    void ILanguageFactory::setG2p(const QString &g2pId) {
        Q_D(ILanguageFactory);
        d->m_selectedG2p = g2pId;
    }

    bool ILanguageFactory::enabled() const {
        Q_D(const ILanguageFactory);
        return d->enabled;
    }

    void ILanguageFactory::setEnabled(const bool &enable) {
        Q_D(ILanguageFactory);
        d->enabled = enable;
    }

    bool ILanguageFactory::discardResult() const {
        Q_D(const ILanguageFactory);
        return d->discardResult;
    }

    void ILanguageFactory::setDiscardResult(const bool &discard) {
        Q_D(ILanguageFactory);
        d->discardResult = discard;
    }

    QString ILanguageFactory::description() const {
        Q_D(const ILanguageFactory);
        return d->description;
    }

    void ILanguageFactory::setDescription(const QString &description) {
        Q_D(ILanguageFactory);
        d->description = description;
    }

    QString ILanguageFactory::author() const {
        Q_D(const ILanguageFactory);
        return d->author;
    }

    void ILanguageFactory::setAuthor(const QString &author) {
        Q_D(ILanguageFactory);
        d->author = author;
    }

    QString ILanguageFactory::randString() const { return {}; }

    bool ILanguageFactory::contains(const QChar &c) const {
        Q_UNUSED(c);
        return false;
    }

    bool ILanguageFactory::contains(const QString &input) const {
        Q_UNUSED(input);
        return false;
    }

    QList<LangNote> ILanguageFactory::split(const QString &input, const QString &g2pId) const {
        Q_UNUSED(input);
        Q_UNUSED(g2pId)
        return {};
    }

    QList<LangNote> ILanguageFactory::split(const QList<LangNote> &input, const QString &g2pId) const {
        Q_D(const ILanguageFactory);
        if (!d->enabled) {
            return input;
        }

        QList<LangNote> result;
        for (const auto &note : input) {
            if (note.language == "Unknown" || note.language == "unknown" || note.language == "") {
                const auto splitRes = split(note.lyric, g2pId);
                for (const auto &res : splitRes) {
                    if (res.language == id() && d->discardResult) {
                        continue;
                    }
                    result.append(res);
                }
            } else {
                result.append(note);
            }
        }
        return result;
    }

    QString ILanguageFactory::analysis(const QString &input) const { return contains(input) ? id() : "unknown"; }

    void ILanguageFactory::correct(const QList<LangNote *> &input, const QString &g2pId) const {
        for (const auto &note : input) {
            if (note->language == "unknown") {
                if (contains(note->lyric)) {
                    note->language = id();
                    note->g2pId = g2pId;
                }
            }
        }
    }

    void ILanguageFactory::loadConfig(const QJsonObject &config) {
        Q_D(ILanguageFactory);
        if (config.contains("enabled"))
            d->enabled = config.value("enabled").toBool();
        if (config.contains("discardResult"))
            d->discardResult = config.value("discardResult").toBool();
        if (config.contains("category"))
            d->categroy = config.value("category").toString();
        if (config.contains("selectedG2p"))
            d->m_selectedG2p = config.value("selectedG2p").toString();
    }

    QJsonObject ILanguageFactory::exportConfig() const {
        Q_D(const ILanguageFactory);
        QJsonObject config;
        config.insert("enabled", d->enabled);
        config.insert("discardResult", d->discardResult);
        config.insert("category", d->categroy);
        config.insert("selectedG2p", d->m_selectedG2p);
        return config;
    }

} // namespace LangMgr
