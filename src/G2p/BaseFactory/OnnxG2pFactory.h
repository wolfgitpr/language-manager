#ifndef LANGUAGE_MANAGER_ONNXG2PFACTORY_H
#define LANGUAGE_MANAGER_ONNXG2PFACTORY_H

#include <filesystem>
#include <synthrt/Support/Expected.h>

#include "G2pModel.h"
#include "language-manager/IG2pFactory.h"

namespace srt
{
    class SynthUnit;
}

namespace LangMgr
{
    class LANG_MANAGER_EXPORT OnnxG2pFactory : public IG2pFactory {
        Q_OBJECT
    public:
        explicit OnnxG2pFactory(const QString &id, const srt::SynthUnit *su, QObject *parent = nullptr);

        QList<LangNote> convert(const QStringList &input);

        srt::Expected<void> open(const std::filesystem::path &modelPath);

        bool is_open() const;

        void terminate() const;

    private:
        G2pModel m_g2p;
    };

} // namespace LangMgr


#endif // LANGUAGE_MANAGER_ONNXG2PFACTORY_H
