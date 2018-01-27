#include "customheaderedtable.h"

CustomHeaderedTable::CustomHeaderedTable(QWidget * parent) :
    QTableWidget(parent)
{
    mHorizontalHeader = new CustomTableHeader(Qt::Horizontal, this);
    setHorizontalHeader(mHorizontalHeader);
}


void CustomHeaderedTable::scrollContentsBy(int dx, int dy)
{
    QTableWidget::scrollContentsBy(dx, dy);

    if (dx != 0)
        mHorizontalHeader->FixComboPositions();
}


void CustomHeaderedTable::SetHorizontalHeaderItemWidget(int column, QWidget * widget)
{
    mHorizontalHeader->SetItemWidget(column, widget);
}


void CustomHeaderedTable::SetHorizontalHeaderItemMargins(int column, CustomTableHeader::Margins margins)
{
    mHorizontalHeader->SetItemMargins(column, margins);
}
