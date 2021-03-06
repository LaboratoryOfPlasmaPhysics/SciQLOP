#include "Visualization/SqpColorScale.h"

#include <Visualization/QCPColorMapIterator.h>

Q_LOGGING_CATEGORY(LOG_SqpColorScale, "SqpColorScale")

namespace
{

const auto DEFAULT_GRADIENT_PRESET = QCPColorGradient::gpJet;
const auto DEFAULT_RANGE = QCPRange { 1.0e3, 1.7e7 };

} // namespace

std::pair<double, double> SqpColorScale::computeThresholds(const SqpColorScale& scale)
{
    auto qcpScale = scale.m_Scale;

    auto colorMaps = qcpScale->colorMaps();
    if (colorMaps.size() != 1)
    {
        return { std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::quiet_NaN() };
    }

    // Computes thresholds
    auto isLogarithmicScale = qcpScale->dataScaleType() == QCPAxis::stLogarithmic;
    auto colorMapData = colorMaps.first()->data();
    QCPColorMapIterator begin { colorMapData, true };
    QCPColorMapIterator end { colorMapData, false };

    return {}; // DataSeriesUtils::thresholds(begin, end, isLogarithmicScale);
}

SqpColorScale::SqpColorScale(QCustomPlot& plot)
        : m_Scale { new QCPColorScale { &plot } }
        , m_AutomaticThreshold { false }
        , m_GradientPreset { DEFAULT_GRADIENT_PRESET }
{
    m_Scale->setGradient(m_GradientPreset);
    m_Scale->setDataRange(DEFAULT_RANGE);
}

void SqpColorScale::updateDataRange() noexcept
{
    // Updates data range only if mode is automatic
    if (!m_AutomaticThreshold)
    {
        return;
    }

    double minThreshold, maxThreshold;
    std::tie(minThreshold, maxThreshold) = computeThresholds(*this);
    if (std::isnan(minThreshold) || std::isnan(maxThreshold))
    {
        qCCritical(LOG_SqpColorScale())
            << "Can't update data range of color scale: thresholds computed are invalid";
        return;
    }

    // Updates thresholds
    m_Scale->setDataRange({ minThreshold, maxThreshold });
}
