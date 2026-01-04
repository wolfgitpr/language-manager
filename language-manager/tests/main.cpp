#include <QCoreApplication>
#include <QDebug>
#include <qjsondocument.h>

#include <filesystem>

#include <LangMgr/Core/LanguageManager.h>

#ifdef Q_OS_WIN
#include <Windows.h>
#endif

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    const auto langMgr = LangMgr::LanguageManager::instance();

    std::string errorMsg;

    langMgr->initialize(errorMsg);
    qDebug() << "LangMgr: errorMsg" << errorMsg << "initialized:" << langMgr->initialized();

    const auto testId = langMgr->defaultOrder();


    return 0;
}
