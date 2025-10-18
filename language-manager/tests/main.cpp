#include <QCoreApplication>
#include <QDebug>
#include <algorithm>
#include <iostream>
#include <qjsondocument.h>
#include <qrandom.h>

#include <filesystem>
#include <fstream>

#include <LangMgr/Core/LanguageEngine.h>
#include <LangMgr/Support/U32Str.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const auto langMgr = LangMgr::LanguageEngine::instance();

    std::string errorMsg;

    langMgr->initialize(errorMsg);
    qDebug() << "LangMgr: errorMsg" << errorMsg << "initialized:" << langMgr->initialized();

    const auto testId = langMgr->defaultOrder();

    std::vector<LangNote *> langNotes;

    for (const auto &g2pId : testId) {
        const int length = QRandomGenerator::global()->bounded(1, 3);
        const auto factory = langMgr->g2p(g2pId);

        for (int i = 0; i < length; i++) {
            const auto note = new LangNote();
            const auto [fst, snd] = factory->randString();
            note->lyric = LangMgr::utf8strToU32str(fst);
            note->standardG2pId = factory->id();
            langNotes.push_back(note);
        }
    }

    std::mt19937 gen(std::random_device{}());
    std::shuffle(langNotes.begin(), langNotes.end(), gen);

    langMgr->correct(langNotes, {});
    langMgr->convert(langNotes);

    for (const auto &note : langNotes) {
        if (note->g2pId != note->standardG2pId) {
            qDebug() << "lyric: " << note->lyric << " standardG2pId: " << note->standardG2pId
                     << " res: " << note->g2pId;
        }
        delete note;
    }

    qDebug() << "LangMgrTest: success";

    const auto res = langMgr->split("xa112好eng");
    for (const auto &note : res)
        qDebug() << note.lyric << note.g2pId;

    const auto &g2p = langMgr->g2p("cmn-pinyin");

    for (const auto &note : langMgr->split("ka好的hello121"))
        qDebug() << note.lyric << note.g2pId;

    qDebug() << "好点"
             << "cmn-pinyin" << g2p->analysis(U"好点");

    const QString onnxRuntimePath = QCoreApplication::applicationDirPath() +
        "/plugins/dsinfer/inferencedrivers/runtimes/onnx/default/onnxruntime.dll";

#ifdef Q_OS_WIN
    const HMODULE hOnnxRuntime = LoadLibraryW(onnxRuntimePath.toStdWString().c_str());
    if (hOnnxRuntime == nullptr) {
        const DWORD error = GetLastError();
        qDebug() << "Failed to load onnxruntime.dll from" << onnxRuntimePath << "Error code:" << error;
        return -1;
    }
    qDebug() << "Successfully loaded onnxruntime.dll";
#endif

    // const auto onnx_g2p = new LangMgr::OnnxG2pFactory("onnx_en");
    // qDebug() << "onnx_g2p: hello ->" << onnx_g2p->convert(QStringList({"hello"})).first().syllable;

#ifdef Q_OS_WIN
    if (hOnnxRuntime) {
        FreeLibrary(hOnnxRuntime);
        qDebug() << "Released onnxruntime.dll";
    }
#endif

    return 0;
}
