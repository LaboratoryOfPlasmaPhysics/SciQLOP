#pragma once
#include <Data/DataProviderParameters.h>
#include <Data/DataSeriesType.h>
#include <Data/IDataProvider.h>
#include <DataSource/DataSourceController.h>
#include <DataSource/DataSourceItem.h>
#include <DataSource/DataSourceItemAction.h>
#include <QPair>
#include <SqpApplication.h>
// must be included last because of Python/Qt definition of slots
#include "numpy_wrappers.h"

struct Product
{
    QString path;
    std::vector<std::string> components;
    QMap<QString, QString> metadata;
    Product() = default;
    explicit Product(const QString& path, const std::vector<std::string>& components,
        const QMap<QString, QString>& metadata)
            : path { path }, components { components }, metadata { metadata }
    {
    }
    ~Product() = default;
};

class PyDataProvider : public IDataProvider
{
public:
    PyDataProvider()
    {
        auto& dataSourceController = sqpApp->dataSourceController();
        dataSourceController.registerProvider(this);
    }

    virtual ~PyDataProvider() {}

    virtual QPair<QPair<NpArray,NpArray>,DataSeriesType> get_data(const QMap<QString,QString>& key, double start_time, double stop_time)
    {
        (void)key, (void)start_time, (void)stop_time;
        return {};
    }

    virtual TimeSeries::ITimeSerie* getData(const DataProviderParameters& parameters) override
    {
        if (parameters.m_Data.contains("name"))
        {
            QMap<QString,QString> metadata;
            std::for_each(parameters.m_Data.constKeyValueBegin(), parameters.m_Data.constKeyValueEnd(), [&metadata](const auto& item) {
                metadata[item.first] = item.second.toString();
                });
            auto [data, type] = get_data(metadata,
                parameters.m_Range.m_TStart, parameters.m_Range.m_TEnd);
            // TODO add shape/type switch
            //if (builder)
            {
                auto& [t,y]=data;
                switch (type)
                {
                    case DataSeriesType::SCALAR:
                        return new ScalarTimeSerie { std::move(t.data),
                            std::move(y.data) };
                        break;
                    case DataSeriesType::VECTOR:
                        return new VectorTimeSerie { std::move(t.data),
                            y.to_std_vect_vect() };
                        break;
                    case DataSeriesType::MULTICOMPONENT:
                    {
                        auto y_size = y.flat_size();
                        auto t_size = t.flat_size();

                        if(t_size && (y_size%t_size)==0)
                        {
                            return new MultiComponentTimeSerie { std::move(t.data),
                                std::move(y.data),{t_size, y_size/t_size} };
                        }
                        break;
                    }
                    case DataSeriesType::SPECTROGRAM:
                    {
                        auto y_size = y.flat_size();
                        auto t_size = t.flat_size();

                        if(t_size && (y_size%t_size)==0)
                        {
                            return new SpectrogramTimeSerie { std::move(t.data),
                                std::move(y.data),{t_size, y_size/t_size} };
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
        return nullptr;
    }


    inline void register_products(const QVector<Product*>& products)
    {
        auto& dataSourceController = sqpApp->dataSourceController();
        auto id = this->id();
        auto data_source_name = this->name();
        std::for_each(std::cbegin(products), std::cend(products),
            [&id, &dataSourceController](const Product* product) {
                dataSourceController.setDataSourceItem(id, product->path, product->metadata);
            });
    }
};
