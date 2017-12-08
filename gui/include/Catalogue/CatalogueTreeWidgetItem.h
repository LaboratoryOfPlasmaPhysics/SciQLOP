#ifndef SCIQLOP_CATALOGUETREEWIDGETITEM_H
#define SCIQLOP_CATALOGUETREEWIDGETITEM_H

#include <Common/spimpl.h>
#include <QTreeWidgetItem>

#include <DBCatalogue.h>


class CatalogueTreeWidgetItem : public QTreeWidgetItem {
public:
    CatalogueTreeWidgetItem(DBCatalogue catalogue, int type = QTreeWidgetItem::Type);

    QVariant data(int column, int role) const override;
    DBCatalogue catalogue() const;

private:
    class CatalogueTreeWidgetItemPrivate;
    spimpl::unique_impl_ptr<CatalogueTreeWidgetItemPrivate> impl;
};

#endif // SCIQLOP_CATALOGUETREEWIDGETITEM_H
