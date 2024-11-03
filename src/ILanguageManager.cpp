#include <language-manager/ILanguageManager.h>
#include "ILanguageManager_p.h"

#include <language-manager/ILanguageFactory.h>

#include "G2p/BaseG2p/LinebreakG2p.h"
#include "G2p/BaseG2p/NumberG2p.h"
#include "G2p/BaseG2p/PunctuationG2p.h"
#include "G2p/BaseG2p/SlurG2p.h"
#include "G2p/BaseG2p/SpaceG2p.h"
#include "G2p/BaseG2p/UnknownG2p.h"

#include "G2p/CantoneseG2p.h"
#include "G2p/EnglishG2p.h"
#include "G2p/JapaneseG2p.h"
#include "G2p/MandarinG2p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>
#include <cpp-pinyin/G2pglobal.h>

namespace LangMgr
{
    static QPair<QString, QString> extractConfig(const QString &g2pId) {
        const auto firstColonIndex = g2pId.indexOf(':');

        if (firstColonIndex == -1) {
            return {g2pId, "0"};
        }

        QString beforeColon = g2pId.left(firstColonIndex);
        QString afterColon = g2pId.mid(firstColonIndex + 1);

        bool isNumber;
        const int value = afterColon.toInt(&isNumber);

        if (!isNumber || value < 0) {
            return {beforeColon, "0"};
        }

        return {beforeColon, afterColon};
    }

    ILanguageManagerPrivate::ILanguageManagerPrivate() {}

    ILanguageManagerPrivate::~ILanguageManagerPrivate() = default;

    IG2pFactory *ILanguageManager::g2p(const QString &id) const {
        Q_D(const ILanguageManager);
        const auto [g2pId, configId] = extractConfig(id);
        const auto it = d->g2ps.find(g2pId);
        if (it == d->g2ps.end()) {
            qWarning() << "LangMgr::ILanguageManager::g2p(): factory does not exist:" << g2pId;
            return nullptr;
        }

        const auto config = it.value()->allConfig().value(configId).toObject();
        if (config.empty()) {
            qWarning() << "LangMgr::ILanguageManager::g2p(): config does not exist:" << g2pId << configId;
            return nullptr;
        }
        return it.value();
    }

    QList<IG2pFactory *> ILanguageManager::g2ps() const {
        Q_D(const ILanguageManager);
        return d->g2ps.values();
    }

    bool ILanguageManager::addG2p(IG2pFactory *factory) {
        Q_D(ILanguageManager);
        if (!factory) {
            qWarning() << "LangMgr::ILanguageManager::addG2p(): trying to add null factory";
            return false;
        }
        if (d->g2ps.contains(factory->id())) {
            qWarning() << "LangMgr::ILanguageManager::addG2p(): trying to add duplicated factory:" << factory->id();
            return false;
        }
        factory->setParent(this);
        d->g2ps[factory->id()] = factory;
        return true;
    }

    bool ILanguageManager::removeG2p(const IG2pFactory *factory) {
        if (factory == nullptr) {
            qWarning() << "LangMgr::ILanguageManager::removeG2p(): trying to remove null factory";
            return false;
        }
        return removeG2p(factory->id());
    }

    bool ILanguageManager::removeG2p(const QString &id) {
        Q_D(ILanguageManager);
        const auto it = d->g2ps.find(id);
        if (it == d->g2ps.end()) {
            qWarning() << "LangMgr::ILanguageManager::removeG2p(): factory does not exist:" << id;
            return false;
        }
        it.value()->setParent(nullptr);
        d->g2ps.erase(it);
        return true;
    }

    void ILanguageManager::clearG2ps() {
        Q_D(ILanguageManager);
        d->g2ps.clear();
    }

    QList<G2p> ILanguageManagerPrivate::priorityG2ps(const QStringList &priorityG2pIds) const {
        Q_Q(const ILanguageManager);
        QStringList order = this->defaultOrder;
        const auto &g2pMgr = LangMgr::ILanguageManager::instance();

        QList<G2p> result;
        // 遍历高优先级g2p绑定语种
        for (const auto &g2pId : priorityG2pIds) {
            const auto &g2p = g2pMgr->g2p(g2pId);
            if (g2p == nullptr)
                continue;
            const auto [g2pType, configId] = extractConfig(g2pId);
            result.append({g2p, configId});
        }

        for (const auto &id : order) {
            const auto &factory = q->g2p(id);
            bool add = true;
            for (const auto &[g2p, confgId] : result) {
                if (g2p == factory) {
                    add = false;
                    break;
                }
            }
            if (add)
                result.append({factory, "0"});
        }
        return result;
    }

    QStringList ILanguageManager::defaultOrder() const {
        Q_D(const ILanguageManager);
        return d->defaultOrder;
    }

    void ILanguageManager::setDefaultOrder(const QStringList &order) {
        Q_D(ILanguageManager);
        d->defaultOrder = order;
    }

    QList<LangNote> ILanguageManager::split(const QString &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityG2ps(priorityG2pIds);
        QList<LangNote> result = {LangNote(input)};
        for (const auto &[analysis, g2pConfig] : analysisers) {
            result = analysis->split(result, g2pConfig);
        }
        return result;
    }

    void ILanguageManager::correct(const QList<LangNote *> &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityG2ps(priorityG2pIds);
        for (const auto &[analysis, g2pConfig] : analysisers) {
            analysis->correct(input, g2pConfig);
        }
    }

    QString ILanguageManager::analysis(const QString &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        QString result = "unknown";
        const auto &analysisers = d->priorityG2ps(priorityG2pIds);

        for (const auto &[analysis, g2pConfig] : analysisers) {
            result = analysis->analysis(input, g2pConfig);
            if (result != "unknown")
                break;
        }

        return result;
    }

    QStringList ILanguageManager::analysis(const QStringList &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityG2ps(priorityG2pIds);
        QList<LangNote *> inputNote;
        for (const auto &lyric : input) {
            inputNote.append(new LangNote(lyric));
        }

        for (const auto &[analysis, g2pConfig] : analysisers) {
            analysis->correct(inputNote, g2pConfig);
        }

        QStringList result;
        for (const auto &note : inputNote)
            result.append(note->g2pId);
        return result;
    }

    ILanguageManager::ILanguageManager(QObject *parent) : ILanguageManager(*new ILanguageManagerPrivate(), parent) {}

    ILanguageManager::~ILanguageManager() = default;

    ILanguageManager *ILanguageManager::instance() {
        static ILanguageManager obj;
        return &obj;
    }

    bool ILanguageManager::initialize(const QJsonObject &args, QString &errMsg) {
        Q_D(ILanguageManager);
        const QString pinyinDictPath = args.value("pinyinDictPath").toString();
        Pinyin::setDictionaryPath(pinyinDictPath.toUtf8().toStdString());
        const auto g2ps = d->g2ps.values();
        for (const auto g2p : g2ps) {
            g2p->initialize(errMsg);
            if (!errMsg.isEmpty()) {
                return false;
            }
        }
        d->initialized = true;
        return true;
    }

    bool ILanguageManager::initialized() {
        Q_D(const ILanguageManager);
        return d->initialized;
    }

    ILanguageManager::ILanguageManager(ILanguageManagerPrivate &d, QObject *parent) : QObject(parent), d_ptr(&d) {
        d.q_ptr = this;

        const QString locale = QLocale::system().name();
        auto *translator = new QTranslator(this);
        if (translator->load(QString(":/share/translations/language-manager_%1.qm").arg(locale))) {
            qDebug() << "ILanguageManager: Loaded translation from resources:"
                     << QString(":/share/translations/language-manager_%1.qm").arg(locale);
        } else {
            qWarning() << "ILanguageManager: Failed to load translation";
        }
        // TODO: Install translator
        // QCoreApplication::installTranslator(translator);

        addG2p(new LinebreakG2p());
        addG2p(new NumberG2p());
        addG2p(new PunctuationG2p());
        addG2p(new SlurG2p());
        addG2p(new SpaceG2p());
        addG2p(new UnknownG2p());

        addG2p(new MandarinG2p());
        addG2p(new CantoneseG2p());
        addG2p(new JapaneseG2p());
        addG2p(new EnglishG2p());
    }

    void ILanguageManager::convert(const QList<LangNote *> &input) const {
        QMap<QString, QList<int>> indexMap;
        QMap<QString, QStringList> lyricMap;

        for (int i = 0; i < input.size(); ++i) {
            const LangNote *note = input.at(i);
            indexMap[note->g2pId].append(i);
            lyricMap[note->g2pId].append(note->lyric);
        }

        const auto &g2pIds = indexMap.keys();
        for (const auto &g2pId : g2pIds) {
            const auto &rawLyrics = lyricMap[g2pId];
            auto [g2pType, configId] = extractConfig(g2pId);

            auto g2pFactory = this->g2p(g2pId);
            if (g2pFactory == nullptr) {
                g2pFactory = this->g2p("unknown");
                configId = "0";
            }

            const auto &tempRes = g2pFactory->convert(rawLyrics, configId);
            for (int i = 0; i < tempRes.size(); i++) {
                const auto &index = indexMap[g2pId][i];
                input[index]->error = tempRes[i].error;
                input[index]->syllable = tempRes[i].syllable;
                input[index]->candidates = tempRes[i].candidates;
            }
        }
    }

} // namespace LangMgr
