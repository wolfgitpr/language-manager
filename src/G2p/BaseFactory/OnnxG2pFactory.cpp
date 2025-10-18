#include "OnnxG2pFactory.h"
#include "G2pDriver.h"

#include <QCoreApplication>
#include <stdcorelib/str.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("onnxruntime.dll")
#elif defined(__APPLE__)
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("libonnxruntime.dylib")
#else
#define ONNXRUNTIME_DYLIB_FILENAME _TSTR("libonnxruntime.so")
#endif

namespace LangMgr
{
    OnnxG2pFactory::OnnxG2pFactory(const QString &id, QObject *parent) : IG2pFactory(id, parent) {
        m_driver = new G2pDriver();

        const auto runtimePath = std::filesystem::path(
            (QCoreApplication::applicationDirPath() + "/plugins/dsinfer/inferencedrivers/runtimes/onnx/default/")
                .toStdString());

        const auto dllPath = runtimePath / ONNXRUNTIME_DYLIB_FILENAME;
        if (!std::filesystem::exists(dllPath)) {
            std::cout << "ONNX Runtime DLL not found at: " << dllPath << std::endl;
            return;
        }

        if (const auto result = m_driver->loadFromProcess(); !result) {
            std::cout << "Failed to load ONNX Runtime DLL: " << std::endl;
            return;
        }

        const auto modelPath = std::filesystem::path(
            (QCoreApplication::applicationDirPath() + "/g2p_onnx/lstm_g2p_en/model.onnx").toStdString());
        if (!std::filesystem::exists(modelPath)) {
            std::cout << "ONNX model not found at: " << modelPath << std::endl;
            return;
        }

        m_g2p = new G2pModel(m_driver, modelPath, ExecutionProvider::CPU, 0);
        if (!m_g2p->is_open()) {
            std::cout << "Failed to create ONNX session for model: " << modelPath << std::endl;
        }
    }

    OnnxG2pFactory::~OnnxG2pFactory() {
        delete m_g2p;
        delete m_driver;
    }

    QList<LangNote> OnnxG2pFactory::convert(const QStringList &input) const {
        QList<LangNote> result;
        for (auto &c : input) {
            LangNote langNote;
            langNote.lyric = c;
            const auto phonemes = m_g2p->forward(c.toLower().toStdString());
            QString phonemeStr;
            for (const auto &phoneme : phonemes)
                phonemeStr += QString::fromStdString(phoneme) + " ";
            langNote.syllable = phonemeStr;
            langNote.candidates = QStringList() << langNote.syllable;
            result.append(langNote);
        }
        return result;
    }

    bool OnnxG2pFactory::is_open() const { return m_g2p->is_open(); }

    void OnnxG2pFactory::terminate() const { m_g2p->terminate(); }

} // namespace LangMgr
