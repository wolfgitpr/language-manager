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


    const auto &g2p = langMgr->g2p("cmn-pinyin");
    qDebug() << g2p->config();

    for (const auto &note : langMgr->split("ka好的hello121"))
        qDebug() << note.lyric << note.g2pId;

    return 0;
}
