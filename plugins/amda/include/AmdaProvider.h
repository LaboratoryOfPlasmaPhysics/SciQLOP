#ifndef SCIQLOP_AMDAPROVIDER_H
#define SCIQLOP_AMDAPROVIDER_H

#include "AmdaGlobal.h"

#include <Data/IDataProvider.h>

#include <QLoggingCategory>


Q_DECLARE_LOGGING_CATEGORY(LOG_AmdaProvider)

class QNetworkReply;

/**
 * @brief The AmdaProvider class is an example of how a data provider can generate data
 */
class SCIQLOP_AMDA_EXPORT AmdaProvider : public IDataProvider {
public:
    explicit AmdaProvider();

    void requestDataLoading(QUuid acqIdentifier, const DataProviderParameters &parameters) override;

    void requestDataAborting(QUuid acqIdentifier) override;

private:
    void retrieveData(QUuid token, const SqpRange &dateTime, const QVariantHash &data);
};

#endif // SCIQLOP_AMDAPROVIDER_H
