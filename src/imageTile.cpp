#include "imageTile.h"
#include "colour.h"
namespace mpMapInteractive
{
	imageTile::~imageTile()
	{
		for(std::vector<QGraphicsPixmapItem*>::iterator i = pixMapItems.getData().begin(); i != pixMapItems.getData().end(); i++)
		{
			delete *i;
		}
	}
	imageTile::imageTile()
		: groupItem(NULL), pixMapItems(0, 0)
	{}
	imageTile::imageTile(imageTile&& other)
		: data(other.data), rowIndices(std::move(other.rowIndices)), columnIndices(std::move(other.columnIndices)), rowGroup(other.rowGroup), columnGroup(other.columnGroup), pixMapItems(std::move(other.pixMapItems)), groupItem(other.groupItem), columnPartition(std::move(other.columnPartition)), rowPartition(std::move(other.rowPartition)), graphicsScene(other.graphicsScene)
	{}
	imageTile::imageTile(std::vector<uchar>* data, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene)
		: data(data), rowIndices(rowIndices), columnIndices(columnIndices), rowGroup(rowGroup), columnGroup(columnGroup), pixMapItems(0, 0), graphicsScene(graphicsScene)
	{
		groupItem.reset(new QGraphicsItemGroup);
		graphicsScene->addItem(groupItem.data());

		regenerate();
	}
	void imageTile::generateSubTile(int columnStart, int columnEnd, int rowStart, int rowEnd, QImage& image)
	{
		std::vector<uchar>& data = *(this->data);
		for(size_t j = columnStart; j < std::min(columnEnd, (int)columnIndices.size()); j++)
		{
			uchar* reorderedData = image.scanLine((int)j - columnStart);
			for(size_t i = rowStart; i < std::min(rowEnd, (int)rowIndices.size()); i++)
			{
				std::size_t rowIndex = rowIndices[i], columnIndex = columnIndices[j];
				if(rowIndex > columnIndex) std::swap(rowIndex, columnIndex);
				reorderedData[i - rowStart] = data[(columnIndex*(columnIndex+1))/2 + rowIndex];
			}
		}
	}
	void imageTile::regenerate()
	{
		for(std::vector<QGraphicsPixmapItem*>::iterator i = pixMapItems.getData().begin(); i != pixMapItems.getData().end(); i++)
		{
			delete *i;
		}
		
		int subTileColumns = (columnIndices.size()+(subTileSize-1))/subTileSize;
		int subTileRows = (rowIndices.size()+(subTileSize-1))/subTileSize;
		pixMapItems.resize(subTileRows, subTileColumns);

		QImage* fullSizeImage = new QImage(subTileSize, subTileSize, QImage::Format_Indexed8);
		
		//get 100 colours
		QVector<QRgb> colours;
		constructColourTable(nColours, colours);
		fullSizeImage->setColorTable(colours);
		std::vector<uchar>& data = *(this->data);

		for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
		{
			int subTileColumnSize = std::min((subTileColumn+1)*subTileSize, (int)columnIndices.size()) - subTileColumn*subTileSize;
			for(int subTileRow = 0; subTileRow < subTileRows; subTileRow++)
			{
				int subTileRowSize = std::min((subTileRow+1)*subTileSize, (int)rowIndices.size()) - subTileRow*subTileSize;
				bool useFullSizeImage = subTileColumnSize == subTileSize && subTileRowSize == subTileSize;

				QImage* currentTileImage;
				if(useFullSizeImage) currentTileImage = fullSizeImage;
				else
				{
					currentTileImage = new QImage(subTileRowSize, subTileColumnSize, QImage::Format_Indexed8);
					currentTileImage->setColorTable(colours);
				}

				generateSubTile(subTileColumn*subTileSize, (subTileColumn+1)*subTileSize, subTileRow*subTileSize, (subTileRow+1)*subTileSize, *currentTileImage);
				QPixmap pixMap = QPixmap::fromImage(*currentTileImage);
				QGraphicsPixmapItem* newItem = graphicsScene->addPixmap(pixMap);
				pixMapItems(subTileRow, subTileColumn) = newItem;
				newItem->setPos(QPoint(subTileRow*subTileSize, subTileColumn*subTileSize));
				groupItem->addToGroup(newItem);

				if(!useFullSizeImage) delete currentTileImage;
			}
		}
		delete fullSizeImage;

		//Data about the column partition
		rowPartition.resize(subTileRows);
		columnPartition.resize(subTileColumns);
		for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
		{
			std::vector<int>& currentEntry = columnPartition[subTileColumn];
			currentEntry.clear();
			int subTileColumnSize = std::min((subTileColumn+1)*subTileSize, (int)columnIndices.size()) - subTileColumn*subTileSize;
			for(size_t j = subTileColumn*subTileSize; j < std::min((subTileColumn+1)*subTileSize, (int)columnIndices.size()); j++)
			{
				currentEntry.push_back(j);
			}
		}
		//And also about the row partition
		for(int subTileRow = 0; subTileRow < subTileRows; subTileRow++)
		{
			std::vector<int>& currentEntry = rowPartition[subTileRow];
			currentEntry.clear();
			int subTileRowSize = std::min((subTileRow+1)*subTileSize, (int)rowIndices.size()) - subTileRow*subTileSize;
			for(size_t i = subTileRow*subTileSize; i < std::min((subTileRow+1)*subTileSize, (int)rowIndices.size()); i++)
			{
				currentEntry.push_back(i);
			}
		}
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
	void imageTile::deleteMarker(int markerIndex)
	{
		//get 100 colours
		QVector<QRgb> colours;
		constructColourTable(nColours, colours);

		int subTileColumns = columnPartition.size();
		int subTileRows = rowPartition.size();
		std::vector<int>::iterator findRow = std::find(rowIndices.begin(), rowIndices.end(), markerIndex);
		if(findRow != rowIndices.end())
		{
			//Position of the row within this image tiles data. 
			int rowCoordinate = (int)std::distance(findRow, rowIndices.end());
			std::vector<std::vector<int> >::iterator partitionIterator = rowPartition.begin();
			while(partitionIterator != rowPartition.end())
			{
				if(std::find(partitionIterator->begin(), partitionIterator->end(), markerIndex) != partitionIterator->end()) break;
			}
			if(partitionIterator == rowPartition.begin()) throw std::runtime_error("Unable to identify marker in rowPartition");
			std::vector<int>& rowPartitionEntry = *partitionIterator;
			int rowIndex = (int)std::distance(rowPartition.begin(), partitionIterator);
			//Are we completely deleting a column of sub-tiles?
			if(partitionIterator->size() == 1)
			{
				//Empty that part of the partition
				partitionIterator->clear();
				//delete the corresponding bits of the image
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					delete pixMapItems(rowIndex, subTileColumn);
					pixMapItems(rowIndex, subTileColumn) = NULL;
				}
			}
			//If not we need to regenerate those tiles that we're deleting from
			else
			{
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					std::vector<int>& columnPartitionEntry = columnPartition[subTileColumn];
					QImage newImage(rowPartitionEntry.size(), columnPartitionEntry.size(), QImage::Format_Indexed8);
					newImage.setColorTable(colours);
					QPixmap pixMap = QPixmap::fromImage(newImage);

					QPointF oldPos = pixMapItems(rowIndex, subTileColumn)->pos();
				}
			}
			//Subtract one from all the rows further on
			for(int subTileRow = rowIndex + 1; subTileRow < subTileRows; subTileRow++)
			{
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					QPointF pos = pixMapItems(subTileRow, subTileColumn)->pos();
					pos.setY(pos.y() - 1);
					pixMapItems(subTileRow, subTileColumn)->setPos(pos);
				}
			}
		}

		std::vector<int>::iterator findColumn = std::find(columnIndices.begin(), columnIndices.end(), markerIndex);
		if(findColumn != columnIndices.end())
		{
		}
	}
}
