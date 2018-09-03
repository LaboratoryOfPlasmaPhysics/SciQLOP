#include "CosinusProvider.h"
#include "MockDefs.h"

#include <Data/DataProviderParameters.h>
#include <Data/ScalarSeries.h>
#include <Data/SpectrogramSeries.h>
#include <Data/VectorSeries.h>

#include <cmath>
#include <set>

#include <QFuture>
#include <QThread>
#include <QtConcurrent/QtConcurrent>


namespace {

/// Number of bands generated for a spectrogram
const auto SPECTROGRAM_NUMBER_BANDS = 30;

/// Bands for which to generate NaN values for a spectrogram
const auto SPECTROGRAM_NAN_BANDS = std::set<int>{1, 3, 10, 20};

/// Bands for which to generate zeros for a spectrogram
const auto SPECTROGRAM_ZERO_BANDS = std::set<int>{2, 15, 19, 29};

/// Abstract cosinus type
struct ICosinusType {
    virtual ~ICosinusType() = default;
    /// @return the number of components generated for the type
    virtual std::size_t componentCount() const = 0;
    /// @return the data series created for the type
    virtual IDataSeries* createDataSeries(std::vector<double> xAxisData,
                                                          std::vector<double> valuesData) const = 0;
    /// Generates values (one value per component)
    /// @param x the x-axis data used to generate values
    /// @param values the vector in which to insert the generated values
    /// @param dataIndex the index of insertion of the generated values
    ///
    virtual void generateValues(double x, std::vector<double> &values, int dataIndex) const = 0;
};

struct ScalarCosinus : public ICosinusType {
    std::size_t componentCount() const override { return 1; }

    IDataSeries* createDataSeries(std::vector<double> xAxisData,
                                                  std::vector<double> valuesData) const override
    {
        return new ScalarSeries(std::move(xAxisData), std::move(valuesData),
                                              Unit{QStringLiteral("t"), true}, Unit{});
    }

    void generateValues(double x, std::vector<double> &values, int dataIndex) const override
    {
        values[dataIndex] = std::cos(x);
    }
};

struct SpectrogramCosinus : public ICosinusType {
    /// Ctor with y-axis
    explicit SpectrogramCosinus(std::vector<double> yAxisData, Unit yAxisUnit, Unit valuesUnit)
            : m_YAxisData{std::move(yAxisData)},
              m_YAxisUnit{std::move(yAxisUnit)},
              m_ValuesUnit{std::move(valuesUnit)}
    {
    }

    std::size_t componentCount() const override { return m_YAxisData.size(); }

    IDataSeries* createDataSeries(std::vector<double> xAxisData,
                                                  std::vector<double> valuesData) const override
    {
        return new SpectrogramSeries(
            std::move(xAxisData), m_YAxisData, std::move(valuesData),
            Unit{QStringLiteral("t"), true}, m_YAxisUnit, m_ValuesUnit);
    }

    void generateValues(double x, std::vector<double> &values, int dataIndex) const override
    {
        auto componentCount = this->componentCount();
        for (int i = 0; i < componentCount; ++i) {
            auto y = m_YAxisData[i];

            double value;

            if (SPECTROGRAM_ZERO_BANDS.find(y) != SPECTROGRAM_ZERO_BANDS.end()) {
                value = 0.;
            }
            else if (SPECTROGRAM_NAN_BANDS.find(y) != SPECTROGRAM_NAN_BANDS.end()) {
                value = std::numeric_limits<double>::quiet_NaN();
            }
            else {
                // Generates value for non NaN/zero bands
                auto r = 3 * std::sqrt(x * x + y * y) + 1e-2;
                value = 2 * x * (std::cos(r + 2) / r - std::sin(r + 2) / r);
            }

            values[componentCount * dataIndex + i] = value;
        }
    }

    std::vector<double> m_YAxisData;
    Unit m_YAxisUnit;
    Unit m_ValuesUnit;
};

struct VectorCosinus : public ICosinusType {
    std::size_t componentCount() const override { return 3; }

    IDataSeries* createDataSeries(std::vector<double> xAxisData,
                                                  std::vector<double> valuesData) const override
    {
        return new VectorSeries(std::move(xAxisData), std::move(valuesData),
                                              Unit{QStringLiteral("t"), true}, Unit{});
    }

    void generateValues(double x, std::vector<double> &values, int dataIndex) const override
    {
        // Generates value for each component: cos(x), cos(x)/2, cos(x)/3
        auto xValue = std::cos(x);
        auto componentCount = this->componentCount();
        for (auto i = 0; i < componentCount; ++i) {
            values[componentCount * dataIndex + i] = xValue / (i + 1);
        }
    }
};

/// Converts string to cosinus type
/// @return the cosinus type if the string could be converted, nullptr otherwise
std::unique_ptr<ICosinusType> cosinusType(const QString &type) noexcept
{
    if (type.compare(QStringLiteral("scalar"), Qt::CaseInsensitive) == 0) {
        return std::make_unique<ScalarCosinus>();
    }
    else if (type.compare(QStringLiteral("spectrogram"), Qt::CaseInsensitive) == 0) {
        // Generates default y-axis data for spectrogram [0., 1., 2., ...]
        std::vector<double> yAxisData(SPECTROGRAM_NUMBER_BANDS);
        std::iota(yAxisData.begin(), yAxisData.end(), 0.);

        return std::make_unique<SpectrogramCosinus>(std::move(yAxisData), Unit{"eV"},
                                                    Unit{"eV/(cm^2-s-sr-eV)"});
    }
    else if (type.compare(QStringLiteral("vector"), Qt::CaseInsensitive) == 0) {
        return std::make_unique<VectorCosinus>();
    }
    else {
        return nullptr;
    }
}

} // namespace

std::shared_ptr<IDataProvider> CosinusProvider::clone() const
{
    // No copy is made in clone
    return std::make_shared<CosinusProvider>();
}

IDataSeries *CosinusProvider::_generate(const DateTimeRange &range, const QVariantHash &metaData)
{
    auto dataIndex = 0;

    // Retrieves cosinus type
    auto typeVariant = metaData.value(COSINUS_TYPE_KEY, COSINUS_TYPE_DEFAULT_VALUE);
    auto type = cosinusType(typeVariant.toString());
    auto freqVariant = metaData.value(COSINUS_FREQUENCY_KEY, COSINUS_FREQUENCY_DEFAULT_VALUE);
    double freq = freqVariant.toDouble();
    double start = std::ceil(range.m_TStart * freq);
    double end = std::floor(range.m_TEnd * freq);
    if (end < start) {
        std::swap(start, end);
    }
    std::size_t dataCount = static_cast<std::size_t>(end - start + 1);
    std::size_t componentCount = type->componentCount();

    auto xAxisData = std::vector<double>{};
    xAxisData.resize(dataCount);

    auto valuesData = std::vector<double>{};
    valuesData.resize(dataCount * componentCount);

    int progress = 0;
    auto progressEnd = dataCount;
    for (auto time = start; time <= end; ++time, ++dataIndex)
    {
            const auto x = time / freq;
            xAxisData[dataIndex] = x;
            // Generates values (depending on the type)
            type->generateValues(x, valuesData, dataIndex);
    }
    return type->createDataSeries(std::move(xAxisData), std::move(valuesData));
}

IDataSeries* CosinusProvider::getData(const DataProviderParameters &parameters)
{
    return _generate(parameters.m_Range, parameters.m_Data);
}

