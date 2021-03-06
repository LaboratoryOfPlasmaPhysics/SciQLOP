#include "Visualization/VisualizationSelectionZoneItem.h"
#include "Visualization/VisualizationGraphWidget.h"
#include "Visualization/VisualizationSelectionZoneManager.h"
#include "Visualization/VisualizationWidget.h"

const QString &DEFAULT_COLOR = QStringLiteral("#E79D41");

struct VisualizationSelectionZoneItem::VisualizationSelectionZoneItemPrivate {

    QCustomPlot *m_Plot;
    double m_T1 = 0;
    double m_T2 = 0;
    QColor m_Color;

    bool m_IsEditionEnabled = true;
    double m_MovedOrinalT1 = 0;
    double m_MovedOrinalT2 = 0;

    QCPItemStraightLine *m_LeftLine;
    QCPItemStraightLine *m_RightLine;
    QCPItemText *m_NameLabelItem = nullptr;

    enum class EditionMode { NoEdition, ResizeLeft, ResizeRight, Move };
    EditionMode m_CurrentEditionMode;

    QVector<VisualizationSelectionZoneItem *> m_AssociatedEditedZones;

    VisualizationSelectionZoneItemPrivate(QCustomPlot *plot)
            : m_Plot(plot), m_Color(Qt::blue), m_CurrentEditionMode(EditionMode::NoEdition)
    {
    }

    void updatePosition(VisualizationSelectionZoneItem *item)
    {
        item->topLeft->setCoords(m_T1, 0);
        item->bottomRight->setCoords(m_T2, 1);
    }

    EditionMode getEditionMode(const QPoint &pos, const VisualizationSelectionZoneItem *zoneItem)
    {
        auto distanceLeft = m_LeftLine->selectTest(pos, false);
        auto distanceRight = m_RightLine->selectTest(pos, false);
        auto distance = zoneItem->selectTest(pos, false);

        if (distanceRight <= distance) {
            return VisualizationSelectionZoneItemPrivate::EditionMode::ResizeRight;
        }
        else if (distanceLeft <= distance) {
            return VisualizationSelectionZoneItemPrivate::EditionMode::ResizeLeft;
        }

        return VisualizationSelectionZoneItemPrivate::EditionMode::Move;
    }

    double pixelSizeToAxisXSize(double pixels)
    {
        auto axis = m_Plot->axisRect()->axis(QCPAxis::atBottom);
        return axis->pixelToCoord(pixels) - axis->pixelToCoord(0);
    }

    bool alignZones(VisualizationSelectionZoneItem *referenceZone,
                    const QVector<VisualizationSelectionZoneItem *> &zonesToAlign, bool alignOnLeft,
                    bool allowResize, bool vertically)
    {
        auto result = false;

        auto referenceTime
            = alignOnLeft ? referenceZone->range().m_TStart : referenceZone->range().m_TEnd;

        auto referenceBottomAxis = m_Plot->axisRect()->axis(QCPAxis::atBottom);
        auto referenceVerticalPosition = referenceBottomAxis->coordToPixel(referenceTime);

        for (auto otherZone : zonesToAlign) {

            auto otherZoneRange = otherZone->range();
            auto newZoneStart = otherZoneRange.m_TStart;
            auto newZoneEnd = otherZoneRange.m_TEnd;

            auto alignedTime = referenceTime;
            if (vertically) {
                auto otherZoneAxis = otherZone->parentPlot()->axisRect()->axis(QCPAxis::atBottom);
                alignedTime = otherZoneAxis->pixelToCoord(referenceVerticalPosition);
            }

            if (alignOnLeft) {
                newZoneStart = alignedTime;
                if (!allowResize) {
                    newZoneEnd = alignedTime + (otherZoneRange.m_TEnd - otherZoneRange.m_TStart);
                }
            }
            else { // align on right
                newZoneEnd = alignedTime;
                if (!allowResize) {
                    newZoneStart = alignedTime - (otherZoneRange.m_TEnd - otherZoneRange.m_TStart);
                }
            }

            if (newZoneStart < newZoneEnd) {
                result = true;
                otherZone->setRange(newZoneStart, newZoneEnd);
                otherZone->parentPlot()->replot();
            }
        }

        return result;
    }
};

VisualizationSelectionZoneItem::VisualizationSelectionZoneItem(QCustomPlot *plot)
        : QCPItemRect(plot),
          impl{spimpl::make_unique_impl<VisualizationSelectionZoneItemPrivate>(plot)}
{
    topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
    topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
    bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
    bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
    setSelectable(false);

    impl->m_RightLine = new QCPItemStraightLine(plot);
    impl->m_RightLine->point1->setParentAnchor(topRight);
    impl->m_RightLine->point2->setParentAnchor(bottomRight);
    impl->m_RightLine->point1->setTypeX(QCPItemPosition::ptAbsolute);
    impl->m_RightLine->point1->setTypeY(QCPItemPosition::ptAbsolute);
    impl->m_RightLine->point2->setTypeX(QCPItemPosition::ptAbsolute);
    impl->m_RightLine->point2->setTypeY(QCPItemPosition::ptAbsolute);
    impl->m_RightLine->setSelectable(false);

    impl->m_LeftLine = new QCPItemStraightLine(plot);
    impl->m_LeftLine->point1->setParentAnchor(topLeft);
    impl->m_LeftLine->point2->setParentAnchor(bottomLeft);
    impl->m_LeftLine->point1->setTypeX(QCPItemPosition::ptAbsolute);
    impl->m_LeftLine->point1->setTypeY(QCPItemPosition::ptAbsolute);
    impl->m_LeftLine->point2->setTypeX(QCPItemPosition::ptAbsolute);
    impl->m_LeftLine->point2->setTypeY(QCPItemPosition::ptAbsolute);
    impl->m_LeftLine->setSelectable(false);

    connect(this, &VisualizationSelectionZoneItem::selectionChanged, impl->m_RightLine,
            &QCPItemStraightLine::setSelected);
    connect(this, &VisualizationSelectionZoneItem::selectionChanged, impl->m_LeftLine,
            &QCPItemStraightLine::setSelected);

    setColor(QColor(DEFAULT_COLOR));
}

VisualizationSelectionZoneItem::~VisualizationSelectionZoneItem()
{
}

VisualizationGraphWidget *VisualizationSelectionZoneItem::parentGraphWidget() const noexcept
{
    auto parent = impl->m_Plot->parentWidget();
    while (parent != nullptr && !qobject_cast<VisualizationGraphWidget *>(parent)) {
        parent = parent->parentWidget();
    }

    return qobject_cast<VisualizationGraphWidget *>(parent);
}

void VisualizationSelectionZoneItem::setName(const QString &name)
{
    if (name.isEmpty() && impl->m_NameLabelItem) {
        impl->m_Plot->removeItem(impl->m_NameLabelItem);
        impl->m_NameLabelItem = nullptr;
    }
    else if (!impl->m_NameLabelItem) {
        impl->m_NameLabelItem = new QCPItemText(impl->m_Plot);
        impl->m_NameLabelItem->setText(name);
        impl->m_NameLabelItem->setPositionAlignment(Qt::AlignHCenter | Qt::AlignTop);
        impl->m_NameLabelItem->setColor(impl->m_Color);
        impl->m_NameLabelItem->position->setParentAnchor(top);
    }
}

QString VisualizationSelectionZoneItem::name() const
{
    if (!impl->m_NameLabelItem) {
        return QString();
    }

    return impl->m_NameLabelItem->text();
}

DateTimeRange VisualizationSelectionZoneItem::range() const
{
    DateTimeRange range;
    range.m_TStart = impl->m_T1 <= impl->m_T2 ? impl->m_T1 : impl->m_T2;
    range.m_TEnd = impl->m_T1 > impl->m_T2 ? impl->m_T1 : impl->m_T2;
    return range;
}

void VisualizationSelectionZoneItem::setRange(double tstart, double tend)
{
    impl->m_T1 = tstart;
    impl->m_T2 = tend;
    impl->updatePosition(this);
}

void VisualizationSelectionZoneItem::setStart(double tstart)
{
    impl->m_T1 = tstart;
    impl->updatePosition(this);
}

void VisualizationSelectionZoneItem::setEnd(double tend)
{
    impl->m_T2 = tend;
    impl->updatePosition(this);
}

void VisualizationSelectionZoneItem::setColor(const QColor &color)
{
    impl->m_Color = color;

    auto brushColor = color;
    brushColor.setAlpha(80);
    setBrush(QBrush(brushColor));
    setPen(QPen(Qt::NoPen));

    auto selectedBrushColor = brushColor;
    selectedBrushColor.setAlpha(150);
    setSelectedBrush(QBrush(selectedBrushColor));
    setSelectedPen(QPen(Qt::NoPen));

    auto linePen = QPen(color);
    linePen.setStyle(Qt::SolidLine);
    linePen.setWidth(4);

    auto selectedLinePen = linePen;
    selectedLinePen.setColor(color.darker(120));
    selectedLinePen.setWidth(4);

    impl->m_LeftLine->setPen(linePen);
    impl->m_RightLine->setPen(linePen);

    impl->m_LeftLine->setSelectedPen(selectedLinePen);
    impl->m_RightLine->setSelectedPen(selectedLinePen);
}

void VisualizationSelectionZoneItem::setEditionEnabled(bool value)
{
    impl->m_IsEditionEnabled = value;
    setSelectable(value);
    if (!value) {
        setSelected(false);
        impl->m_CurrentEditionMode = VisualizationSelectionZoneItemPrivate::EditionMode::NoEdition;
    }
}

bool VisualizationSelectionZoneItem::isEditionEnabled() const
{
    return impl->m_IsEditionEnabled;
}

void VisualizationSelectionZoneItem::moveToTop()
{
    moveToLayer(layer(), false);
}

Qt::CursorShape
VisualizationSelectionZoneItem::curshorShapeForPosition(const QPoint &position) const
{
    auto mode = impl->m_CurrentEditionMode
                        == VisualizationSelectionZoneItemPrivate::EditionMode::NoEdition
                    ? impl->getEditionMode(position, this)
                    : impl->m_CurrentEditionMode;
    switch (mode) {
        case VisualizationSelectionZoneItemPrivate::EditionMode::Move:
            return Qt::SizeAllCursor;
        case VisualizationSelectionZoneItemPrivate::EditionMode::ResizeLeft:
        case VisualizationSelectionZoneItemPrivate::EditionMode::ResizeRight: // fallthrough
            return Qt::SizeHorCursor;
        default:
            return Qt::ArrowCursor;
    }
}

void VisualizationSelectionZoneItem::setHovered(bool value)
{
    if (value) {
        auto linePen = impl->m_LeftLine->pen();
        linePen.setStyle(Qt::DotLine);
        linePen.setWidth(3);

        auto selectedLinePen = impl->m_LeftLine->selectedPen();
        ;
        selectedLinePen.setStyle(Qt::DotLine);
        selectedLinePen.setWidth(3);

        impl->m_LeftLine->setPen(linePen);
        impl->m_RightLine->setPen(linePen);

        impl->m_LeftLine->setSelectedPen(selectedLinePen);
        impl->m_RightLine->setSelectedPen(selectedLinePen);
    }
    else {
        setColor(impl->m_Color);
    }
}

void VisualizationSelectionZoneItem::setAssociatedEditedZones(
    const QVector<VisualizationSelectionZoneItem *> &associatedZones)
{
    impl->m_AssociatedEditedZones = associatedZones;
    impl->m_AssociatedEditedZones.removeAll(this);
}

bool VisualizationSelectionZoneItem::alignZonesVerticallyOnLeft(
    const QVector<VisualizationSelectionZoneItem *> &zonesToAlign, bool allowResize)
{
    return impl->alignZones(this, zonesToAlign, true, allowResize, true);
}

bool VisualizationSelectionZoneItem::alignZonesVerticallyOnRight(
    const QVector<VisualizationSelectionZoneItem *> &zonesToAlign, bool allowResize)
{
    return impl->alignZones(this, zonesToAlign, false, allowResize, true);
}

bool VisualizationSelectionZoneItem::alignZonesTemporallyOnLeft(
    const QVector<VisualizationSelectionZoneItem *> &zonesToAlign, bool allowResize)
{
    return impl->alignZones(this, zonesToAlign, true, allowResize, false);
}

bool VisualizationSelectionZoneItem::alignZonesTemporallyOnRight(
    const QVector<VisualizationSelectionZoneItem *> &zonesToAlign, bool allowResize)
{
    return impl->alignZones(this, zonesToAlign, false, allowResize, false);
}

void VisualizationSelectionZoneItem::mousePressEvent(QMouseEvent *event, const QVariant &details)
{
    Q_UNUSED(details);

    if (isEditionEnabled() && event->button() == Qt::LeftButton) {
        impl->m_CurrentEditionMode = impl->getEditionMode(event->pos(), this);

        impl->m_MovedOrinalT1 = impl->m_T1;
        impl->m_MovedOrinalT2 = impl->m_T2;
        for (auto associatedZone : impl->m_AssociatedEditedZones) {
            associatedZone->impl->m_MovedOrinalT1 = associatedZone->impl->m_T1;
            associatedZone->impl->m_MovedOrinalT2 = associatedZone->impl->m_T2;
        }
    }
    else {
        impl->m_CurrentEditionMode = VisualizationSelectionZoneItemPrivate::EditionMode::NoEdition;
        event->ignore();
    }
}

void VisualizationSelectionZoneItem::mouseMoveEvent(QMouseEvent *event, const QPointF &startPos)
{
    if (isEditionEnabled()) {
        if (!selected()) {
            // Force the item to be selected during the edition
            parentGraphWidget()->parentVisualizationWidget()->selectionZoneManager().setSelected(
                this, true);
        }

        auto axis = impl->m_Plot->axisRect()->axis(QCPAxis::atBottom);
        auto pixelDiff = event->pos().x() - startPos.x();
        auto diff = impl->pixelSizeToAxisXSize(pixelDiff);

        switch (impl->m_CurrentEditionMode) {
            case VisualizationSelectionZoneItemPrivate::EditionMode::Move:
                setRange(impl->m_MovedOrinalT1 + diff, impl->m_MovedOrinalT2 + diff);
                for (auto associatedZone : impl->m_AssociatedEditedZones) {
                    associatedZone->move(pixelDiff);
                }
                break;
            case VisualizationSelectionZoneItemPrivate::EditionMode::ResizeLeft:
                setStart(impl->m_MovedOrinalT1 + diff);
                for (auto associatedZone : impl->m_AssociatedEditedZones) {
                    impl->m_MovedOrinalT1 < impl->m_MovedOrinalT2
                        ? associatedZone->resizeLeft(pixelDiff)
                        : associatedZone->resizeRight(pixelDiff);
                }
                break;
            case VisualizationSelectionZoneItemPrivate::EditionMode::ResizeRight:
                setEnd(impl->m_MovedOrinalT2 + diff);
                for (auto associatedZone : impl->m_AssociatedEditedZones) {
                    impl->m_MovedOrinalT1 < impl->m_MovedOrinalT2
                        ? associatedZone->resizeRight(pixelDiff)
                        : associatedZone->resizeLeft(pixelDiff);
                }
                break;
            default:
                break;
        }

        emit rangeEdited(range());

        for (auto associatedZone : impl->m_AssociatedEditedZones) {
            associatedZone->parentPlot()->replot();
            emit associatedZone->rangeEdited(associatedZone->range());
        }
    }
    else {
        event->ignore();
    }
}

void VisualizationSelectionZoneItem::mouseReleaseEvent(QMouseEvent *event, const QPointF &startPos)
{
    Q_UNUSED(startPos);

    if (isEditionEnabled()) {
        impl->m_CurrentEditionMode = VisualizationSelectionZoneItemPrivate::EditionMode::NoEdition;
    }
    else {
        event->ignore();
    }

    impl->m_AssociatedEditedZones.clear();
}

void VisualizationSelectionZoneItem::resizeLeft(double pixelDiff)
{
    auto diff = impl->pixelSizeToAxisXSize(pixelDiff);
    if (impl->m_MovedOrinalT1 <= impl->m_MovedOrinalT2) {
        setStart(impl->m_MovedOrinalT1 + diff);
    }
    else {
        setEnd(impl->m_MovedOrinalT2 + diff);
    }
}

void VisualizationSelectionZoneItem::resizeRight(double pixelDiff)
{
    auto diff = impl->pixelSizeToAxisXSize(pixelDiff);
    if (impl->m_MovedOrinalT1 > impl->m_MovedOrinalT2) {
        setStart(impl->m_MovedOrinalT1 + diff);
    }
    else {
        setEnd(impl->m_MovedOrinalT2 + diff);
    }
}

void VisualizationSelectionZoneItem::move(double pixelDiff)
{
    auto diff = impl->pixelSizeToAxisXSize(pixelDiff);
    setRange(impl->m_MovedOrinalT1 + diff, impl->m_MovedOrinalT2 + diff);
}
