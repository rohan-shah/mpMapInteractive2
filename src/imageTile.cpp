#include "imageTile.h"
#include "colour.h"
namespace mpMapInteractive
{
	imageTile::~imageTile()
	{
		if(groupItem) graphicsScene->removeItem(groupItem.get());
		for(std::vector<QGraphicsPixmapItem*>::iterator i = pixMapItems.getData().begin(); i != pixMapItems.getData().end(); i++)
		{
			delete *i;
		}
	}
	imageTile::imageTile()
		: pixMapItems(0, 0)
	{}
	imageTile::imageTile(imageTile&& other)
		: data(other.data), rowIndices(std::move(other.rowIndices)), columnIndices(std::move(other.columnIndices)), rowGroup(other.rowGroup), columnGroup(other.columnGroup), pixMapItems(std::move(other.pixMapItems)), groupItem(std::move(other.groupItem)), columnPartition(std::move(other.columnPartition)), rowPartition(std::move(other.rowPartition)), graphicsScene(other.graphicsScene)
	{
	}
	imageTile::imageTile(std::vector<uchar>* data, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene)
		: data(data), rowIndices(rowIndices), columnIndices(columnIndices), rowGroup(rowGroup), columnGroup(columnGroup), pixMapItems(0, 0), graphicsScene(graphicsScene)
	{
		groupItem.reset(new QGraphicsItemGroup);
		graphicsScene->addItem(groupItem.get());

		regenerate();
	}
	void imageTile::generateSubTile(int columnStart, int columnEnd, int rowStart, int rowEnd, QImage& image) const
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
	void imageTile::makeSubtileBoundariesBefore(std::vector<int>& markerOffsets) const
	{
		//There is only one argument, so we require that this tile be symmetric / on a diagonal
		if(rowGroup != columnGroup || pixMapItems.getNRows() != pixMapItems.getNColumns())
		{
			throw std::runtime_error("Cannot call makeSubtileBoundariesBefore except on a symmetric imageTile object");
		}

		//Remove duplicates
		std::sort(markerOffsets.begin(), markerOffsets.end());
		markerOffsets.erase(std::unique(markerOffsets.begin(), markerOffsets.end()), markerOffsets.end());

		//Now get out the subset which are not *already* boundaries
		std::vector<int> notAlreadyBoundaries;

		for(std::vector<int>::iterator markerIterator = markerOffsets.begin(); markerIterator != markerOffsets.end(); markerIterator++)
		{
			//Start going through the rowPartition object, looking for the entry which contains the (*markerIterator)'th entry 
			std::vector<std::vector<int> >::iterator partitionSearch = rowPartition.begin();
			int counter = 0;
			for(; partitionSearch != rowPartition.end(); partitionSearch++)
			{
				counter += partitionSearch->size();
				if(counter > *markerIterator) break;
			}
			std::vector<int>::iterator withinPartitionEntrySearch = std::find(partitionSearch->begin(), partitionSearch->end(), rowIndices[*markerIterator]);
			//We should find the target marker within this entry of the partition
			if(withinPartitionEntrySearch == partitionSearch->end()) throw std::runtime_error("Internal error");
			//We only need to do something if the target marker is not the first in the partition entry
			if(withinPartitionEntrySearch != partitionSearch->begin())
			{
				notAlreadyBoundaries.push_back(*markerIterator);
			}
		}
		if(notAlreadyBoundaries.size() == 0) return;

		std::sort(notAlreadyBoundaries.begin(), notAlreadyBoundaries.end());
		int newTilesCount = pixMapItems.getNRows() + notAlreadyBoundaries.size();
		//New matrix of graphics objects
		rowMajorMatrix<QGraphicsPixmapItem*> newPixMapItems(newTilesCount, newTilesCount, NULL);
		//Set up the new partition. 
		std::vector<std::vector<int> > newPartition(newTilesCount);
		std::vector<std::vector<int> >::iterator newPartitionEntry = newPartition.begin();
		//The next new boundary that we're looking for
		std::vector<int>::iterator boundary = notAlreadyBoundaries.begin();
		//The next boundary could also be an existing partition boundary
		std::vector<std::vector<int> >::iterator previousPartitionIterator = rowPartition.begin();
		//The subtiles that need to be regenerated
		std::vector<int> correspondingOldTiles;
		int oldTilesCounter = 0;
		bool hasOldTile = true;
		for(int counter = 0; counter < rowIndices.size(); counter++)
		{
			if(boundary != notAlreadyBoundaries.end() && counter == *boundary)
			{
				newPartitionEntry++;
				boundary++;
				int newPartitionIndex = (int)std::distance(newPartition.begin(), newPartitionEntry);
				correspondingOldTiles.push_back(-1);
				newPartitionEntry->push_back(rowIndices[counter]);
				hasOldTile = false;
				if(rowIndices[counter] == previousPartitionIterator->back())
				{
					correspondingOldTiles.push_back(-1);
					oldTilesCounter++;
					newPartitionEntry++;
					hasOldTile = true;
					previousPartitionIterator++;
				}
			}
			else if(rowIndices[counter] == previousPartitionIterator->back())
			{
				if(hasOldTile)
				{
					correspondingOldTiles.push_back(oldTilesCounter);
					oldTilesCounter++;
				}
				else
				{
					correspondingOldTiles.push_back(-1);
					hasOldTile = true;
					oldTilesCounter++;
				}
				newPartitionEntry->push_back(rowIndices[counter]);
				newPartitionEntry++;
				previousPartitionIterator++;
			}
			else newPartitionEntry->push_back(rowIndices[counter]);
		}
		if(correspondingOldTiles.size() != newTilesCount)
		{
			throw std::runtime_error("Internal error");
		}
		QVector<QRgb> colours;
		constructColourTable(nColours, colours);
		//Copy across all the subtiles that can be reused, and rebuild the tiles that can't
		int cumulativeX = 0;
		for(int newTileX = 0; newTileX < newTilesCount; newTileX++)
		{
			int cumulativeY = 0;
			for(int newTileY = 0; newTileY < newTilesCount; newTileY++)
			{
				if(correspondingOldTiles[newTileY] == -1 || correspondingOldTiles[newTileX] == -1)
				{
					QImage* currentTileImage = new QImage(newPartition[newTileX].size(), newPartition[newTileY].size(), QImage::Format_Indexed8);
					currentTileImage->setColorTable(colours);
					generateSubTile(newPartition[newTileY], newPartition[newTileX], *currentTileImage);

					QPixmap pixMap = QPixmap::fromImage(*currentTileImage);
					QGraphicsPixmapItem* newItem = graphicsScene->addPixmap(pixMap);
					newPixMapItems(newTileX, newTileY) = newItem;
					newItem->setPos(QPoint(cumulativeX, cumulativeY));
					groupItem->addToGroup(newItem);
					delete currentTileImage;
				}
				else
				{
					QGraphicsPixmapItem*& copiedItem = pixMapItems(correspondingOldTiles[newTileX], correspondingOldTiles[newTileY]);
					if(copiedItem == NULL)
					{
						throw std::runtime_error("Internal error");
					}
					copiedItem->setPos(QPoint(cumulativeX, cumulativeY));
					newPixMapItems(newTileX, newTileY) = copiedItem;
					copiedItem = NULL;
				}
				cumulativeY += newPartition[newTileY].size();
			}
			cumulativeX += newPartition[newTileX].size();
		}
		//Delete all the subtiles that weren't reused
		for(std::vector<QGraphicsPixmapItem*>::iterator i = pixMapItems.getData().begin(); i != pixMapItems.getData().end(); i++)
		{
			delete *i;
		}
		
		//Swap in the new data structures
		pixMapItems.swap(newPixMapItems);
		rowPartition = newPartition;
		columnPartition.swap(newPartition);
	}
	void imageTile::generateSubTile(const std::vector<int>& columnIndices, const std::vector<int>& rowIndices, QImage& image) const
	{
		std::vector<uchar>& data = *(this->data);
		for(size_t j = 0; j < (int)columnIndices.size(); j++)
		{
			uchar* reorderedData = image.scanLine((int)j);
			for(size_t i = 0; i < (int)rowIndices.size(); i++)
			{
				std::size_t rowIndex = rowIndices[i], columnIndex = columnIndices[j];
				if(rowIndex > columnIndex) std::swap(rowIndex, columnIndex);
				reorderedData[i] = data[(columnIndex*(columnIndex+1))/2 + rowIndex];
			}
		}
	}
	void imageTile::regenerate() const
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
				currentEntry.push_back(columnIndices[j]);
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
				currentEntry.push_back(rowIndices[i]);
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
		return groupItem.get();
	}
	int imageTile::getRowGroup() const
	{
		return rowGroup;
	}
	int imageTile::getColumnGroup() const
	{
		return columnGroup;
	}
	void imageTile::deleteMarker(int markerIndex) const
	{
		//get 100 colours
		QVector<QRgb> colours;
		constructColourTable(nColours, colours);

		int subTileColumns = columnPartition.size();
		int subTileRows = rowPartition.size();
		bool shouldRegenerate = false;

	removeMarkerColumn:
		std::vector<int>::iterator findColumn = std::find(columnIndices.begin(), columnIndices.end(), markerIndex);
		if(findColumn != columnIndices.end())
		{
			//Position of the row within this image tiles data. 
			int columnCoordinate = (int)std::distance(columnIndices.begin(), findColumn);
			std::vector<std::vector<int> >::iterator partitionIterator = columnPartition.begin();
			std::vector<int>::iterator columnPartitionEntryIterator;
			while(partitionIterator != columnPartition.end())
			{
				if((columnPartitionEntryIterator = std::find(partitionIterator->begin(), partitionIterator->end(), markerIndex)) != partitionIterator->end()) break;
				partitionIterator++;
			}
			if(partitionIterator == columnPartition.end()) throw std::runtime_error("Unable to identify marker in columnPartition");
			std::vector<int>& columnPartitionEntry = *partitionIterator;
			int columnIndex = (int)std::distance(columnPartition.begin(), partitionIterator);

			columnPartitionEntry.erase(columnPartitionEntryIterator);
			columnIndices.erase(findColumn);
			//Are we completely deleting a column of sub-tiles?
			if(columnPartitionEntry.size() == 0)
			{
				//delete the corresponding bits of the image
				for(int subTileRow = 0; subTileRow < subTileRows; subTileRow++)
				{
					shouldRegenerate = true;
					goto removeMarkerRow;
				}
			}
			//If not we need to regenerate those tiles that we're deleting from
			else
			{
				for(int subTileRow = 0; subTileRow < subTileRows; subTileRow++)
				{
					std::vector<int>& rowPartitionEntry = rowPartition[subTileRow];
					if(rowPartitionEntry.size() == 0 || columnPartitionEntry.size() == 0)
					{
						shouldRegenerate = true;
						goto removeMarkerRow;
					}
					else
					{
						QImage* newImage = new QImage(rowPartitionEntry.size(), columnPartitionEntry.size(), QImage::Format_Indexed8);
						newImage->setColorTable(colours);
						generateSubTile(columnPartitionEntry, rowPartitionEntry, *newImage);
						QPixmap pixMap = QPixmap::fromImage(*newImage);

						QPointF oldPos = pixMapItems(subTileRow, columnIndex)->pos();

						QGraphicsPixmapItem* newItem = graphicsScene->addPixmap(pixMap);
						newItem->setPos(oldPos);
						delete pixMapItems(subTileRow, columnIndex);
						pixMapItems(subTileRow, columnIndex) = newItem;
						delete newImage;
					}
				}
			}
			//Subtract one from all the column further on
			for(int subTileColumn = columnIndex + 1; subTileColumn < subTileColumns; subTileColumn++)
			{
				for(int subTileRow = 0; subTileRow < subTileRows; subTileRow++)
				{
					if(pixMapItems(subTileRow, subTileColumn) != NULL)
					{
						QPointF pos = pixMapItems(subTileRow, subTileColumn)->pos();
						pos.setY(pos.y() - 1);
						pixMapItems(subTileRow, subTileColumn)->setPos(pos);
					}
				}
			}
		}
	removeMarkerRow:
		std::vector<int>::iterator findRow = std::find(rowIndices.begin(), rowIndices.end(), markerIndex);
		if(findRow != rowIndices.end())
		{
			//Position of the row within this image tiles data. 
			int rowCoordinate = (int)std::distance(rowIndices.begin(), findRow);
			std::vector<std::vector<int> >::iterator partitionIterator = rowPartition.begin();
			std::vector<int>::iterator rowPartitionEntryIterator;
			while(partitionIterator != rowPartition.end())
			{
				if((rowPartitionEntryIterator = std::find(partitionIterator->begin(), partitionIterator->end(), markerIndex)) != partitionIterator->end()) break;
				partitionIterator++;
			}
			if(partitionIterator == rowPartition.end()) throw std::runtime_error("Unable to identify marker in rowPartition");
			std::vector<int>& rowPartitionEntry = *partitionIterator;
			int rowIndex = (int)std::distance(rowPartition.begin(), partitionIterator);
			
			rowPartitionEntry.erase(rowPartitionEntryIterator);
			rowIndices.erase(findRow);
			//Are we completely deleting a column of sub-tiles?
			if(rowPartitionEntry.size() == 0)
			{
				//delete the corresponding bits of the image
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					shouldRegenerate = true;
					goto checkRegenerate;
				}
			}
			//If not we need to regenerate those tiles that we're deleting from
			else
			{
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					std::vector<int>& columnPartitionEntry = columnPartition[subTileColumn];
					if(columnPartitionEntry.size() == 0 || rowPartitionEntry.size() == 0)
					{
						shouldRegenerate = true;
						goto checkRegenerate;
					}
					else
					{
						QImage* newImage = new QImage(rowPartitionEntry.size(), columnPartitionEntry.size(), QImage::Format_Indexed8);
						newImage->setColorTable(colours);
						generateSubTile(columnPartitionEntry, rowPartitionEntry, *newImage);
						QPixmap pixMap = QPixmap::fromImage(*newImage);
		
						QPointF oldPos = pixMapItems(rowIndex, subTileColumn)->pos();

						QGraphicsPixmapItem* newItem = graphicsScene->addPixmap(pixMap);
						newItem->setPos(oldPos);
						delete pixMapItems(rowIndex, subTileColumn);
						pixMapItems(rowIndex, subTileColumn) = newItem;
						delete newImage;
					}
				}
			}
			//Subtract one from all the rows further on
			for(int subTileRow = rowIndex + 1; subTileRow < subTileRows; subTileRow++)
			{
				for(int subTileColumn = 0; subTileColumn < subTileColumns; subTileColumn++)
				{
					if(pixMapItems(subTileRow, subTileColumn) != NULL)
					{
						QPointF pos = pixMapItems(subTileRow, subTileColumn)->pos();
						pos.setX(pos.x() - 1);
						pixMapItems(subTileRow, subTileColumn)->setPos(pos);
					}
				}
			}
		}
	checkRegenerate:
		if(shouldRegenerate) regenerate();
	}
	void imageTile::shiftMarkers(int cutStartIndex, int cutEndIndex, int startIndex) const
	{
		//This tile has to be symmetric, otherwise throw an error
		if(rowGroup != columnGroup)
		{
			throw std::runtime_error("Cannot call shiftMarkers except on a symmetric imageTile object");
		}
		if(startIndex >= cutStartIndex && startIndex <= cutEndIndex+1)
		{
			throw std::runtime_error("Trying to cut and paste to the same location");
		}
		std::vector<int> additionalBoundaries;
		if(cutStartIndex != 0) additionalBoundaries.push_back(cutStartIndex);
		if(cutEndIndex != rowIndices.size()-1) additionalBoundaries.push_back(cutEndIndex+1);
		if(startIndex != 0) additionalBoundaries.push_back(startIndex);

		makeSubtileBoundariesBefore(additionalBoundaries);
		int cutStartSubtile, cutEndSubtile, startSubtile;
		for(std::vector<std::vector<int> >::iterator i = rowPartition.begin(); i != rowPartition.end(); i++)
		{
			int j = (int)std::distance(rowPartition.begin(), i);
			if(i->front() == rowIndices[cutStartIndex]) cutStartSubtile = j;
			if(i->front() == rowIndices[startIndex]) startSubtile = j;
			if(i->back() == rowIndices[cutEndIndex]) cutEndSubtile = j;
		}
		//Now it's going to be easy to do the cut and replace. 
		int newSubtilesCount = pixMapItems.getNRows();
		//New matrix of subtiles
		rowMajorMatrix<QGraphicsPixmapItem*> newPixMapItems(newSubtilesCount, newSubtilesCount);
		int cutSize = cutEndIndex - cutStartIndex + 1;
		int cutSizeSubtile = cutEndSubtile - cutStartSubtile + 1;
		if(startIndex < cutStartIndex)
		{
			//Indices and partition first
			for(int i = startIndex; i < startIndex + cutSize; i++)
			{
				rowIndices[i] = columnIndices[i - startIndex + cutStartIndex];
			}
			for(int i = startSubtile; i < startSubtile + cutSizeSubtile; i++)
			{
				rowPartition[i] = columnPartition[i - startSubtile + cutStartSubtile];
			}
			for(int i = startIndex + cutSize; i < cutEndIndex+1; i++)
			{
				rowIndices[i] = columnIndices[i - cutSize];
			}
			for(int i = startSubtile + cutSizeSubtile; i < cutEndSubtile + 1; i++)
			{
				rowPartition[i] = columnPartition[i - cutSizeSubtile];
			}
			//Now the image subtiles. This is a horrible mess. 
			int cumulativeX = 0;
			for(int i = 0; i < startSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < startSubtile + cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - startSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile + cutSizeSubtile; j <= cutEndSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutEndSubtile + 1; j < newSubtilesCount; j++) newPixMapItems(i, j) = pixMapItems(i, j);
				cumulativeX += rowPartition[i].size();
			}
			for(int i = startSubtile; i < startSubtile + cutSizeSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(cutStartSubtile + (i - startSubtile), j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < startSubtile + cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(cutStartSubtile + (i - startSubtile), cutStartSubtile + (j - startSubtile));
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile + cutSizeSubtile; j <= cutEndSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(cutStartSubtile + (i - startSubtile), j - cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutEndSubtile + 1; j < newSubtilesCount; j++)
				{
					newPixMapItems(i, j) = pixMapItems(cutStartSubtile + (i - startSubtile), j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				cumulativeX += rowPartition[i].size();
			}
			for(int i = startSubtile + cutSizeSubtile; i <= cutEndSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - cutSizeSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < startSubtile + cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - cutSizeSubtile, j - startSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile + cutSizeSubtile; j <= cutEndSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - cutSizeSubtile, j - cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutEndSubtile + 1; j < newSubtilesCount; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - cutSizeSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				cumulativeX += rowPartition[i].size();
			}
			for(int i = cutEndSubtile + 1; i < newSubtilesCount; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < startSubtile + cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - startSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile + cutSizeSubtile; j <= cutEndSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutEndSubtile + 1; j < newSubtilesCount; j++) newPixMapItems(i, j) = pixMapItems(i, j);
				cumulativeX += rowPartition[i].size();
			}
		}
		else
		{
			//Indices and partition first
			for(int i = cutStartIndex; i < startIndex - cutSize; i++)
			{
				rowIndices[i] = columnIndices[i + cutSize];
			}
			for(int i = cutStartSubtile; i < startSubtile - cutSizeSubtile; i++)
			{
				rowPartition[i] = columnPartition[i + cutSizeSubtile];
			}
			for(int i = startIndex - cutSize; i < startIndex; i++)
			{
				rowIndices[i] = columnIndices[i - (startIndex - cutSize) + cutStartIndex];
			}
			for(int i = startSubtile - cutSizeSubtile; i < startSubtile; i++)
			{
				rowPartition[i] = columnPartition[i - (startSubtile - cutSizeSubtile) + cutStartSubtile];
			}
			//Now the image subtiles. This is a horrible mess.
			int cumulativeX = 0;
			for(int i = 0; i < cutStartSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < cutStartSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutStartSubtile; j < startSubtile - cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j + cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile - cutSizeSubtile; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - startSubtile + cutSizeSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < newSubtilesCount; j++) newPixMapItems(i, j) = pixMapItems(i, j);
				cumulativeX += rowPartition[i].size();
			}
			for(int i = cutStartSubtile; i < startSubtile - cutSizeSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < cutStartSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i + cutSizeSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutStartSubtile; j < startSubtile - cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i + cutSizeSubtile, j + cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile - cutSizeSubtile; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i + cutSizeSubtile, j - startSubtile + cutSizeSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < newSubtilesCount; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i + cutSizeSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				cumulativeX += rowPartition[i].size();
			}
			for(int i = startSubtile - cutSizeSubtile; i < startSubtile; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < cutStartSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - startSubtile + cutSizeSubtile + cutStartSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutStartSubtile; j < startSubtile - cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - startSubtile + cutSizeSubtile + cutStartSubtile, j + cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile - cutSizeSubtile; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i - startSubtile + cutSizeSubtile + cutStartSubtile, j - startSubtile + cutSizeSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < newSubtilesCount; j++) 
				{
					newPixMapItems(i, j) = pixMapItems(i - startSubtile + cutSizeSubtile + cutStartSubtile, j);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				cumulativeX += rowPartition[i].size();
			}
			for(int i = startSubtile; i < newSubtilesCount; i++)
			{
				int cumulativeY = 0;
				for(int j = 0; j < cutStartSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = cutStartSubtile; j < startSubtile - cutSizeSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j + cutSizeSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile - cutSizeSubtile; j < startSubtile; j++)
				{
					newPixMapItems(i, j) = pixMapItems(i, j - startSubtile + cutSizeSubtile + cutStartSubtile);
					newPixMapItems(i, j)->setPos(cumulativeX, cumulativeY);
					cumulativeY += rowPartition[j].size();
				}
				for(int j = startSubtile; j < newSubtilesCount; j++) newPixMapItems(i, j) = pixMapItems(i, j);
				cumulativeX += rowPartition[i].size();
			}
		}
		columnIndices = rowIndices;
		columnPartition = rowPartition;

		//No need to delete anything here, because we're just re-arranging the image bits. 
		pixMapItems.swap(newPixMapItems);
	}
}
