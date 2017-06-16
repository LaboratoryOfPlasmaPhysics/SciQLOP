#ifndef SCIQLOP_IDATASERIES_H
#define SCIQLOP_IDATASERIES_H

#include <QString>

#include <memory>

template <int Dim>
class ArrayData;

struct Unit {
    explicit Unit(const QString &name = {}, bool timeUnit = false)
            : m_Name{name}, m_TimeUnit{timeUnit}
    {
    }

    QString m_Name;  ///< Unit name
    bool m_TimeUnit; ///< The unit is a unit of time
};

/**
 * @brief The IDataSeries aims to declare a data series.
 *
 * A data series is an entity that contains at least :
 * - one dataset representing the x-axis
 * - one dataset representing the values
 *
 * Each dataset is represented by an ArrayData, and is associated with a unit.
 *
 * An ArrayData can be unidimensional or two-dimensional, depending on the implementation of the
 * IDataSeries. The x-axis dataset is always unidimensional.
 *
 * @sa ArrayData
 */
class IDataSeries {
public:
    virtual ~IDataSeries() noexcept = default;

    /// Returns the x-axis dataset
    virtual std::shared_ptr<ArrayData<1> > xAxisData() = 0;

    virtual Unit xAxisUnit() const = 0;

    virtual Unit valuesUnit() const = 0;
};

#endif // SCIQLOP_IDATASERIES_H
