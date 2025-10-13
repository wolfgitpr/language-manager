#include "OnnxG2pFactory.h"

#include <QCoreApplication>
#include <stdcorelib/str.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <synthrt/Core/Contribute.h>

namespace LangMgr
{
    OnnxG2pFactory::OnnxG2pFactory(const QString &id, const srt::SynthUnit *su, QObject *parent) :
        IG2pFactory(id, parent), m_g2p(G2pModel(su)) {
        const auto modelPath =
            std::filesystem::path((QCoreApplication::applicationDirPath() + "/g2p_onnx/lstm_g2p_en").toStdString());
        if (auto exp = m_g2p.open(modelPath); !exp) {
            std::cerr << "failed to open Lstm G2p model " << modelPath << ": " << exp.error().message() << std::endl;
        }
    }

    QList<LangNote> OnnxG2pFactory::convert(const QStringList &input) {
        QList<LangNote> result;
        for (auto &c : input) {
            LangNote langNote;
            langNote.lyric = c;
            const auto phonemes = m_g2p.forward(c.toLower().toStdString());
            QString phonemeStr;
            for (const auto &phoneme : phonemes.value())
                phonemeStr += QString::fromStdString(phoneme) + " ";
            langNote.syllable = phonemeStr;
            langNote.candidates = QStringList() << langNote.syllable;
            result.append(langNote);
        }
        return result;
    }

    srt::Expected<void> OnnxG2pFactory::open(const std::filesystem::path &modelPath) { return m_g2p.open(modelPath); }

    bool OnnxG2pFactory::is_open() const { return m_g2p.is_open(); }


    void OnnxG2pFactory::terminate() const { m_g2p.terminate(); }


} // namespace LangMgr
