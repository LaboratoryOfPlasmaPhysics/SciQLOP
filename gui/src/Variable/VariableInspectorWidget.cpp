#include <DataSource/datasources.h>
#include <Variable/RenameVariableDialog.h>
#include <Variable/VariableController2.h>
#include <Variable/VariableInspectorWidget.h>
#include <Variable/VariableMenuHeaderWidget.h>
#include <Variable/VariableModel2.h>

#include <ui_VariableInspectorWidget.h>

#include <QMouseEvent>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QWidgetAction>

#include <DragAndDrop/DragDropGuiController.h>
#include <SqpApplication.h>

Q_LOGGING_CATEGORY(LOG_VariableInspectorWidget, "VariableInspectorWidget")


class QProgressBarItemDelegate : public QStyledItemDelegate
{

public:
    QProgressBarItemDelegate(QObject* parent) : QStyledItemDelegate { parent } {}

    void paint(
        QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        auto data = index.data(Qt::DisplayRole);
        auto progressData = index.data(VariableRoles::ProgressRole);
        if (data.isValid() && progressData.isValid())
        {
            auto name = data.value<QString>();
            auto progress = progressData.value<double>();
            if (progress > 0)
            {
                auto cancelButtonWidth = 20;
                auto progressBarOption = QStyleOptionProgressBar {};
                auto progressRect = option.rect;
                progressRect.setWidth(progressRect.width() - cancelButtonWidth);
                progressBarOption.rect = progressRect;
                progressBarOption.minimum = 0;
                progressBarOption.maximum = 100;
                progressBarOption.progress = progress;
                progressBarOption.text
                    = QString("%1 %2").arg(name).arg(QString::number(progress, 'f', 2) + "%");
                progressBarOption.textVisible = true;
                progressBarOption.textAlignment = Qt::AlignCenter;


                QApplication::style()->drawControl(
                    QStyle::CE_ProgressBar, &progressBarOption, painter);

                // Cancel button
                auto buttonRect = QRect(progressRect.right(), option.rect.top(), cancelButtonWidth,
                    option.rect.height());
                auto buttonOption = QStyleOptionButton {};
                buttonOption.rect = buttonRect;
                buttonOption.text = "X";

                QApplication::style()->drawControl(QStyle::CE_PushButton, &buttonOption, painter);
            }
            else
            {
                QStyledItemDelegate::paint(painter, option, index);
            }
        }
        else
        {
            QStyledItemDelegate::paint(painter, option, index);
        }
    }

    bool editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option,
        const QModelIndex& index)
    {
        if (event->type() == QEvent::MouseButtonRelease)
        {
            auto data = index.data(Qt::DisplayRole);
            auto progressData = index.data(VariableRoles::ProgressRole);
            if (data.isValid() && progressData.isValid())
            {
                auto cancelButtonWidth = 20;
                auto progressRect = option.rect;
                progressRect.setWidth(progressRect.width() - cancelButtonWidth);
                // Cancel button
                auto buttonRect = QRect(progressRect.right(), option.rect.top(), cancelButtonWidth,
                    option.rect.height());

                auto e = (QMouseEvent*)event;
                auto clickX = e->x();
                auto clickY = e->y();

                auto x = buttonRect.left(); // the X coordinate
                auto y = buttonRect.top(); // the Y coordinate
                auto w = buttonRect.width(); // button width
                auto h = buttonRect.height(); // button height

                if (clickX > x && clickX < x + w)
                {
                    if (clickY > y && clickY < y + h)
                    {
                        // auto& variableModel = sqpApp->variableModel();
                        // variableModel->abortProgress(index);
                    }
                    return true;
                }
                else
                {
                    return QStyledItemDelegate::editorEvent(event, model, option, index);
                }
            }
            else
            {
                return QStyledItemDelegate::editorEvent(event, model, option, index);
            }
        }
        else
        {
            return QStyledItemDelegate::editorEvent(event, model, option, index);
        }


        return QStyledItemDelegate::editorEvent(event, model, option, index);
    }
};

VariableInspectorWidget::VariableInspectorWidget(QWidget* parent)
        : QWidget { parent }
        , ui { new Ui::VariableInspectorWidget }
        , m_ProgressBarItemDelegate { new QProgressBarItemDelegate { this } }
{
    ui->setupUi(this);

    // Sets model for table
    //    auto sortFilterModel = new QSortFilterProxyModel{this};
    //    sortFilterModel->setSourceModel(sqpApp->variableController().variableModel());

    m_model = new VariableModel2();
    ui->tableView->setModel(m_model);
    connect(m_model, &VariableModel2::createVariable, [](const QString& productPath) {
        sqpApp->dataSources().createVariable(productPath);
    });
    auto vc = &(sqpApp->variableController());
    connect(vc, &VariableController2::variableAdded, m_model, &VariableModel2::variableAdded);
    connect(vc, &VariableController2::variableDeleted, m_model, &VariableModel2::variableDeleted);
    connect(m_model, &VariableModel2::asyncChangeRange, vc, &VariableController2::asyncChangeRange);

    // Adds extra signal/slot between view and model, so the view can be updated instantly when
    // there is a change of data in the model
    // connect(m_model, SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)), this,
    //        SLOT(refresh()));

    // ui->tableView->setSelectionModel(sqpApp->variableController().variableSelectionModel());
    ui->tableView->setItemDelegateForColumn(0, m_ProgressBarItemDelegate);

    // Fixes column sizes
    auto model = ui->tableView->model();
    const auto count = model->columnCount();
    for (auto i = 0; i < count; ++i)
    {
        ui->tableView->setColumnWidth(
            i, model->headerData(i, Qt::Horizontal, Qt::SizeHintRole).toSize().width());
    }

    // Sets selection options
    ui->tableView->setSelectionBehavior(QTableView::SelectRows);
    ui->tableView->setSelectionMode(QTableView::ExtendedSelection);

    // Connection to show a menu when right clicking on the tree
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView, &QTableView::customContextMenuRequested, this,
        &VariableInspectorWidget::onTableMenuRequested);
}

VariableInspectorWidget::~VariableInspectorWidget()
{
    delete ui;
}

void VariableInspectorWidget::onTableMenuRequested(const QPoint& pos) noexcept
{
    auto selectedRows = ui->tableView->selectionModel()->selectedRows();
    auto selectedVariables = QVector<std::shared_ptr<Variable2>> {};
    for (const auto& selectedRow : qAsConst(selectedRows))
    {
        if (auto selectedVariable = this->m_model->variables()[selectedRow.row()])
        {
            selectedVariables.push_back(selectedVariable);
        }
    }

    QMenu tableMenu {};

    // Emits a signal so that potential receivers can populate the menu before displaying it
    emit tableMenuAboutToBeDisplayed(&tableMenu, selectedVariables);

    // Adds menu-specific actions
    if (!selectedVariables.isEmpty())
    {
        tableMenu.addSeparator();

        // 'Rename' and 'Duplicate' actions (only if one variable selected)
        if (selectedVariables.size() == 1)
        {
            auto selectedVariable = selectedVariables.front();

            auto duplicateFun = [varW = std::weak_ptr<Variable2>(selectedVariable)]() {
                if (auto var = varW.lock())
                {
                    sqpApp->variableController().cloneVariable(var);
                }
            };

            tableMenu.addAction(tr("Duplicate"), duplicateFun);

            auto renameFun = [varW = std::weak_ptr<Variable2>(selectedVariable), this]() {
                if (auto var = varW.lock())
                {
                    // Generates forbidden names (names associated to existing variables)
                    auto allVariables = sqpApp->variableController().variables();
                    auto forbiddenNames = QVector<QString>(allVariables.size());
                    std::transform(allVariables.cbegin(), allVariables.cend(),
                        forbiddenNames.begin(),
                        [](const auto& variable) { return variable->name(); });

                    RenameVariableDialog dialog { var->name(), forbiddenNames, this };
                    if (dialog.exec() == QDialog::Accepted)
                    {
                        var->setName(dialog.name());
                    }
                }
            };

            tableMenu.addAction(tr("Rename..."), renameFun);
        }

        // 'Delete' action
        auto deleteFun = [&selectedVariables]() {
            for (const auto& var : selectedVariables)
                sqpApp->variableController().deleteVariable(var);
        };

        tableMenu.addAction(QIcon { ":/icones/delete.png" }, tr("Delete"), deleteFun);
    }

    if (!tableMenu.isEmpty())
    {
        // Generates menu header (inserted before first action)
        auto firstAction = tableMenu.actions().first();
        auto headerAction = new QWidgetAction { &tableMenu };
        headerAction->setDefaultWidget(
            new VariableMenuHeaderWidget { selectedVariables, &tableMenu });
        tableMenu.insertAction(firstAction, headerAction);

        // Displays menu
        tableMenu.exec(QCursor::pos());
    }
}

void VariableInspectorWidget::refresh() noexcept
{
    ui->tableView->viewport()->update();
}
