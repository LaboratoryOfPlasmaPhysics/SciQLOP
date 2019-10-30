#include "SqpApplication.h"

#include <Actions/ActionsGuiController.h>
#include <Catalogue/CatalogueController.h>
#include <Data/IDataProvider.h>
#include <DataSource/datasources.h>
#include <DragAndDrop/DragDropGuiController.h>
#include <Network/NetworkController.h>
#include <QThread>
#include <Time/TimeController.h>
#include <Variable/VariableController2.h>
#include <Variable/VariableModel2.h>

Q_LOGGING_CATEGORY(LOG_SqpApplication, "SqpApplication")

class SqpApplication::SqpApplicationPrivate
{
public:
    SqpApplicationPrivate()
            : m_VariableController { std::make_shared<VariableController2>() }
            , m_PlotInterractionMode(SqpApplication::PlotsInteractionMode::None)
            , m_PlotCursorMode(SqpApplication::PlotsCursorMode::NoCursor)
    {
        // /////////////////////////////// //
        // Connections between controllers //
        // /////////////////////////////// //

        // VariableController <-> DataSourceController
        connect(&m_DataSources, static_cast<void (DataSources::*)(const QString&, const QVariantHash&,
                std::shared_ptr<IDataProvider>)>(&DataSources::createVariable),
            [](const QString& variableName, const QVariantHash& variableMetadata,
                std::shared_ptr<IDataProvider> variableProvider) {
                sqpApp->variableController().createVariable(variableName, variableMetadata,
                    variableProvider, sqpApp->timeController().dateTime());
            });


        m_NetworkController.moveToThread(&m_NetworkControllerThread);
        m_NetworkControllerThread.setObjectName("NetworkControllerThread");

        // Additionnal init
        // m_VariableController->setTimeController(m_TimeController.get());
    }

    virtual ~SqpApplicationPrivate()
    {

        m_NetworkControllerThread.quit();
        m_NetworkControllerThread.wait();
    }

    DataSources m_DataSources;
    std::shared_ptr<VariableController2> m_VariableController;
    TimeController m_TimeController;
    NetworkController m_NetworkController;
    CatalogueController m_CatalogueController;

    QThread m_NetworkControllerThread;

    DragDropGuiController m_DragDropGuiController;
    ActionsGuiController m_ActionsGuiController;

    SqpApplication::PlotsInteractionMode m_PlotInterractionMode;
    SqpApplication::PlotsCursorMode m_PlotCursorMode;
};


SqpApplication::SqpApplication(int& argc, char** argv)
        : QApplication { argc, argv }, impl { spimpl::make_unique_impl<SqpApplicationPrivate>() }
{
    this->setStyle(new MyProxyStyle(this->style()));
    qCDebug(LOG_SqpApplication()) << tr("SqpApplication construction") << QThread::currentThread();

    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    connect(&impl->m_NetworkControllerThread, &QThread::started, &impl->m_NetworkController,
        &NetworkController::initialize);
    connect(&impl->m_NetworkControllerThread, &QThread::finished, &impl->m_NetworkController,
        &NetworkController::finalize);

    impl->m_NetworkControllerThread.start();
}

SqpApplication::~SqpApplication() {}

void SqpApplication::initialize() {}

//DataSourceController& SqpApplication::dataSourceController() noexcept
//{
//    return impl->m_DataSourceController;
//}

DataSources& SqpApplication::dataSources() noexcept
{
    return impl->m_DataSources;
}

NetworkController& SqpApplication::networkController() noexcept
{
    return impl->m_NetworkController;
}

TimeController& SqpApplication::timeController() noexcept
{
    return impl->m_TimeController;
}

VariableController2& SqpApplication::variableController() noexcept
{
    return *impl->m_VariableController;
}

std::shared_ptr<VariableController2> SqpApplication::variableControllerOwner() noexcept
{
    return impl->m_VariableController;
}

CatalogueController& SqpApplication::catalogueController() noexcept
{
    return impl->m_CatalogueController;
}

DragDropGuiController& SqpApplication::dragDropGuiController() noexcept
{
    return impl->m_DragDropGuiController;
}

ActionsGuiController& SqpApplication::actionsGuiController() noexcept
{
    return impl->m_ActionsGuiController;
}

SqpApplication::PlotsInteractionMode SqpApplication::plotsInteractionMode() const
{
    return impl->m_PlotInterractionMode;
}

void SqpApplication::setPlotsInteractionMode(SqpApplication::PlotsInteractionMode mode)
{
    impl->m_PlotInterractionMode = mode;
}

SqpApplication::PlotsCursorMode SqpApplication::plotsCursorMode() const
{
    return impl->m_PlotCursorMode;
}

void SqpApplication::setPlotsCursorMode(SqpApplication::PlotsCursorMode mode)
{
    impl->m_PlotCursorMode = mode;
}
