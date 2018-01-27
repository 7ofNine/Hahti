#include "customtableheader.h"

CustomTableHeader::CustomTableHeader(Qt::Orientation orientation, QWidget * parent) :
    QHeaderView(orientation, parent)
{
    connect(this, SIGNAL(CustomTableHeader::sectionResized(int)), this, SLOT(CustomTableHeader::HandleSectionResized(int)));
    connect(this, SIGNAL(CustomTableHeader::sectionMoved),   this, SLOT(CustomTableHeader::HandleSectionMoved));
}


void CustomTableHeader::showEvent(QShowEvent *e)
{
    for (int i = 0; i < count(); i++)
    {
        if (!mItems[i].item)
            mItems[i].item = new QWidget(this);
        else
            mItems[i].item->setParent(this);

        mItems[i].item->setGeometry(sectionViewportPosition(i) + mItems[i].margins.left,
                                    mItems[i].margins.top,
                                    sectionSize(i) - mItems[i].margins.left - mItems[i].margins.right - 1,
                                    height() - mItems[i].margins.top - mItems[i].margins.buttom - 1);
        mItems[i].item->show();
    }

    QHeaderView::showEvent(e);
}


void CustomTableHeader::HandleSectionResized(int i)
{
    int logical;

    for (int j = visualIndex(i); j < count(); j++)
    {
        logical = logicalIndex(j);
        mItems[logical].item->setGeometry(sectionViewportPosition(logical) + mItems[i].margins.left,
                                          mItems[i].margins.top,
                                          sectionSize(logical) - mItems[i].margins.left - mItems[i].margins.right - 1,
                                          height() - mItems[i].margins.top - mItems[i].margins.buttom - 1);
    }
}


void CustomTableHeader::HandleSectionMoved(int logical, int oldVisualIndex, int newVisualIndex)
{
    Q_UNUSED(logical);

    for (int i = qMin(oldVisualIndex, newVisualIndex); i < count(); i++)
    {
        int logical = logicalIndex(i);
        mItems[logical].item->setGeometry(sectionViewportPosition(logical) + mItems[i].margins.left,
                                          mItems[i].margins.top,
                                          sectionSize(logical) - mItems[i].margins.left - mItems[i].margins.right - 1,
                                          height() - mItems[i].margins.top - mItems[i].margins.buttom - 1);
    }
}


void CustomTableHeader::FixComboPositions()
{
    for (int i = 0; i < count(); i++)
        mItems[i].item->setGeometry(sectionViewportPosition(i) + mItems[i].margins.left,
                                    mItems[i].margins.top,
                                    sectionSize(i) - mItems[i].margins.left - mItems[i].margins.right - 1,
                                    height() - mItems[i].margins.top - mItems[i].margins.buttom - 1);
}


void CustomTableHeader::SetItemWidget(int index, QWidget * widget)
{
    widget->setParent(this);
    mItems[index].item = widget;
}


void CustomTableHeader::SetItemMargins(int index, CustomTableHeader::Margins margins)
{
    mItems[index].margins = margins;
}


CustomTableHeader::Margins::Margins(int left, int right, int top, int buttom) :
    left(left),
    right(right),
    top(top),
    buttom(buttom)

{
}


CustomTableHeader::Item::Item() :
    item(0)
{
}
