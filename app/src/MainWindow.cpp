/*------------------------------------------------------------------------------
-- This file is a part of the SciQLop Software
-- Copyright (C) 2017, Plasma Physics Laboratory - CNRS
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
-------------------------------------------------------------------------------*/
/*-- Author : Alexis Jeandet
-- Mail : alexis.jeandet@member.fsf.org
----------------------------------------------------------------------------*/
#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <DataSource/DataSourceController.h>
#include <DataSource/DataSourceWidget.h>
#include <SidePane/SqpSidePane.h>
#include <SqpApplication.h>
#include <Time/TimeController.h>
#include <TimeWidget/TimeWidget.h>
#include <Variable/Variable.h>
#include <Variable/VariableController.h>
#include <Visualization/VisualizationController.h>

#include <QAction>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QToolBar>
#include <QToolButton>
#include <memory.h>

#include "iostream"

Q_LOGGING_CATEGORY(LOG_MainWindow, "MainWindow")

namespace {
const auto LEFTMAININSPECTORWIDGETSPLITTERINDEX = 0;
const auto LEFTINSPECTORSIDEPANESPLITTERINDEX = 1;
const auto VIEWPLITTERINDEX = 2;
const auto RIGHTINSPECTORSIDEPANESPLITTERINDEX = 3;
const auto RIGHTMAININSPECTORWIDGETSPLITTERINDEX = 4;
}

class MainWindow::MainWindowPrivate {
public:
    QSize m_LastOpenLeftInspectorSize;
    QSize m_LastOpenRightInspectorSize;
};

MainWindow::MainWindow(QWidget *parent)
        : QMainWindow{parent},
          m_Ui{new Ui::MainWindow},
          impl{spimpl::make_unique_impl<MainWindowPrivate>()}
{
    m_Ui->setupUi(this);

    m_Ui->splitter->setCollapsible(LEFTINSPECTORSIDEPANESPLITTERINDEX, false);
    m_Ui->splitter->setCollapsible(RIGHTINSPECTORSIDEPANESPLITTERINDEX, false);


    auto leftSidePane = m_Ui->leftInspectorSidePane->sidePane();
    auto openLeftInspectorAction = new QAction{QIcon{
                                                   ":/icones/previous.png",
                                               },
                                               tr("Show/hide the left inspector"), this};


    auto spacerLeftTop = new QWidget{};
    spacerLeftTop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto spacerLeftBottom = new QWidget{};
    spacerLeftBottom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    leftSidePane->addWidget(spacerLeftTop);
    leftSidePane->addAction(openLeftInspectorAction);
    leftSidePane->addWidget(spacerLeftBottom);


    auto rightSidePane = m_Ui->rightInspectorSidePane->sidePane();
    auto openRightInspectorAction = new QAction{QIcon{
                                                    ":/icones/next.png",
                                                },
                                                tr("Show/hide the right inspector"), this};

    auto spacerRightTop = new QWidget{};
    spacerRightTop->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto spacerRightBottom = new QWidget{};
    spacerRightBottom->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    rightSidePane->addWidget(spacerRightTop);
    rightSidePane->addAction(openRightInspectorAction);
    rightSidePane->addWidget(spacerRightBottom);

    openLeftInspectorAction->setCheckable(true);
    openRightInspectorAction->setCheckable(true);

    auto openInspector = [this](bool checked, bool right, auto action) {

        action->setIcon(QIcon{(checked xor right) ? ":/icones/next.png" : ":/icones/previous.png"});

        auto &lastInspectorSize
            = right ? impl->m_LastOpenRightInspectorSize : impl->m_LastOpenLeftInspectorSize;

        auto nextInspectorSize = right ? m_Ui->rightMainInspectorWidget->size()
                                       : m_Ui->leftMainInspectorWidget->size();

        // Update of the last opened geometry
        if (checked) {
            lastInspectorSize = nextInspectorSize;
        }

        auto startSize = lastInspectorSize;
        auto endSize = startSize;
        endSize.setWidth(0);

        auto splitterInspectorIndex
            = right ? RIGHTMAININSPECTORWIDGETSPLITTERINDEX : LEFTMAININSPECTORWIDGETSPLITTERINDEX;

        auto currentSizes = m_Ui->splitter->sizes();
        if (checked) {
            // adjust sizes individually here, e.g.
            currentSizes[splitterInspectorIndex] -= lastInspectorSize.width();
            currentSizes[VIEWPLITTERINDEX] += lastInspectorSize.width();
            m_Ui->splitter->setSizes(currentSizes);
        }
        else {
            // adjust sizes individually here, e.g.
            currentSizes[splitterInspectorIndex] += lastInspectorSize.width();
            currentSizes[VIEWPLITTERINDEX] -= lastInspectorSize.width();
            m_Ui->splitter->setSizes(currentSizes);
        }

    };


    connect(openLeftInspectorAction, &QAction::triggered,
            [openInspector, openLeftInspectorAction](bool checked) {
                openInspector(checked, false, openLeftInspectorAction);
            });
    connect(openRightInspectorAction, &QAction::triggered,
            [openInspector, openRightInspectorAction](bool checked) {
                openInspector(checked, true, openRightInspectorAction);
            });

    this->menuBar()->addAction(tr("File"));
    auto mainToolBar = this->addToolBar(QStringLiteral("MainToolBar"));

    auto timeWidget = new TimeWidget{};
    mainToolBar->addWidget(timeWidget);

    // Controllers / controllers connections
    connect(&sqpApp->timeController(), SIGNAL(timeUpdated(SqpDateTime)),
            &sqpApp->variableController(), SLOT(onDateTimeOnSelection(SqpDateTime)));

    // Widgets / controllers connections

    // DataSource
    connect(&sqpApp->dataSourceController(), SIGNAL(dataSourceItemSet(DataSourceItem *)),
            m_Ui->dataSourceWidget, SLOT(addDataSource(DataSourceItem *)));

    // Time
    connect(timeWidget, SIGNAL(timeUpdated(SqpDateTime)), &sqpApp->timeController(),
            SLOT(onTimeToUpdate(SqpDateTime)));

    // Visualization
    connect(&sqpApp->visualizationController(),
            SIGNAL(variableAboutToBeDeleted(std::shared_ptr<Variable>)), m_Ui->view,
            SLOT(onVariableAboutToBeDeleted(std::shared_ptr<Variable>)));

    connect(&sqpApp->visualizationController(),
            SIGNAL(rangeChanged(std::shared_ptr<Variable>, const SqpDateTime &)), m_Ui->view,
            SLOT(onRangeChanged(std::shared_ptr<Variable>, const SqpDateTime &)));

    // Widgets / widgets connections

    // For the following connections, we use DirectConnection to allow each widget that can
    // potentially attach a menu to the variable's menu to do so before this menu is displayed.
    // The order of connections is also important, since it determines the order in which each
    // widget will attach its menu
    connect(
        m_Ui->variableInspectorWidget,
        SIGNAL(tableMenuAboutToBeDisplayed(QMenu *, const QVector<std::shared_ptr<Variable> > &)),
        m_Ui->view, SLOT(attachVariableMenu(QMenu *, const QVector<std::shared_ptr<Variable> > &)),
        Qt::DirectConnection);
}

MainWindow::~MainWindow()
{
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
        case QEvent::LanguageChange:
            m_Ui->retranslateUi(this);
            break;
        default:
            break;
    }
}
