#ifndef SCIQLOP_SQPCOLORSCALE_H
#define SCIQLOP_SQPCOLORSCALE_H

#include <Visualization/qcustomplot.h>

#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(LOG_SqpColorScale)

/**
 * @brief The SqpColorScale struct represents the color scale for some graphs (such as
 * spectrograms).
 *
 * Its implementation is based on the QCustomPlot color scale (@sa QCPColorScale) to which are added
 * other useful properties for viewing in SciQlop
 */
struct SqpColorScale {
    static std::pair<double, double> computeThresholds(const SqpColorScale &scale);

    explicit SqpColorScale(QCustomPlot &plot);

    void updateDataRange() noexcept;

    /// QCustomPlot object representing the color scale.
    /// @remarks The SqpColorScale instance has not the property on this pointer. The pointer must
    /// remain valid throughout the existence of the SqpColorScale instance
    QCPColorScale *m_Scale{nullptr};
    bool m_AutomaticThreshold{false};
    QCPColorGradient::GradientPreset m_GradientPreset{QCPColorGradient::gpJet};
};

#endif // SCIQLOP_SQPCOLORSCALE_H
