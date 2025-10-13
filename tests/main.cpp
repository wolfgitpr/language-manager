#include <QCoreApplication>
#include <QDebug>
#include <algorithm>
#include <iostream>
#include <qjsondocument.h>
#include <qrandom.h>

#include <filesystem>
#include <fstream>
#include <string>

#include <language-manager/ILanguageManager.h>

#include <dsinfer/Api/Drivers/Onnx/OnnxDriverApi.h>
#include <dsinfer/Inference/InferenceDriver.h>
#include <dsinfer/Inference/InferenceDriverPlugin.h>
#include <stdcorelib/str.h>

#include <stdcorelib/system.h>
#include <synthrt/Core/Contribute.h>
#include <synthrt/Core/NamedObject.h>
#include <synthrt/Core/SynthUnit.h>

#include "../src/G2p/BaseFactory/OnnxG2pFactory.h"
using EP = ds::Api::Onnx::ExecutionProvider;
static srt::Expected<void> initializeSU(srt::SynthUnit &su, const EP ep, int deviceIndex) {
    // Get basic directories
    auto pluginRootDir =
#if defined(Q_OS_MAC)
        MacOSUtils::getMainBundlePath() / _TSTR("Contents/PlugIns");
#elif defined(Q_OS_WIN)
        stdc::system::application_directory() / _TSTR("plugins");
#else
        stdc::system::application_directory().parent_path() / _TSTR("lib/plugins");
#endif
    const auto defaultPluginDir = pluginRootDir / _TSTR("dsinfer");

    // Set default plugin directories
    su.addPluginPath("org.openvpi.InferenceDriver", defaultPluginDir / _TSTR("inferencedrivers"));

    // Load driver
    const auto plugin = su.plugin<ds::InferenceDriverPlugin>("onnx");
    if (!plugin) {
        return srt::Error(srt::Error::FileNotOpen, "failed to load inference driver");
    }

    const auto onnxDriver = plugin->create();
    const auto onnxArgs = srt::NO<ds::Api::Onnx::DriverInitArgs>::create();

    onnxArgs->ep = ep;
    const auto ortParentPath = plugin->path().parent_path() / _TSTR("runtimes") / _TSTR("onnx");
    if (ep == ds::Api::Onnx::CUDAExecutionProvider) {
        onnxArgs->runtimePath = ortParentPath / _TSTR("cuda");
    } else {
        onnxArgs->runtimePath = ortParentPath / _TSTR("default");
    }
    onnxArgs->deviceIndex = deviceIndex;

    if (auto exp = onnxDriver->initialize(onnxArgs); !exp) {
        return srt::Error(srt::Error::FileNotOpen,
                          stdc::formatN(R"(failed to initialize onnx driver: %1)", exp.error().message()));
    }

    // Add driver
    auto &ic = *su.category("inference");
    ic.addObject("dsdriver", onnxDriver);
    return srt::Expected<void>();
}
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
            const auto [fst, snd] = factory->randString();
            note->lyric = fst;
            note->standardG2pId = factory->id();
            langNotes.append(note);
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
    qDebug() << g2p->config();

    for (const auto &note : langMgr->split("ka好的hello121"))
        qDebug() << note.lyric << note.g2pId;

    qDebug() << "好点"
             << "cmn-pinyin" << g2p->analysis("好点");

    const auto g2pProvider = [](const std::string &provider_) -> EP
    {
        const auto provider_lower = stdc::to_lower(provider_);
        if (provider_lower == "dml" || provider_lower == "directml") {
            return EP::DMLExecutionProvider;
        }
        if (provider_lower == "cuda") {
            return EP::CUDAExecutionProvider;
        }
        if (provider_lower == "coreml") {
            return EP::CoreMLExecutionProvider;
        }
        return EP::CPUExecutionProvider;
    }("cpu");

    srt::SynthUnit su;
    if (auto exp = initializeSU(su, g2pProvider, 0); !exp) {
        std::cerr << "failed to initialize SynthUnit: " << exp.error().message() << std::endl;
    }
    const auto onnx_g2p = new LangMgr::OnnxG2pFactory("onnx_en", &su);
    qDebug() << "onnx_g2p: hello ->" << onnx_g2p->convert(QStringList({"hello"})).first().syllable;
    return 0;
}
