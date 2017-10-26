#include "Visualization/VisualizationDragDropContainer.h"
#include "DragDropHelper.h"
#include "SqpApplication.h"
#include "Visualization/VisualizationDragWidget.h"

#include <QDrag>
#include <QDragEnterEvent>
#include <QVBoxLayout>

#include <cmath>
#include <memory>

struct VisualizationDragDropContainer::VisualizationDragDropContainerPrivate {

    QVBoxLayout *m_Layout;
    QStringList m_AcceptedMimeTypes;
    QStringList m_MergeAllowedMimeTypes;

    explicit VisualizationDragDropContainerPrivate(QWidget *widget)
    {
        m_Layout = new QVBoxLayout(widget);
        m_Layout->setContentsMargins(0, 0, 0, 0);
    }

    bool acceptMimeData(const QMimeData *data) const
    {
        for (const auto &type : m_AcceptedMimeTypes) {
            if (data->hasFormat(type)) {
                return true;
            }
        }

        return false;
    }

    bool allowMergeMimeData(const QMimeData *data) const
    {
        for (const auto &type : m_MergeAllowedMimeTypes) {
            if (data->hasFormat(type)) {
                return true;
            }
        }

        return false;
    }

    bool hasPlaceHolder() const
    {
        return sqpApp->dragDropHelper().placeHolder().parentWidget() == m_Layout->parentWidget();
    }

    VisualizationDragWidget *getChildDragWidgetAt(QWidget *parent, const QPoint &pos) const
    {
        VisualizationDragWidget *dragWidget = nullptr;

        for (auto child : parent->children()) {
            auto widget = qobject_cast<VisualizationDragWidget *>(child);
            if (widget && widget->isVisible()) {
                if (widget->frameGeometry().contains(pos)) {
                    dragWidget = widget;
                    break;
                }
            }
        }

        return dragWidget;
    }

    bool cursorIsInContainer(QWidget *container) const
    {
        auto adustNum = 18; // to be safe, in case of scrollbar on the side
        auto containerRect = QRect(QPoint(), container->contentsRect().size())
                                 .adjusted(adustNum, adustNum, -adustNum, -adustNum);
        return containerRect.contains(container->mapFromGlobal(QCursor::pos()));
    }
};

VisualizationDragDropContainer::VisualizationDragDropContainer(QWidget *parent)
        : QWidget{parent},
          impl{spimpl::make_unique_impl<VisualizationDragDropContainerPrivate>(this)}
{
    setAcceptDrops(true);
}

void VisualizationDragDropContainer::addDragWidget(VisualizationDragWidget *dragWidget)
{
    impl->m_Layout->addWidget(dragWidget);
    disconnect(dragWidget, &VisualizationDragWidget::dragDetected, nullptr, nullptr);
    connect(dragWidget, &VisualizationDragWidget::dragDetected, this,
            &VisualizationDragDropContainer::startDrag);
}

void VisualizationDragDropContainer::insertDragWidget(int index,
                                                      VisualizationDragWidget *dragWidget)
{
    impl->m_Layout->insertWidget(index, dragWidget);
    disconnect(dragWidget, &VisualizationDragWidget::dragDetected, nullptr, nullptr);
    connect(dragWidget, &VisualizationDragWidget::dragDetected, this,
            &VisualizationDragDropContainer::startDrag);
}

void VisualizationDragDropContainer::setAcceptedMimeTypes(const QStringList &mimeTypes)
{
    impl->m_AcceptedMimeTypes = mimeTypes;
}

void VisualizationDragDropContainer::setMergeAllowedMimeTypes(const QStringList &mimeTypes)
{
    impl->m_MergeAllowedMimeTypes = mimeTypes;
}

int VisualizationDragDropContainer::countDragWidget() const
{
    auto nbGraph = 0;
    for (auto child : children()) {
        if (qobject_cast<VisualizationDragWidget *>(child)) {
            nbGraph += 1;
        }
    }

    return nbGraph;
}

void VisualizationDragDropContainer::startDrag(VisualizationDragWidget *dragWidget,
                                               const QPoint &dragPosition)
{
    auto &helper = sqpApp->dragDropHelper();

    // Note: The management of the drag object is done by Qt
    auto drag = new QDrag{dragWidget};
    drag->setHotSpot(dragPosition);

    auto mimeData = dragWidget->mimeData();
    drag->setMimeData(mimeData);

    auto pixmap = QPixmap(dragWidget->size());
    dragWidget->render(&pixmap);
    drag->setPixmap(pixmap);

    auto image = pixmap.toImage();
    mimeData->setImageData(image);
    mimeData->setUrls({helper.imageTemporaryUrl(image)});

    if (impl->m_Layout->indexOf(dragWidget) >= 0) {
        helper.setCurrentDragWidget(dragWidget);

        if (impl->cursorIsInContainer(this)) {
            auto dragWidgetIndex = impl->m_Layout->indexOf(dragWidget);
            helper.insertPlaceHolder(impl->m_Layout, dragWidgetIndex);
            dragWidget->setVisible(false);
        }
    }

    // Note: The exec() is blocking on windows but not on linux and macOS
    drag->exec(Qt::MoveAction | Qt::CopyAction);
}

void VisualizationDragDropContainer::dragEnterEvent(QDragEnterEvent *event)
{
    if (impl->acceptMimeData(event->mimeData())) {
        event->acceptProposedAction();

        auto &helper = sqpApp->dragDropHelper();

        if (!impl->hasPlaceHolder()) {
            auto dragWidget = helper.getCurrentDragWidget();
            auto parentWidget
                = qobject_cast<VisualizationDragDropContainer *>(dragWidget->parentWidget());
            if (parentWidget) {
                dragWidget->setVisible(false);
            }

            auto dragWidgetHovered = impl->getChildDragWidgetAt(this, event->pos());

            if (dragWidgetHovered) {
                auto hoveredWidgetIndex = impl->m_Layout->indexOf(dragWidgetHovered);
                auto dragWidgetIndex = impl->m_Layout->indexOf(helper.getCurrentDragWidget());
                if (dragWidgetIndex >= 0 && dragWidgetIndex <= hoveredWidgetIndex) {
                    hoveredWidgetIndex
                        += 1; // Correction of the index if the drop occurs in the same container
                }

                helper.insertPlaceHolder(impl->m_Layout, hoveredWidgetIndex);
            }
            else {
                helper.insertPlaceHolder(impl->m_Layout, 0);
            }
        }
    }
    else {
        event->ignore();
    }

    QWidget::dragEnterEvent(event);
}

void VisualizationDragDropContainer::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);

    auto &helper = sqpApp->dragDropHelper();

    if (!impl->cursorIsInContainer(this)) {
        helper.removePlaceHolder();

        bool isInternal = true;
        if (isInternal) {
            // Only if the drag is started from the visualization
            // Show the drag widget at its original place
            // So the drag widget doesn't stay hidden if the drop occurs outside the visualization
            // drop zone (It is not possible to catch a drop event outside of the application)

            auto dragWidget = sqpApp->dragDropHelper().getCurrentDragWidget();
            if (dragWidget) {
                dragWidget->setVisible(true);
            }
        }
    }

    QWidget::dragLeaveEvent(event);
}

void VisualizationDragDropContainer::dragMoveEvent(QDragMoveEvent *event)
{
    if (impl->acceptMimeData(event->mimeData())) {
        auto dragWidgetHovered = impl->getChildDragWidgetAt(this, event->pos());
        if (dragWidgetHovered) {
            auto canMerge = impl->allowMergeMimeData(event->mimeData());

            auto nbDragWidget = countDragWidget();
            if (nbDragWidget > 0) {
                auto graphHeight = size().height() / nbDragWidget;
                auto dropIndex = floor(event->pos().y() / graphHeight);
                auto zoneSize = qMin(graphHeight / 3.0, 150.0);

                auto isOnTop = event->pos().y() < dropIndex * graphHeight + zoneSize;
                auto isOnBottom = event->pos().y() > (dropIndex + 1) * graphHeight - zoneSize;

                auto &helper = sqpApp->dragDropHelper();
                auto placeHolderIndex = impl->m_Layout->indexOf(&(helper.placeHolder()));

                if (isOnTop || isOnBottom) {
                    if (isOnBottom) {
                        dropIndex += 1;
                    }

                    auto dragWidgetIndex = impl->m_Layout->indexOf(helper.getCurrentDragWidget());
                    if (dragWidgetIndex >= 0 && dragWidgetIndex <= dropIndex) {
                        dropIndex += 1; // Correction of the index if the drop occurs in the same
                                        // container
                    }

                    if (dropIndex != placeHolderIndex) {
                        helper.insertPlaceHolder(impl->m_Layout, dropIndex);
                    }
                }
                else if (canMerge) {
                    // drop on the middle -> merge
                    if (impl->hasPlaceHolder()) {
                        helper.removePlaceHolder();
                    }
                }
            }
        }
    }
    else {
        event->ignore();
    }

    QWidget::dragMoveEvent(event);
}

void VisualizationDragDropContainer::dropEvent(QDropEvent *event)
{
    if (impl->acceptMimeData(event->mimeData())) {
        auto dragWidget = sqpApp->dragDropHelper().getCurrentDragWidget();
        if (impl->hasPlaceHolder() && dragWidget) {
            auto &helper = sqpApp->dragDropHelper();

            auto droppedIndex = impl->m_Layout->indexOf(&helper.placeHolder());

            auto dragWidgetIndex = impl->m_Layout->indexOf(dragWidget);
            if (dragWidgetIndex >= 0 && dragWidgetIndex < droppedIndex) {
                droppedIndex
                    -= 1; // Correction of the index if the drop occurs in the same container
            }

            dragWidget->setVisible(true);

            event->acceptProposedAction();

            helper.removePlaceHolder();

            emit dropOccured(droppedIndex, event->mimeData());
        }
    }
    else {
        event->ignore();
    }

    QWidget::dropEvent(event);
}