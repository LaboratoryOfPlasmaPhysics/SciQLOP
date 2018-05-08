#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include "CoreGlobal.h"
#include "Response.h"

#include <Common/MetaTypes.h>
#include <Common/spimpl.h>
#include <functional>

#include <QString>
#include <QByteArray>
#include <QUuid>

/**
 * @brief The Downloader handles all data donwloads in SciQLOP.
 */
class SCIQLOP_CORE_EXPORT Downloader{
public:
    static Response get(const QString& url, const QString& user="", const QString& passwd="");
    static QUuid getAsync(const QString& url, std::function<void (QUuid, Response)> callback, const QString& user="", const QString& passwd="");
    static bool downloadFinished(QUuid uuid);

    static Downloader& instance()
    {
        static Downloader inst;
        return  inst;
    }

private:
    class p_Downloader;

    explicit Downloader();

    spimpl::unique_impl_ptr<Downloader::p_Downloader> impl;
};

#endif // DOWNLOADER_H