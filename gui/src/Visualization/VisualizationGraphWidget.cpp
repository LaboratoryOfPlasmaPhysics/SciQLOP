#include "Visualization/VisualizationGraphWidget.h"
#include "Visualization/IVisualizationWidgetVisitor.h"
#include "Visualization/VisualizationGraphHelper.h"
#include "ui_VisualizationGraphWidget.h"

#include <Data/ArrayData.h>
#include <Data/IDataSeries.h>
#include <SqpApplication.h>
#include <Variable/Variable.h>
#include <Variable/VariableController.h>

#include <unordered_map>

Q_LOGGING_CATEGORY(LOG_VisualizationGraphWidget, "VisualizationGraphWidget")

namespace {

/// Key pressed to enable zoom on horizontal axis
const auto HORIZONTAL_ZOOM_MODIFIER = Qt::NoModifier;

/// Key pressed to enable zoom on vertical axis
const auto VERTICAL_ZOOM_MODIFIER = Qt::ControlModifier;

} // namespace

struct VisualizationGraphWidget::VisualizationGraphWidgetPrivate {

    explicit VisualizationGraphWidgetPrivate() : m_DoSynchronize(true) {}

    // 1 variable -> n qcpplot
    std::multimap<std::shared_ptr<Variable>, QCPAbstractPlottable *> m_VariableToPlotMultiMap;

    bool m_DoSynchronize;
};

VisualizationGraphWidget::VisualizationGraphWidget(const QString &name, QWidget *parent)
        : QWidget{parent},
          ui{new Ui::VisualizationGraphWidget},
          impl{spimpl::make_unique_impl<VisualizationGraphWidgetPrivate>()}
{
    ui->setupUi(this);

    ui->graphNameLabel->setText(name);

    // 'Close' options : widget is deleted when closed
    setAttribute(Qt::WA_DeleteOnClose);
    connect(ui->closeButton, &QToolButton::clicked, this, &VisualizationGraphWidget::close);
    ui->closeButton->setIcon(sqpApp->style()->standardIcon(QStyle::SP_TitleBarCloseButton));

    // Set qcpplot properties :
    // - Drag (on x-axis) and zoom are enabled
    // - Mouse wheel on qcpplot is intercepted to determine the zoom orientation
    ui->widget->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->widget->axisRect()->setRangeDrag(Qt::Horizontal);
    connect(ui->widget, &QCustomPlot::mouseWheel, this, &VisualizationGraphWidget::onMouseWheel);
    connect(ui->widget->xAxis, static_cast<void (QCPAxis::*)(const QCPRange &, const QCPRange &)>(
                                   &QCPAxis::rangeChanged),
            this, &VisualizationGraphWidget::onRangeChanged);

    // Activates menu when right clicking on the graph
    ui->widget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->widget, &QCustomPlot::customContextMenuRequested, this,
            &VisualizationGraphWidget::onGraphMenuRequested);

    connect(this, &VisualizationGraphWidget::requestDataLoading, &sqpApp->variableController(),
            &VariableController::onRequestDataLoading);
}


VisualizationGraphWidget::~VisualizationGraphWidget()
{
    delete ui;
}

void VisualizationGraphWidget::enableSynchronize(bool enable)
{
    impl->m_DoSynchronize = enable;
}

void VisualizationGraphWidget::addVariable(std::shared_ptr<Variable> variable)
{
    // Uses delegate to create the qcpplot components according to the variable
    auto createdPlottables = VisualizationGraphHelper::create(variable, *ui->widget);

    for (auto createdPlottable : qAsConst(createdPlottables)) {
        impl->m_VariableToPlotMultiMap.insert({variable, createdPlottable});
    }

    connect(variable.get(), SIGNAL(updated()), this, SLOT(onDataCacheVariableUpdated()));
}
void VisualizationGraphWidget::addVariableUsingGraph(std::shared_ptr<Variable> variable)
{

    // when adding a variable, we need to set its time range to the current graph range
    auto grapheRange = ui->widget->xAxis->range();
    auto dateTime = SqpDateTime{grapheRange.lower, grapheRange.upper};
    variable->setDateTime(dateTime);

    auto variableDateTimeWithTolerance = dateTime;

    // add 10% tolerance for each side
    auto tolerance = 0.1 * (dateTime.m_TEnd - dateTime.m_TStart);
    variableDateTimeWithTolerance.m_TStart -= tolerance;
    variableDateTimeWithTolerance.m_TEnd += tolerance;

    // Uses delegate to create the qcpplot components according to the variable
    auto createdPlottables = VisualizationGraphHelper::create(variable, *ui->widget);

    for (auto createdPlottable : qAsConst(createdPlottables)) {
        impl->m_VariableToPlotMultiMap.insert({variable, createdPlottable});
    }

    connect(variable.get(), SIGNAL(updated()), this, SLOT(onDataCacheVariableUpdated()));

    // CHangement detected, we need to ask controller to request data loading
    emit requestDataLoading(variable, variableDateTimeWithTolerance);
}

void VisualizationGraphWidget::removeVariable(std::shared_ptr<Variable> variable) noexcept
{
    // Each component associated to the variable :
    // - is removed from qcpplot (which deletes it)
    // - is no longer referenced in the map
    auto componentsIt = impl->m_VariableToPlotMultiMap.equal_range(variable);
    for (auto it = componentsIt.first; it != componentsIt.second;) {
        ui->widget->removePlottable(it->second);
        it = impl->m_VariableToPlotMultiMap.erase(it);
    }

    // Updates graph
    ui->widget->replot();
}

void VisualizationGraphWidget::setRange(std::shared_ptr<Variable> variable,
                                        const SqpDateTime &range)
{
    //    auto componentsIt = impl->m_VariableToPlotMultiMap.equal_range(variable);
    //    for (auto it = componentsIt.first; it != componentsIt.second;) {
    //    }
    ui->widget->xAxis->setRange(range.m_TStart, range.m_TEnd);
    ui->widget->replot();
}

SqpDateTime VisualizationGraphWidget::graphRange()
{
    auto grapheRange = ui->widget->xAxis->range();
    return SqpDateTime{grapheRange.lower, grapheRange.upper};
}

void VisualizationGraphWidget::setGraphRange(const SqpDateTime &range)
{
    qCDebug(LOG_VisualizationGraphWidget())
        << tr("VisualizationGraphWidget::setGraphRange START");
    ui->widget->xAxis->setRange(range.m_TStart, range.m_TEnd);
    ui->widget->replot();
    qCDebug(LOG_VisualizationGraphWidget()) << tr("VisualizationGraphWidget::setGraphRange END");
}

void VisualizationGraphWidget::accept(IVisualizationWidgetVisitor *visitor)
{
    if (visitor) {
        visitor->visit(this);
    }
    else {
        qCCritical(LOG_VisualizationGraphWidget())
            << tr("Can't visit widget : the visitor is null");
    }
}

bool VisualizationGraphWidget::canDrop(const Variable &variable) const
{
    /// @todo : for the moment, a graph can always accomodate a variable
    Q_UNUSED(variable);
    return true;
}

bool VisualizationGraphWidget::contains(const Variable &variable) const
{
    // Finds the variable among the keys of the map
    auto variablePtr = &variable;
    auto findVariable
        = [variablePtr](const auto &entry) { return variablePtr == entry.first.get(); };

    auto end = impl->m_VariableToPlotMultiMap.cend();
    auto it = std::find_if(impl->m_VariableToPlotMultiMap.cbegin(), end, findVariable);
    return it != end;
}

QString VisualizationGraphWidget::name() const
{
    return ui->graphNameLabel->text();
}

void VisualizationGraphWidget::onGraphMenuRequested(const QPoint &pos) noexcept
{
    QMenu graphMenu{};

    // Iterates on variables (unique keys)
    for (auto it = impl->m_VariableToPlotMultiMap.cbegin(),
              end = impl->m_VariableToPlotMultiMap.cend();
         it != end; it = impl->m_VariableToPlotMultiMap.upper_bound(it->first)) {
        // 'Remove variable' action
        graphMenu.addAction(tr("Remove variable %1").arg(it->first->name()),
                            [ this, var = it->first ]() { removeVariable(var); });
    }

    if (!graphMenu.isEmpty()) {
        graphMenu.exec(mapToGlobal(pos));
    }
}

void VisualizationGraphWidget::onRangeChanged(const QCPRange &t1, const QCPRange &t2)
{
    qCInfo(LOG_VisualizationGraphWidget()) << tr("VisualizationGraphWidget::onRangeChanged")
                                           << QThread::currentThread()->objectName();

    auto dateTimeRange = SqpDateTime{t1.lower, t1.upper};

    auto zoomType = VisualizationGraphWidgetZoomType::ZoomOut;
    for (auto it = impl->m_VariableToPlotMultiMap.cbegin();
         it != impl->m_VariableToPlotMultiMap.cend(); ++it) {

        auto variable = it->first;
        auto currentDateTime = dateTimeRange;

        auto toleranceFactor = 0.2;
        auto tolerance = toleranceFactor * (currentDateTime.m_TEnd - currentDateTime.m_TStart);
        auto variableDateTimeWithTolerance = currentDateTime;
        variableDateTimeWithTolerance.m_TStart -= tolerance;
        variableDateTimeWithTolerance.m_TEnd += tolerance;

        qCDebug(LOG_VisualizationGraphWidget()) << "r" << currentDateTime;
        qCDebug(LOG_VisualizationGraphWidget()) << "t" << variableDateTimeWithTolerance;
        qCDebug(LOG_VisualizationGraphWidget()) << "v" << variable->dateTime();
        // If new range with tol is upper than variable datetime parameters. we need to request new
        // data
        if (!variable->contains(variableDateTimeWithTolerance)) {

            auto variableDateTimeWithTolerance = currentDateTime;
            if (!variable->isInside(currentDateTime)) {
                auto variableDateTime = variable->dateTime();
                if (variable->contains(variableDateTimeWithTolerance)) {
                    qCInfo(LOG_VisualizationGraphWidget())
                        << tr("TORM: Detection zoom in that need request:");
                    // add 10% tolerance for each side
                    tolerance
                        = toleranceFactor * (currentDateTime.m_TEnd - currentDateTime.m_TStart);
                    variableDateTimeWithTolerance.m_TStart -= tolerance;
                    variableDateTimeWithTolerance.m_TEnd += tolerance;
                    zoomType = VisualizationGraphWidgetZoomType::ZoomIn;
                }
                else if (variableDateTime.m_TStart < currentDateTime.m_TStart) {
                    qCInfo(LOG_VisualizationGraphWidget()) << tr("TORM: Detection pan to right:");

                    auto diffEndToKeepDelta = currentDateTime.m_TEnd - variableDateTime.m_TEnd;
                    currentDateTime.m_TStart = variableDateTime.m_TStart + diffEndToKeepDelta;
                    // Tolerance have to be added to the right
                    // add tolerance for right (end) side
                    tolerance
                        = toleranceFactor * (currentDateTime.m_TEnd - currentDateTime.m_TStart);
                    variableDateTimeWithTolerance.m_TEnd += tolerance;
                    zoomType = VisualizationGraphWidgetZoomType::PanRight;
                }
                else if (variableDateTime.m_TEnd > currentDateTime.m_TEnd) {
                    qCInfo(LOG_VisualizationGraphWidget()) << tr("TORM: Detection pan to left: ");
                    auto diffStartToKeepDelta
                        = variableDateTime.m_TStart - currentDateTime.m_TStart;
                    currentDateTime.m_TEnd = variableDateTime.m_TEnd - diffStartToKeepDelta;
                    // Tolerance have to be added to the left
                    // add tolerance for left (start) side
                    tolerance
                        = toleranceFactor * (currentDateTime.m_TEnd - currentDateTime.m_TStart);
                    variableDateTimeWithTolerance.m_TStart -= tolerance;
                    zoomType = VisualizationGraphWidgetZoomType::PanLeft;
                }
                else {
                    qCInfo(LOG_VisualizationGraphWidget())
                        << tr("Detection anormal zoom detection: ");
                    zoomType = VisualizationGraphWidgetZoomType::Unknown;
                }
            }
            else {
                qCInfo(LOG_VisualizationGraphWidget()) << tr("TORM: Detection zoom out: ");
                // add 10% tolerance for each side
                tolerance = toleranceFactor * (currentDateTime.m_TEnd - currentDateTime.m_TStart);
                variableDateTimeWithTolerance.m_TStart -= tolerance;
                variableDateTimeWithTolerance.m_TEnd += tolerance;
                zoomType = VisualizationGraphWidgetZoomType::ZoomOut;
            }
            if (!variable->contains(dateTimeRange)) {
                qCInfo(LOG_VisualizationGraphWidget())
                    << "TORM: Modif on variable datetime detected" << currentDateTime;
                variable->setDateTime(currentDateTime);
            }

            qCInfo(LOG_VisualizationGraphWidget()) << tr("TORM: Request data detection: ");
            // CHangement detected, we need to ask controller to request data loading
            emit requestDataLoading(variable, variableDateTimeWithTolerance);
        }
        else {
            qCInfo(LOG_VisualizationGraphWidget())
                << tr("TORM: Detection zoom in that doesn't need request: ");
            zoomType = VisualizationGraphWidgetZoomType::ZoomIn;
        }
    }

    if (impl->m_DoSynchronize) {
        auto oldDateTime = SqpDateTime{t2.lower, t2.upper};
        qCDebug(LOG_VisualizationGraphWidget())
            << tr("TORM: VisualizationGraphWidget::Synchronize notify !!")
            << QThread::currentThread()->objectName();
        emit synchronize(dateTimeRange, oldDateTime, zoomType);
    }
}

void VisualizationGraphWidget::onMouseWheel(QWheelEvent *event) noexcept
{
    auto zoomOrientations = QFlags<Qt::Orientation>{};

    // Lambda that enables a zoom orientation if the key modifier related to this orientation
    // has
    // been pressed
    auto enableOrientation
        = [&zoomOrientations, event](const auto &orientation, const auto &modifier) {
              auto orientationEnabled = event->modifiers().testFlag(modifier);
              zoomOrientations.setFlag(orientation, orientationEnabled);
          };
    enableOrientation(Qt::Vertical, VERTICAL_ZOOM_MODIFIER);
    enableOrientation(Qt::Horizontal, HORIZONTAL_ZOOM_MODIFIER);

    ui->widget->axisRect()->setRangeZoom(zoomOrientations);
}

void VisualizationGraphWidget::onDataCacheVariableUpdated()
{
    // NOTE:
    //    We don't want to call the method for each component of a variable unitarily, but for
    //    all
    //    its components at once (eg its three components in the case of a vector).

    //    The unordered_multimap does not do this easily, so the question is whether to:
    //    - use an ordered_multimap and the algos of std to group the values by key
    //    - use a map (unique keys) and store as values directly the list of components

    auto grapheRange = ui->widget->xAxis->range();
    auto dateTime = SqpDateTime{grapheRange.lower, grapheRange.upper};

    for (auto it = impl->m_VariableToPlotMultiMap.cbegin();
         it != impl->m_VariableToPlotMultiMap.cend(); ++it) {
        auto variable = it->first;
        qCDebug(LOG_VisualizationGraphWidget())
            << "TORM: VisualizationGraphWidget::onDataCacheVariableUpdated S"
            << variable->dateTime();
        qCDebug(LOG_VisualizationGraphWidget())
            << "TORM: VisualizationGraphWidget::onDataCacheVariableUpdated E" << dateTime;
        if (dateTime.contains(variable->dateTime()) || dateTime.intersect(variable->dateTime())) {

            VisualizationGraphHelper::updateData(QVector<QCPAbstractPlottable *>{} << it->second,
                                                 variable->dataSeries(), variable->dateTime());
        }
    }
}
