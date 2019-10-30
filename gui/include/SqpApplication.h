#ifndef SCIQLOP_SQPAPPLICATION_H
#define SCIQLOP_SQPAPPLICATION_H

#include "SqpApplication.h"

#include <QAction>
#include <QApplication>
#include <QLoggingCategory>
#include <QMenuBar>
#include <QProxyStyle>
#include <QStyleOption>
#include <QWidget>
#include <QWidgetAction>

#include <Common/spimpl.h>

Q_DECLARE_LOGGING_CATEGORY(LOG_SqpApplication)

#if defined(sqpApp)
#undef sqpApp
#endif
#define sqpApp (static_cast<SqpApplication*>(QCoreApplication::instance()))

class DataSourceController;
class DataSources;
class NetworkController;
class TimeController;
class VariableController;
class VariableController2;
class VariableModel2;
class DragDropGuiController;
class ActionsGuiController;
class CatalogueController;

/* stolen from here https://forum.qt.io/topic/90403/show-tooltip-immediatly/6 */
class MyProxyStyle : public QProxyStyle
{
public:
    using QProxyStyle::QProxyStyle;

    int styleHint(StyleHint hint, const QStyleOption* option = nullptr,
        const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
    {
        if (hint == QStyle::SH_ToolButton_PopupDelay && widget
            /*&& widget->inherits(QWidgetAction::staticMetaObject.className())*/)
        {
            return 0;
        }

        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

/**
 * @brief The SqpApplication class aims to make the link between SciQlop
 * and its plugins. This is the intermediate class that SciQlop has to use
 * in the way to connect a data source. Please first use load method to initialize
 * a plugin specified by its metadata name (JSON plugin source) then others specifics
 * method will be able to access it.
 * You can load a data source driver plugin then create a data source.
 */

class SqpApplication : public QApplication
{
    Q_OBJECT
public:
    explicit SqpApplication(int& argc, char** argv);
    ~SqpApplication() override;
    void initialize();

    /// Accessors for the differents sciqlop controllers
    //DataSourceController& dataSourceController() noexcept;
    DataSources& dataSources() noexcept;
    NetworkController& networkController() noexcept;
    TimeController& timeController() noexcept;
    VariableController2& variableController() noexcept;
    std::shared_ptr<VariableController2> variableControllerOwner() noexcept;
    CatalogueController& catalogueController() noexcept;

    /// Accessors for the differents sciqlop helpers, these helpers classes are like controllers but
    /// doesn't live in a thread and access gui
    DragDropGuiController& dragDropGuiController() noexcept;
    ActionsGuiController& actionsGuiController() noexcept;

    enum class PlotsInteractionMode
    {
        None,
        ZoomBox,
        DragAndDrop,
        SelectionZones
    };

    enum class PlotsCursorMode
    {
        NoCursor,
        Vertical,
        Temporal,
        Horizontal,
        Cross
    };

    PlotsInteractionMode plotsInteractionMode() const;
    void setPlotsInteractionMode(PlotsInteractionMode mode);

    PlotsCursorMode plotsCursorMode() const;
    void setPlotsCursorMode(PlotsCursorMode mode);

private:
    class SqpApplicationPrivate;
    spimpl::unique_impl_ptr<SqpApplicationPrivate> impl;
};

inline SqpApplication* SqpApplication_ctor()
{
    static int argc;
    static char** argv;
    return new SqpApplication(argc, argv);
}

#endif // SCIQLOP_SQPAPPLICATION_H
