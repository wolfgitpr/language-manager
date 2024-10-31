#include <language-manager/ILanguageManager.h>
#include "ILanguageManager_p.h"

#include <language-manager/ILanguageFactory.h>

#include "LangAnalysis/BaseAnalysis/LinebreakAnalysis.h"
#include "LangAnalysis/BaseAnalysis/NumberAnalysis.h"
#include "LangAnalysis/BaseAnalysis/Punctuation.h"
#include "LangAnalysis/BaseAnalysis/SlurAnalysis.h"
#include "LangAnalysis/BaseAnalysis/SpaceAnalysis.h"
#include "LangAnalysis/BaseAnalysis/UnknownAnalysis.h"

#include "LangAnalysis/CantoneseAnalysis.h"
#include "LangAnalysis/EnglishAnalysis.h"
#include "LangAnalysis/JyutpingAnalysis.h"
#include "LangAnalysis/KanaAnalysis.h"
#include "LangAnalysis/MandarinAnalysis.h"
#include "LangAnalysis/PinyinAnalysis.h"
#include "LangAnalysis/RomajiAnalysis.h"

#include <language-manager/IG2pManager.h>

#include <QCoreApplication>
#include <QDebug>
#include <QLocale>
#include <QTranslator>

namespace LangMgr
{
    ILanguageManagerPrivate::ILanguageManagerPrivate() {}

    ILanguageManagerPrivate::~ILanguageManagerPrivate() = default;

    ILanguageFactory *ILanguageManager::language(const QString &id) const {
        Q_D(const ILanguageManager);
        const auto &it = d->languages.find(id);
        if (it == d->languages.end()) {
            qWarning() << "LangMgr::ILanguageManager::language(): factory does not exist:" << id;
            return nullptr;
        }
        return it.value();
    }

    QList<ILanguageFactory *> ILanguageManager::languages() const {
        Q_D(const ILanguageManager);
        return d->languages.values();
    }

    bool ILanguageManager::addLanguage(ILanguageFactory *factory) {
        Q_D(ILanguageManager);
        if (!factory) {
            qWarning() << "LangMgr::ILanguageManager::addLanguage(): trying to add null factory";
            return false;
        }
        if (d->languages.contains(factory->id())) {
            qWarning() << "LangMgr::ILanguageManager::addLanguage(): trying to add duplicated factory:"
                       << factory->id();
            return false;
        }
        factory->setParent(this);
        d->languages[factory->id()] = factory;
        return true;
    }

    bool ILanguageManager::removeLanguage(const ILanguageFactory *factory) {
        if (factory == nullptr) {
            qWarning() << "LangMgr::ILanguageManager::removeLanguage(): trying to remove null factory";
            return false;
        }
        return removeLanguage(factory->id());
    }

    bool ILanguageManager::removeLanguage(const QString &id) {
        Q_D(ILanguageManager);
        if (!d->languages.contains(id)) {
            qWarning() << "LangMgr::ILanguageManager::removeLanguage(): factory does not exist:" << id;
            return false;
        }
        d->languages.remove(id);
        return true;
    }

    void ILanguageManager::clearLanguages() {
        Q_D(ILanguageManager);
        d->languages.clear();
    }

    QList<Analysiser> ILanguageManagerPrivate::priorityLanguages(const QStringList &priorityG2pIds) const {
        Q_Q(const ILanguageManager);
        QStringList order = this->defaultOrder;
        const auto &g2pMgr = LangMgr::IG2pManager::instance();

        QList<Analysiser> result;
        // 遍历高优先级g2p绑定语种
        for (const auto &g2pId : priorityG2pIds) {
            const auto &g2p = g2pMgr->g2p(g2pId);
            if (g2p == nullptr)
                continue;
            const auto category = g2p->category();
            for (const auto &lang : order) {
                const auto &factory = q->language(lang);
                if (factory->category() == category) {
                    result.append({factory, g2pId});
                }
            }
        }

        for (const auto &id : order) {
            const auto &factory = q->language(id);
            bool add = true;
            for (const auto &[analysis, g2pId] : result) {
                if (analysis == factory) {
                    add = false;
                    break;
                }
            }
            if (add)
                result.append({factory, factory->selectedG2p()});
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

    QList<LangNote> ILanguageManager::split(const QString &input) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityLanguages();
        QList<LangNote> result = {LangNote(input)};
        for (const auto &[analysis, g2pId] : analysisers) {
            result = analysis->split(result, g2pId);
        }
        return result;
    }

    void ILanguageManager::correct(const QList<LangNote *> &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityLanguages(priorityG2pIds);
        for (const auto &[analysis, g2pId] : analysisers) {
            analysis->correct(input, g2pId);
        }
    }

    QString ILanguageManager::analysis(const QString &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        QString result = "unknown";
        const auto &analysisers = d->priorityLanguages(priorityG2pIds);

        for (const auto &[analysis, g2pId] : analysisers) {
            result = analysis->analysis(input);
            if (result != "unknown")
                break;
        }

        return result;
    }

    QStringList ILanguageManager::analysis(const QStringList &input, const QStringList &priorityG2pIds) const {
        Q_D(const ILanguageManager);
        const auto &analysisers = d->priorityLanguages(priorityG2pIds);
        QList<LangNote *> inputNote;
        for (const auto &lyric : input) {
            inputNote.append(new LangNote(lyric));
        }

        for (const auto &[analysis, g2pId] : analysisers) {
            analysis->correct(inputNote, g2pId);
        }

        QStringList result;
        for (const auto &note : inputNote)
            result.append(note->language);
        return result;
    }

    ILanguageManager::ILanguageManager(QObject *parent) : ILanguageManager(*new ILanguageManagerPrivate(), parent) {}

    ILanguageManager::~ILanguageManager() = default;

    ILanguageManager *ILanguageManager::instance() {
        static ILanguageManager obj;
        return &obj;
    }

    bool ILanguageManager::initialize(QString &errMsg) {
        Q_D(ILanguageManager);
        for (const auto &factory : d->languages) {
            if (!factory->initialize(errMsg)) {
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

        addLanguage(new NumberAnalysis());
        addLanguage(new SlurAnalysis());
        addLanguage(new SpaceAnalysis());
        addLanguage(new LinebreakAnalysis());
        addLanguage(new Punctuation());
        addLanguage(new UnknownAnalysis());

        addLanguage(new MandarinAnalysis());
        addLanguage(new PinyinAnalysis());
        addLanguage(new CantoneseAnalysis());
        addLanguage(new JyutpingAnalysis());
        addLanguage(new KanaAnalysis());
        addLanguage(new RomajiAnalysis());
        addLanguage(new EnglishAnalysis());
    }

    void ILanguageManager::convert(const QList<LangNote *> &input) const {
        const auto &g2pMgr = LangMgr::IG2pManager::instance();

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
            auto g2pFactory = g2pMgr->g2p(g2pId);
            if (g2pFactory == nullptr)
                g2pFactory = g2pMgr->g2p("unknown");

            const auto &tempRes = g2pFactory->convert(rawLyrics);
            for (int i = 0; i < tempRes.size(); i++) {
                const auto &index = indexMap[g2pId][i];
                input[index]->syllable = tempRes[i].syllable;
                input[index]->error = tempRes[i].error;
                input[index]->candidates = tempRes[i].candidates;
            }
        }
    }

} // namespace LangMgr
