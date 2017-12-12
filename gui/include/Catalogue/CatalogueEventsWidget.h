#ifndef SCIQLOP_CATALOGUEEVENTSWIDGET_H
#define SCIQLOP_CATALOGUEEVENTSWIDGET_H

#include <Common/spimpl.h>
#include <QWidget>

class DBCatalogue;
class DBEvent;
class VisualizationWidget;

namespace Ui {
class CatalogueEventsWidget;
}

class CatalogueEventsWidget : public QWidget {
    Q_OBJECT

signals:
    void eventsSelected(const QVector<DBEvent> &event);

public:
    explicit CatalogueEventsWidget(QWidget *parent = 0);
    virtual ~CatalogueEventsWidget();

    void setVisualizationWidget(VisualizationWidget *visualization);

public slots:
    void populateWithCatalogues(const QVector<DBCatalogue> &catalogues);

private:
    Ui::CatalogueEventsWidget *ui;

    class CatalogueEventsWidgetPrivate;
    spimpl::unique_impl_ptr<CatalogueEventsWidgetPrivate> impl;
};

#endif // SCIQLOP_CATALOGUEEVENTSWIDGET_H
