#include "Visualization/VisualizationGraphHelper.h"
#include "Visualization/qcustomplot.h"

#include <Data/ScalarSeries.h>
#include <Data/SpectrogramSeries.h>
#include <Data/VectorSeries.h>

#include <Variable/Variable.h>

Q_LOGGING_CATEGORY(LOG_VisualizationGraphHelper, "VisualizationGraphHelper")

namespace {

class SqpDataContainer : public QCPGraphDataContainer {
public:
    void appendGraphData(const QCPGraphData &data) { mData.append(data); }
};

/**
 * Struct used to create plottables, depending on the type of the data series from which to create
 * them
 * @tparam T the data series' type
 * @remarks Default implementation can't create plottables
 */
template <typename T, typename Enabled = void>
struct PlottablesCreator {
    static PlottablesMap createPlottables(T &, QCustomPlot &)
    {
        qCCritical(LOG_DataSeries())
            << QObject::tr("Can't create plottables: unmanaged data series type");
        return {};
    }
};

/**
 * Specialization of PlottablesCreator for scalars and vectors
 * @sa ScalarSeries
 * @sa VectorSeries
 */
template <typename T>
struct PlottablesCreator<T,
                         typename std::enable_if_t<std::is_base_of<ScalarSeries, T>::value
                                                   or std::is_base_of<VectorSeries, T>::value> > {
    static PlottablesMap createPlottables(T &dataSeries, QCustomPlot &plot)
    {
        PlottablesMap result{};

        // Gets the number of components of the data series
        dataSeries.lockRead();
        auto componentCount = dataSeries.valuesData()->componentCount();
        dataSeries.unlock();

        // For each component of the data series, creates a QCPGraph to add to the plot
        for (auto i = 0; i < componentCount; ++i) {
            auto graph = plot.addGraph();
            result.insert({i, graph});
        }

        plot.replot();

        return result;
    }
};

/**
 * Specialization of PlottablesCreator for spectrograms
 * @sa SpectrogramSeries
 */
template <typename T>
struct PlottablesCreator<T,
                         typename std::enable_if_t<std::is_base_of<SpectrogramSeries, T>::value> > {
    static PlottablesMap createPlottables(T &dataSeries, QCustomPlot &plot)
    {
        PlottablesMap result{};
        result.insert({0, new QCPColorMap{plot.xAxis, plot.yAxis}});

        plot.replot();

        return result;
    }
};

/**
 * Struct used to update plottables, depending on the type of the data series from which to update
 * them
 * @tparam T the data series' type
 * @remarks Default implementation can't update plottables
 */
template <typename T, typename Enabled = void>
struct PlottablesUpdater {
    static void setPlotYAxisRange(T &, const SqpRange &, QCustomPlot &)
    {
        qCCritical(LOG_VisualizationGraphHelper())
            << QObject::tr("Can't set plot y-axis range: unmanaged data series type");
    }

    static void updatePlottables(T &, PlottablesMap &, const SqpRange &, bool)
    {
        qCCritical(LOG_VisualizationGraphHelper())
            << QObject::tr("Can't update plottables: unmanaged data series type");
    }
};

/**
 * Specialization of PlottablesUpdater for scalars and vectors
 * @sa ScalarSeries
 * @sa VectorSeries
 */
template <typename T>
struct PlottablesUpdater<T,
                         typename std::enable_if_t<std::is_base_of<ScalarSeries, T>::value
                                                   or std::is_base_of<VectorSeries, T>::value> > {
    static void setPlotYAxisRange(T &dataSeries, const SqpRange &xAxisRange, QCustomPlot &plot)
    {
        auto minValue = 0., maxValue = 0.;

        dataSeries.lockRead();
        auto valuesBounds = dataSeries.valuesBounds(xAxisRange.m_TStart, xAxisRange.m_TEnd);
        auto end = dataSeries.cend();
        if (valuesBounds.first != end && valuesBounds.second != end) {
            auto rangeValue = [](const auto &value) { return std::isnan(value) ? 0. : value; };

            minValue = rangeValue(valuesBounds.first->minValue());
            maxValue = rangeValue(valuesBounds.second->maxValue());
        }
        dataSeries.unlock();

        plot.yAxis->setRange(QCPRange{minValue, maxValue});
    }

    static void updatePlottables(T &dataSeries, PlottablesMap &plottables, const SqpRange &range,
                                 bool rescaleAxes)
    {

        // For each plottable to update, resets its data
        std::map<int, QSharedPointer<SqpDataContainer> > dataContainers{};
        for (const auto &plottable : plottables) {
            if (auto graph = dynamic_cast<QCPGraph *>(plottable.second)) {
                auto dataContainer = QSharedPointer<SqpDataContainer>::create();
                graph->setData(dataContainer);

                dataContainers.insert({plottable.first, dataContainer});
            }
        }
        dataSeries.lockRead();

        // - Gets the data of the series included in the current range
        // - Updates each plottable by adding, for each data item, a point that takes x-axis data
        // and value data. The correct value is retrieved according to the index of the component
        auto subDataIts = dataSeries.xAxisRange(range.m_TStart, range.m_TEnd);
        for (auto it = subDataIts.first; it != subDataIts.second; ++it) {
            for (const auto &dataContainer : dataContainers) {
                auto componentIndex = dataContainer.first;
                dataContainer.second->appendGraphData(
                    QCPGraphData(it->x(), it->value(componentIndex)));
            }
        }

        dataSeries.unlock();

        if (!plottables.empty()) {
            auto plot = plottables.begin()->second->parentPlot();

            if (rescaleAxes) {
                plot->rescaleAxes();
            }

            plot->replot();
        }
    }
};

/**
 * Specialization of PlottablesUpdater for spectrograms
 * @sa SpectrogramSeries
 */
template <typename T>
struct PlottablesUpdater<T,
                         typename std::enable_if_t<std::is_base_of<SpectrogramSeries, T>::value> > {
    static void setPlotYAxisRange(T &dataSeries, const SqpRange &xAxisRange, QCustomPlot &plot)
    {
        double min, max;
        /// @todo ALX: use iterators here
        std::tie(min, max) = dataSeries.yAxis().bounds();

        if (!std::isnan(min) && !std::isnan(max)) {
            plot.yAxis->setRange(QCPRange{min, max});
        }
    }

    static void updatePlottables(T &dataSeries, PlottablesMap &plottables, const SqpRange &range,
                                 bool rescaleAxes)
    {
        if (plottables.empty()) {
            qCDebug(LOG_VisualizationGraphHelper())
                << QObject::tr("Can't update spectrogram: no colormap has been associated");
            return;
        }

        // Gets the colormap to update (normally there is only one colormap)
        Q_ASSERT(plottables.size() == 1);
        auto colormap = dynamic_cast<QCPColorMap *>(plottables.at(0));
        Q_ASSERT(colormap != nullptr);

        dataSeries.lockRead();

        auto its = dataSeries.xAxisRange(range.m_TStart, range.m_TEnd);
        /// @todo ALX: use iterators here
        auto yAxis = dataSeries.yAxis();

        // Gets properties of x-axis and y-axis to set size and range of the colormap
        auto nbX = std::distance(its.first, its.second);
        auto xMin = nbX != 0 ? its.first->x() : 0.;
        auto xMax = nbX != 0 ? (its.second - 1)->x() : 0.;

        auto nbY = yAxis.size();
        auto yMin = 0., yMax = 0.;
        if (nbY != 0) {
            std::tie(yMin, yMax) = yAxis.bounds();
        }

        colormap->data()->setSize(nbX, nbY);
        colormap->data()->setRange(QCPRange{xMin, xMax}, QCPRange{yMin, yMax});

        // Sets values
        auto xIndex = 0;
        for (auto it = its.first; it != its.second; ++it, ++xIndex) {
            for (auto yIndex = 0; yIndex < nbY; ++yIndex) {
                auto value = it->value(yIndex);

                colormap->data()->setCell(xIndex, yIndex, value);

                // Processing spectrogram data for display in QCustomPlot
                /// For the moment, we just make the NaN values to be transparent in the colormap
                /// @todo ALX: complete treatments (mesh generation, etc.)
                if (std::isnan(value)) {
                    colormap->data()->setAlpha(xIndex, yIndex, 0);
                }
            }
        }

        dataSeries.unlock();

        // Rescales axes
        auto plot = colormap->parentPlot();

        if (rescaleAxes) {
            plot->rescaleAxes();
        }

        plot->replot();
    }
};

/**
 * Helper used to create/update plottables
 */
struct IPlottablesHelper {
    virtual ~IPlottablesHelper() noexcept = default;
    virtual PlottablesMap create(QCustomPlot &plot) const = 0;
    virtual void setYAxisRange(const SqpRange &xAxisRange, QCustomPlot &plot) const = 0;
    virtual void update(PlottablesMap &plottables, const SqpRange &range,
                        bool rescaleAxes = false) const = 0;
};

/**
 * Default implementation of IPlottablesHelper, which takes data series to create/update plottables
 * @tparam T the data series' type
 */
template <typename T>
struct PlottablesHelper : public IPlottablesHelper {
    explicit PlottablesHelper(T &dataSeries) : m_DataSeries{dataSeries} {}

    PlottablesMap create(QCustomPlot &plot) const override
    {
        return PlottablesCreator<T>::createPlottables(m_DataSeries, plot);
    }

    void update(PlottablesMap &plottables, const SqpRange &range, bool rescaleAxes) const override
    {
        PlottablesUpdater<T>::updatePlottables(m_DataSeries, plottables, range, rescaleAxes);
    }

    void setYAxisRange(const SqpRange &xAxisRange, QCustomPlot &plot) const override
    {
        return PlottablesUpdater<T>::setPlotYAxisRange(m_DataSeries, xAxisRange, plot);
    }

    T &m_DataSeries;
};

/// Creates IPlottablesHelper according to a data series
std::unique_ptr<IPlottablesHelper> createHelper(std::shared_ptr<IDataSeries> dataSeries) noexcept
{
    if (auto scalarSeries = std::dynamic_pointer_cast<ScalarSeries>(dataSeries)) {
        return std::make_unique<PlottablesHelper<ScalarSeries> >(*scalarSeries);
    }
    else if (auto spectrogramSeries = std::dynamic_pointer_cast<SpectrogramSeries>(dataSeries)) {
        return std::make_unique<PlottablesHelper<SpectrogramSeries> >(*spectrogramSeries);
    }
    else if (auto vectorSeries = std::dynamic_pointer_cast<VectorSeries>(dataSeries)) {
        return std::make_unique<PlottablesHelper<VectorSeries> >(*vectorSeries);
    }
    else {
        return std::make_unique<PlottablesHelper<IDataSeries> >(*dataSeries);
    }
}

} // namespace

PlottablesMap VisualizationGraphHelper::create(std::shared_ptr<Variable> variable,
                                               QCustomPlot &plot) noexcept
{
    if (variable) {
        auto helper = createHelper(variable->dataSeries());
        auto plottables = helper->create(plot);
        return plottables;
    }
    else {
        qCDebug(LOG_VisualizationGraphHelper())
            << QObject::tr("Can't create graph plottables : the variable is null");
        return PlottablesMap{};
    }
}

void VisualizationGraphHelper::setYAxisRange(std::shared_ptr<Variable> variable,
                                             QCustomPlot &plot) noexcept
{
    if (variable) {
        auto helper = createHelper(variable->dataSeries());
        helper->setYAxisRange(variable->range(), plot);
    }
    else {
        qCDebug(LOG_VisualizationGraphHelper())
            << QObject::tr("Can't set y-axis range of plot: the variable is null");
    }
}

void VisualizationGraphHelper::updateData(PlottablesMap &plottables,
                                          std::shared_ptr<IDataSeries> dataSeries,
                                          const SqpRange &dateTime)
{
    auto helper = createHelper(dataSeries);
    helper->update(plottables, dateTime);
}
