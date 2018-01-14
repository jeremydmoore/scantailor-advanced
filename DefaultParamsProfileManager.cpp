
#include <QtGui/QtGui>
#include <QtXml/QDomDocument>
#include "DefaultParamsProfileManager.h"
#include "DefaultParams.h"

DefaultParamsProfileManager::DefaultParamsProfileManager()
        : path(qApp->applicationDirPath() + "/config/profiles") {
}

DefaultParamsProfileManager::DefaultParamsProfileManager(const QString& path)
        : path(path) {
}

std::unique_ptr<std::list<QString>> DefaultParamsProfileManager::getProfileList() {
    auto profileList = std::make_unique<std::list<QString>>();

    QDir dir(path);
    if (dir.exists()) {
        QList<QFileInfo> fileInfoList = dir.entryInfoList();
        for (const QFileInfo& fileInfo : fileInfoList) {
            if (fileInfo.isFile()
                && ((fileInfo.suffix() == "stp")
                    || (fileInfo.suffix() == "xml"))) {
                profileList->push_back(fileInfo.baseName());
            }
        }
    }

    return std::move(profileList);
}

std::unique_ptr<DefaultParams> DefaultParamsProfileManager::readProfile(const QString& name) {
    QDir dir(path);
    QFileInfo profile(dir.absoluteFilePath(name + ".stp"));
    if (!profile.exists()) {
        profile = dir.absoluteFilePath(name + ".xml");
        if (!profile.exists()) {
            return nullptr;
        }
    }

    QFile profileFile(profile.filePath());
    if (!profileFile.open(QIODevice::ReadOnly)) {
        return nullptr;
    }

    QDomDocument doc;
    if (!doc.setContent(&profileFile)) {
        return nullptr;
    }

    profileFile.close();

    return std::make_unique<DefaultParams>(doc.documentElement());
}

bool DefaultParamsProfileManager::writeProfile(const DefaultParams& params, const QString& name) {
    QDomDocument doc;
    doc.appendChild(params.toXml(doc, "profile"));

    QDir dir(path);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(dir.absoluteFilePath(name + ".stp"));
    if (file.open(QIODevice::WriteOnly)) {
        QTextStream textStream(&file);
        doc.save(textStream, 2);
        return true;
    }

    return false;
}

std::unique_ptr<DefaultParams> DefaultParamsProfileManager::createDefaultProfile() {
    return std::make_unique<DefaultParams>();
}

std::unique_ptr<DefaultParams> DefaultParamsProfileManager::createSourceProfile() {
    return std::make_unique<DefaultParams>();
}

bool DefaultParamsProfileManager::deleteProfile(const QString& name) {
    QDir dir(path);
    QFileInfo profile(dir.absoluteFilePath(name + ".stp"));
    if (!profile.exists()) {
        profile = dir.absoluteFilePath(name + ".xml");
        if (!profile.exists()) {
            return false;
        }
    }

    QFile profileFile(profile.filePath());
    return profileFile.remove();
}

