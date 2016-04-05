#include "imageTile.h"
#include "colour.h"
namespace mpMapInteractive
{
	imageTile::~imageTile()
	{
		for(std::vector<QGraphicsPixmapItem*>::iterator i = pixMapItems.begin(); i != pixMapItems.end(); i++)
		{
			delete *i;
		}
		pixMapItems.clear();
	}
	imageTile::imageTile()
		: groupItem(NULL)
	{}
	imageTile::imageTile(imageTile&& other)
		:rowIndices(std::move(other.rowIndices)), columnIndices(std::move(other.columnIndices)), rowGroup(other.rowGroup), columnGroup(other.columnGroup), pixMapItems(std::move(other.pixMapItems)), groupItem(other.groupItem)
	{
		groupItem.swap(other.groupItem);
	}
	imageTile::imageTile(std::vector<uchar>& data, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene)
	:rowIndices(rowIndices), columnIndices(columnIndices), rowGroup(rowGroup), columnGroup(columnGroup)
	{
		QImage* image = new QImage((int)rowIndices.size(), (int)columnIndices.size(), QImage::Format_Indexed8);
		//get 100 colours
		QVector<QRgb> colours;
		constructColourTable(nColours, colours);
		image->setColorTable(colours);
		
		for(size_t j = 0; j < columnIndices.size(); j++)
		{
			uchar* reorderedData = image->scanLine((int)j);
			for(size_t i = 0; i < rowIndices.size(); i++)
			{
				std::size_t rowIndex = rowIndices[i], columnIndex = columnIndices[j];
				if(rowIndex > columnIndex) std::swap(rowIndex, columnIndex);
				reorderedData[i] = data[(columnIndex*(columnIndex+1))/2 + rowIndex];
			}
		}
		QPixmap pixMap = QPixmap::fromImage(*image);
		pixMapItems.push_back(graphicsScene->addPixmap(pixMap));
		pixMapItems[0]->setPos(QPoint(0, 0));

		groupItem.reset(new QGraphicsItemGroup);
		graphicsScene->addItem(groupItem.data());
		groupItem->addToGroup(pixMapItems[0]);
		delete image;
	}
	bool imageTile::checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const
	{
		if(rowIndices.size() != otherRowIndices.size() || columnIndices.size() != otherColumnIndices.size())
		{
				return false;
		}
		for(size_t i = 0; i < otherRowIndices.size(); i++)
		{
			if(otherRowIndices[(int)i] != rowIndices[(int)i]) return false;
		}
		for(size_t i = 0; i < otherColumnIndices.size(); i++)
		{
			if(otherColumnIndices[i] != columnIndices[i]) return false;
		}
		return true;
	}
	std::set<imageTile>::const_iterator imageTile::find(const std::set<imageTile, imageTileComparer>& collection, int rowGroup, int columnGroup)
	{
		imageTile toFind;
		toFind.rowGroup = rowGroup;
		toFind.columnGroup = columnGroup;
		return collection.find(toFind);
	}
	const std::vector<int>& imageTile::getRowIndices() const
	{
		return rowIndices;
	}
	const std::vector<int>& imageTile::getColumnIndices() const
	{
		return columnIndices;
	}
	QGraphicsItemGroup* imageTile::getItem() const
	{
		return groupItem.data();
	}
	int imageTile::getRowGroup() const
	{
		return rowGroup;
	}
	int imageTile::getColumnGroup() const
	{
		return columnGroup;
	}
}
