#include <QCoreApplication>
#include <QDebug>
#include <algorithm>
#include <qjsondocument.h>
#include <qrandom.h>

#include <language-manager/ILanguageManager.h>

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const auto langMgr = LangMgr::ILanguageManager::instance();

    QString errorMsg;

    auto args = QJsonObject();
    args.insert("pinyinDictPath", qApp->applicationDirPath() + "/dict");

    langMgr->initialize(args, errorMsg);
    qDebug() << "LangMgr: errorMsg" << errorMsg << "initialized:" << langMgr->initialized();

    const QStringList testId = langMgr->defaultOrder();
    // langMgr->setDefaultOrder(testId);

    const auto g2pFactory = langMgr->g2p("cmn");
    qDebug() << "g2pFactory config:" << g2pFactory->config();
    QJsonObject rjson;
    rjson.insert("0",
                 "{\"cmn\":{\"discardResult\":true,\"enabled\":true},\"cmn-pinyin\":{\"discardResult\":"
                 "true,\"enabled\":true},\"tone\":true}");
    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(rjson.value("0").toString().toUtf8(), &parseError);
    if (parseError.error == QJsonParseError::NoError && jsonDoc.isObject()) {
        QJsonObject json = jsonDoc.object();
        qDebug() << "Parsed json:" << json;
        qDebug() << json.value("cmn").toObject() << json.value("cmn").toObject().value("enabled").toBool();

        // 调用 g2pFactory->loadLanguageConfig()
        g2pFactory->loadG2pConfig(json);
        g2pFactory->loadLanguageConfig(json);
        qDebug() << "update" << g2pFactory->languageConfig();
    } else {
        qDebug() << "JSON parse error:" << parseError.errorString();
    }

    QList<LangNote *> langNotes;

    for (const auto &g2pId : testId) {
        const int lenth = QRandomGenerator::global()->bounded(1, 3);
        const auto factory = langMgr->g2p(g2pId);

        for (int i = 0; i < lenth; i++) {
            const auto note = new LangNote();
            note->lyric = factory->randString();
            note->standard = g2pId;
            langNotes.append(note);
        }
    }

    std::mt19937 gen(std::random_device{}());
    std::shuffle(langNotes.begin(), langNotes.end(), gen);

    langMgr->correct(langNotes, {});
    langMgr->convert(langNotes);

    for (const auto &note : langNotes) {
        if (note->g2pId != note->standard) {
            qDebug() << "lyric: " << note->lyric << " standard: " << note->standard << " res: " << note->g2pId;
        }
        delete note;
    }

    qDebug() << "LangMgrTest: success";

    const auto res = langMgr->split("xa112好eng");
    for (const auto &note : res)
        qDebug() << note.lyric << note.g2pId;


    const auto &g2p = langMgr->g2p("cmn");
    qDebug() << g2p->config();

    for (const auto &note : langMgr->split("ka好的hello121"))
        qDebug() << note.lyric << note.g2pId;

    return 0;
}
