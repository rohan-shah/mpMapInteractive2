#ifndef IMAGE_TILE_HEADER_GUARD
#define IMAGE_TILE_HEADER_GUARD
#include "imageTileComparer.h"
#include <set>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <vector>
#include <memory>
namespace mpMapInteractive
{
	template<typename T> class rowMajorMatrix
	{
	public:
		rowMajorMatrix(int nRows, int nColumns)
			: nRows(nRows), nColumns(nColumns), data(nRows*nColumns)
		{}
		rowMajorMatrix(int nRows, int nColumns, T value)
			: nRows(nRows), nColumns(nColumns), data(nRows*nColumns, value)
		{}
		rowMajorMatrix(rowMajorMatrix<T>&& other)
			:data(std::move(other.data)), nRows(other.nRows), nColumns(other.nColumns)
		{}
		void resize(int nRows, int nColumns)
		{
			data.resize(nRows*nColumns);
			this->nRows = nRows;
			this->nColumns = nColumns;
		}
		typename std::vector<T>::reference operator()(int row, int column)
		{
			return data[column + row*nColumns];
		}
		typename std::vector<T>::const_reference operator()(int row, int column) const
		{
			return data[column + row*nColumns];
		}
		int getNRows() const
		{
			return nRows;
		}
		int getNColumns() const
		{
			return nColumns;
		}
		void swap(rowMajorMatrix<T>& other)
		{
			nRows = other.nRows;
			nColumns = other.nColumns;
			data.swap(other.data);
		}
		std::vector<T>& getData()
		{
			return data;
		}
	private:
		int nRows, nColumns;
		std::vector<T> data;
	};
	class imageTile
	{
	public:
		imageTile(std::vector<uchar>* data, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene);
		~imageTile();
		imageTile(imageTile&& other);
		const std::vector<int>& getRowIndices() const;
		const std::vector<int>& getColumnIndices() const;
		int getRowGroup() const;
		int getColumnGroup() const;
		static std::set<imageTile, imageTileComparer>::const_iterator find(const std::set<imageTile, imageTileComparer>& collection, int rowGroup, int columnGroup);
		QGraphicsItemGroup* getItem() const;
		bool checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const;
		//These don't change the sort order (which is based on rowGroup and columnGroup), so allow them to be called on const objects
		void deleteMarker(int marker) const;
		void shiftMarkers(int cutStartIndex, int cutEndIndex, int startIndex) const;
	private:
		static const int subTileSize = 500;
		void regenerate() const;
		void generateSubTile(int columnStart, int columnEnd, int rowStart, int rowEnd, QImage& image) const;
		void generateSubTile(const std::vector<int>& columnIndices, const std::vector<int>& rowIndices, QImage& image) const;
		//The input to this function is a set of indices into rowIndices / columnIndices (They're required to be identical for this function)
		void makeSubtileBoundariesBefore(std::vector<int>& markerOffsets) const;
		imageTile(const imageTile& other);
		imageTile& operator=(const imageTile& other);
		imageTile();
		std::vector<uchar>* data;
		mutable std::vector<int> rowIndices, columnIndices;
		int rowGroup, columnGroup;
		//Vectors that tell us what sub tile row / column a certain row-column is contained in
		mutable std::vector<std::vector<int> > rowPartition, columnPartition;
		mutable rowMajorMatrix<QGraphicsPixmapItem*> pixMapItems;
		std::unique_ptr<QGraphicsItemGroup> groupItem;
		QGraphicsScene* graphicsScene;
	};
}
#endif
