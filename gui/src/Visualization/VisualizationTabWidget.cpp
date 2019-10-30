#include "Visualization/VisualizationTabWidget.h"
#include "Visualization/IVisualizationWidgetVisitor.h"
#include "ui_VisualizationTabWidget.h"

#include "Visualization/VisualizationGraphWidget.h"
#include "Visualization/VisualizationZoneWidget.h"

#include "Visualization/MacScrollBarStyle.h"

#include "DataSource/datasources.h"
#include "Variable/VariableController2.h"

#include "MimeTypes/MimeTypes.h"

#include "DragAndDrop/DragDropGuiController.h"
#include "SqpApplication.h"

Q_LOGGING_CATEGORY(LOG_VisualizationTabWidget, "VisualizationTabWidget")

namespace
{

/**
 * Applies a function to all zones of the tab represented by its layout
 * @param layout the layout that contains zones
 * @param fun the function to apply to each zone
 */
template <typename Fun>
void processZones(QLayout& layout, Fun fun)
{
    for (auto i = 0; i < layout.count(); ++i)
    {
        if (auto item = layout.itemAt(i))
        {
            if (auto visualizationZoneWidget
                = qobject_cast<VisualizationZoneWidget*>(item->widget()))
            {
                fun(*visualizationZoneWidget);
            }
        }
    }
}

/// Generates a default name for a new zone, according to the number of zones already displayed in
/// the tab
QString defaultZoneName(QLayout& layout)
{
    QSet<QString> existingNames;
    processZones(
        layout, [&existingNames](auto& zoneWidget) { existingNames.insert(zoneWidget.name()); });

    int zoneNum = 1;
    QString name;
    do
    {
        name = QObject::tr("Zone ").append(QString::number(zoneNum));
        ++zoneNum;
    } while (existingNames.contains(name));

    return name;
}

} // namespace

struct VisualizationTabWidget::VisualizationTabWidgetPrivate
{
    explicit VisualizationTabWidgetPrivate(const QString& name) : m_Name { name } {}

    QString m_Name;

#ifdef Q_OS_MAC
    std::unique_ptr<MacScrollBarStyle> m_MacScrollBarStyle = std::make_unique<MacScrollBarStyle>();
#endif

    void dropGraph(int index, VisualizationTabWidget* tabWidget);
    void dropZone(int index, VisualizationTabWidget* tabWidget);
    void dropVariables(const std::vector<std::shared_ptr<Variable2>>& variables, int index,
        VisualizationTabWidget* tabWidget);
    void dropProducts(
        const QVariantList& productsMetaData, int index, VisualizationTabWidget* tabWidget);
};

VisualizationTabWidget::VisualizationTabWidget(const QString& name, QWidget* parent)
        : QWidget { parent }
        , ui { new Ui::VisualizationTabWidget }
        , impl { spimpl::make_unique_impl<VisualizationTabWidgetPrivate>(name) }
{
    ui->setupUi(this);

#ifdef Q_OS_MAC
    impl->m_MacScrollBarStyle->selfInstallOn(ui->scrollArea, true);
#endif

    ui->dragDropContainer->setPlaceHolderType(DragDropGuiController::PlaceHolderType::Zone, "Zone");
    ui->dragDropContainer->layout()->setContentsMargins(0, 0, 0, 12);
    ui->dragDropContainer->layout()->setSpacing(0);
    ui->dragDropContainer->setMimeType(
        MIME::MIME_TYPE_GRAPH, VisualizationDragDropContainer::DropBehavior::Inserted);
    ui->dragDropContainer->setMimeType(
        MIME::MIME_TYPE_ZONE, VisualizationDragDropContainer::DropBehavior::Inserted);
    ui->dragDropContainer->setMimeType(
        MIME::MIME_TYPE_VARIABLE_LIST, VisualizationDragDropContainer::DropBehavior::Inserted);
    ui->dragDropContainer->setMimeType(
        MIME::MIME_TYPE_PRODUCT_LIST, VisualizationDragDropContainer::DropBehavior::Inserted);

    ui->dragDropContainer->setAcceptMimeDataFunction([this](auto mimeData) {
        return sqpApp->dragDropGuiController().checkMimeDataForVisualization(
            mimeData, ui->dragDropContainer);
    });

    connect(ui->dragDropContainer, &VisualizationDragDropContainer::dropOccuredInContainer, this,
        &VisualizationTabWidget::dropMimeData);

    sqpApp->dragDropGuiController().addDragDropScrollArea(ui->scrollArea);

    // Widget is deleted when closed
    setAttribute(Qt::WA_DeleteOnClose);
}

VisualizationTabWidget::~VisualizationTabWidget()
{
    sqpApp->dragDropGuiController().removeDragDropScrollArea(ui->scrollArea);
    delete ui;
}

void VisualizationTabWidget::addZone(VisualizationZoneWidget* zoneWidget)
{
    ui->dragDropContainer->addDragWidget(zoneWidget);
}

void VisualizationTabWidget::insertZone(int index, VisualizationZoneWidget* zoneWidget)
{
    ui->dragDropContainer->insertDragWidget(index, zoneWidget);
}

QStringList VisualizationTabWidget::availableZoneWidgets() const
{
    QStringList zones;
    processZones(
        tabLayout(), [&zones](VisualizationZoneWidget& zoneWidget) { zones << zoneWidget.name(); });

    return zones;
}

VisualizationZoneWidget* VisualizationTabWidget::getZoneWithName(const QString& zoneName)
{
    VisualizationZoneWidget* result = nullptr;
    processZones(tabLayout(), [&zoneName, &result](VisualizationZoneWidget& zoneWidget) {
        if (!result && zoneWidget.name() == zoneName)
        {
            result = &zoneWidget;
        }
    });

    return result;
}

VisualizationZoneWidget* VisualizationTabWidget::createZone(std::shared_ptr<Variable2> variable)
{
    return createZone({ variable }, -1);
}

VisualizationZoneWidget* VisualizationTabWidget::createZone(
    const std::vector<std::shared_ptr<Variable2>>& variables, int index)
{
    auto zoneWidget = createEmptyZone(index);

    // Creates a new graph into the zone
    zoneWidget->createGraph(variables, index);

    return zoneWidget;
}

VisualizationZoneWidget* VisualizationTabWidget::createEmptyZone(int index)
{
    auto zoneWidget
        = new VisualizationZoneWidget { defaultZoneName(*ui->dragDropContainer->layout()), this };
    this->insertZone(index, zoneWidget);

    return zoneWidget;
}

void VisualizationTabWidget::accept(IVisualizationWidgetVisitor* visitor)
{
    if (visitor)
    {
        visitor->visitEnter(this);

        // Apply visitor to zone children: widgets different from zones are not visited (no action)
        processZones(tabLayout(),
            [visitor](VisualizationZoneWidget& zoneWidget) { zoneWidget.accept(visitor); });

        visitor->visitLeave(this);
    }
    else
    {
        qCCritical(LOG_VisualizationTabWidget()) << tr("Can't visit widget : the visitor is null");
    }
}

bool VisualizationTabWidget::canDrop(Variable2& variable) const
{
    // A tab can always accomodate a variable
    Q_UNUSED(variable);
    return true;
}

bool VisualizationTabWidget::contains(Variable2& variable) const
{
    Q_UNUSED(variable);
    return false;
}

QString VisualizationTabWidget::name() const
{
    return impl->m_Name;
}

void VisualizationTabWidget::closeEvent(QCloseEvent* event)
{
    // Closes zones in the tab
    processZones(tabLayout(), [](VisualizationZoneWidget& zoneWidget) { zoneWidget.close(); });

    QWidget::closeEvent(event);
}

QLayout& VisualizationTabWidget::tabLayout() const noexcept
{
    return *ui->dragDropContainer->layout();
}

void VisualizationTabWidget::dropMimeData(int index, const QMimeData* mimeData)
{
    if (mimeData->hasFormat(MIME::MIME_TYPE_GRAPH))
    {
        impl->dropGraph(index, this);
    }
    else if (mimeData->hasFormat(MIME::MIME_TYPE_ZONE))
    {
        impl->dropZone(index, this);
    }
    else if (mimeData->hasFormat(MIME::MIME_TYPE_VARIABLE_LIST))
    {
        auto variables = sqpApp->variableController().variables(
            Variable2::IDs(mimeData->data(MIME::MIME_TYPE_VARIABLE_LIST)));
        impl->dropVariables(variables, index, this);
    }
    else if (mimeData->hasFormat(MIME::MIME_TYPE_PRODUCT_LIST))
    {
        auto productsData = MIME::decode(
            mimeData->data(MIME::MIME_TYPE_PRODUCT_LIST));
        impl->dropProducts(productsData, index, this);
    }
    else
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropMimeData, unknown MIME data received.");
    }
}

void VisualizationTabWidget::VisualizationTabWidgetPrivate::dropGraph(
    int index, VisualizationTabWidget* tabWidget)
{
    auto& helper = sqpApp->dragDropGuiController();

    auto graphWidget = qobject_cast<VisualizationGraphWidget*>(helper.getCurrentDragWidget());
    if (!graphWidget)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropGraph, drop aborted, the dropped graph is not "
                  "found or invalid.");
        Q_ASSERT(false);
        return;
    }

    auto parentDragDropContainer
        = qobject_cast<VisualizationDragDropContainer*>(graphWidget->parentWidget());
    if (!parentDragDropContainer)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropGraph, drop aborted, the parent container of "
                  "the dropped graph is not found.");
        Q_ASSERT(false);
        return;
    }

    auto nbGraph = parentDragDropContainer->countDragWidget();

    const auto& variables = graphWidget->variables();

    if (!variables.empty())
    {
        // Abort the requests for the variables (if any)
        // Commented, because it's not sure if it's needed or not
        // for (const auto& var : variables)
        //{
        //    sqpApp->variableController().onAbortProgressRequested(var);
        //}

        if (nbGraph == 1)
        {
            // This is the only graph in the previous zone, close the zone
            helper.delayedCloseWidget(graphWidget->parentZoneWidget());
        }
        else
        {
            // Close the graph
            helper.delayedCloseWidget(graphWidget);
        }

        auto zoneWidget = tabWidget->createZone(variables, index);
        auto firstGraph = zoneWidget->firstGraph();
        if (firstGraph)
        {
            firstGraph->addSelectionZones(graphWidget->selectionZoneRanges());
        }
        else
        {
            qCWarning(LOG_VisualizationZoneWidget())
                << tr("VisualizationTabWidget::dropGraph, no graph added in the widget.");
            Q_ASSERT(false);
        }
    }
    else
    {
        // The graph is empty, create an empty zone and move the graph inside

        auto parentZoneWidget = graphWidget->parentZoneWidget();

        parentDragDropContainer->layout()->removeWidget(graphWidget);

        auto zoneWidget = tabWidget->createEmptyZone(index);
        zoneWidget->addGraph(graphWidget);

        // Close the old zone if it was the only graph inside
        if (nbGraph == 1)
        {
            helper.delayedCloseWidget(parentZoneWidget);
        }
    }
}

void VisualizationTabWidget::VisualizationTabWidgetPrivate::dropZone(
    int index, VisualizationTabWidget* tabWidget)
{
    auto& helper = sqpApp->dragDropGuiController();

    auto zoneWidget = qobject_cast<VisualizationZoneWidget*>(helper.getCurrentDragWidget());
    if (!zoneWidget)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropZone, drop aborted, the dropped zone is not "
                  "found or invalid.");
        Q_ASSERT(false);
        return;
    }

    auto parentDragDropContainer
        = qobject_cast<VisualizationDragDropContainer*>(zoneWidget->parentWidget());
    if (!parentDragDropContainer)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropZone, drop aborted, the parent container of "
                  "the dropped zone is not found.");
        Q_ASSERT(false);
        return;
    }

    // Simple move of the zone, no variable operation associated
    parentDragDropContainer->layout()->removeWidget(zoneWidget);
    tabWidget->ui->dragDropContainer->insertDragWidget(index, zoneWidget);
}

void VisualizationTabWidget::VisualizationTabWidgetPrivate::dropVariables(
    const std::vector<std::shared_ptr<Variable2>>& variables, int index,
    VisualizationTabWidget* tabWidget)
{
    // Note: the AcceptMimeDataFunction (set on the drop container) ensure there is a single and
    // compatible variable here
    if (variables.size() > 1)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropVariables, dropping multiple variables, operation "
                  "aborted.");
        return;
    }

    tabWidget->createZone(variables, index);
}

void VisualizationTabWidget::VisualizationTabWidgetPrivate::dropProducts(
    const QVariantList& productsMetaData, int index, VisualizationTabWidget* tabWidget)
{
    // Note: the AcceptMimeDataFunction (set on the drop container) ensure there is a single and
    // compatible variable here
    if (productsMetaData.count() != 1)
    {
        qCWarning(LOG_VisualizationZoneWidget())
            << tr("VisualizationTabWidget::dropProducts, dropping multiple products, operation "
                  "aborted.");
        return;
    }

    auto context = new QObject { tabWidget };
    connect(&sqpApp->variableController(), &VariableController2::variableAdded, context,
        [this, index, tabWidget, context](auto variable) {
            tabWidget->createZone({ variable }, index);
            delete context; // removes the connection
        },
        Qt::QueuedConnection);

    auto productPath = productsMetaData.first().toString();
    QMetaObject::invokeMethod(&sqpApp->dataSources(), "createVariable",
                              Qt::QueuedConnection, Q_ARG(QString, productPath));
}
