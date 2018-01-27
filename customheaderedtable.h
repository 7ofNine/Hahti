#ifndef CUSTOMHEADEREDTABLE_H
#define CUSTOMHEADEREDTABLE_H

#include <QtCore/qglobal.h>
#include <QTableWidget>

#include "customtableheader.h"

class CustomHeaderedTable : public QTableWidget
{
    public:
        CustomHeaderedTable(QWidget * parent = 0);

        void scrollContentsBy(int dx, int dy);

        void SetHorizontalHeaderItemWidget(int column, QWidget * widget);
        void SetHorizontalHeaderItemMargins(int column, CustomTableHeader::Margins margins);

    private:
        CustomTableHeader * mHorizontalHeader;
};
#endif // CUSTOMHEADEREDTABLE_H
