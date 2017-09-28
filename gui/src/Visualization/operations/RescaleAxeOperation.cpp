#include "Visualization/operations/RescaleAxeOperation.h"
#include "Visualization/VisualizationGraphWidget.h"

Q_LOGGING_CATEGORY(LOG_RescaleAxeOperation, "RescaleAxeOperation")

struct RescaleAxeOperation::RescaleAxeOperationPrivate {
    explicit RescaleAxeOperationPrivate(std::shared_ptr<Variable> variable, const SqpRange &range)
            : m_Variable{variable}, m_Range{range}
    {
    }

    std::shared_ptr<Variable> m_Variable;
    SqpRange m_Range;
};

RescaleAxeOperation::RescaleAxeOperation(std::shared_ptr<Variable> variable, const SqpRange &range)
        : impl{spimpl::make_unique_impl<RescaleAxeOperationPrivate>(variable, range)}
{
}

void RescaleAxeOperation::visitEnter(VisualizationWidget *widget)
{
    // VisualizationWidget is not intended to contain a variable
    Q_UNUSED(widget)
}

void RescaleAxeOperation::visitLeave(VisualizationWidget *widget)
{
    // VisualizationWidget is not intended to contain a variable
    Q_UNUSED(widget)
}

void RescaleAxeOperation::visitEnter(VisualizationTabWidget *tabWidget)
{
    // VisualizationTabWidget is not intended to contain a variable
    Q_UNUSED(tabWidget)
}

void RescaleAxeOperation::visitLeave(VisualizationTabWidget *tabWidget)
{
    // VisualizationTabWidget is not intended to contain a variable
    Q_UNUSED(tabWidget)
}

void RescaleAxeOperation::visitEnter(VisualizationZoneWidget *zoneWidget)
{
    // VisualizationZoneWidget is not intended to contain a variable
    Q_UNUSED(zoneWidget)
}

void RescaleAxeOperation::visitLeave(VisualizationZoneWidget *zoneWidget)
{
    // VisualizationZoneWidget is not intended to contain a variable
    Q_UNUSED(zoneWidget)
}

void RescaleAxeOperation::visit(VisualizationGraphWidget *graphWidget)
{
    if (graphWidget) {
        // If the widget contains the variable, rescale it
        if (impl->m_Variable && graphWidget->contains(*impl->m_Variable)) {
            graphWidget->enableAcquisition(false);
            graphWidget->setGraphRange(impl->m_Range);
            graphWidget->enableAcquisition(true);
        }
    }
    else {
        qCCritical(LOG_RescaleAxeOperation(),
                   "Can't visit VisualizationGraphWidget : the widget is null");
    }
}
