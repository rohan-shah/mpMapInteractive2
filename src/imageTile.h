#ifndef IMAGE_TILE_HEADER_GUARD
#define IMAGE_TILE_HEADER_GUARD
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
		rowMajorMatrix<T>& operator=(rowMajorMatrix<T>&& other)
		{
			data = std::move(other.data);
			nRows = other.nRows;
			nColumns = other.nColumns;
			return *this;
		}
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
		friend class imageTileWithAux;
		imageTile(const unsigned char* data, int dataRows, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene, const QVector<QRgb>& colours);
		~imageTile();
		imageTile(imageTile&& other);
		imageTile& operator=(imageTile&& other);
		const std::vector<int>& getRowIndices() const;
		const std::vector<int>& getColumnIndices() const;
		QGraphicsItemGroup* getItem() const;
		bool checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const;
		void deleteMarker(int marker) const;
		void shiftMarkers(int cutStartIndex, int cutEndIndex, int startIndex) const;
	private:
		friend class imageTileWithAux;
		static const int subTileSize = 500;
		void regenerate() const;
		void generateSubTile(int columnStart, int columnEnd, int rowStart, int rowEnd, QImage& image) const;
		void generateSubTile(const std::vector<int>& columnIndices, const std::vector<int>& rowIndices, QImage& image) const;
		//The input to this function is a set of indices into rowIndices / columnIndices (They're required to be identical for this function)
		void makeSubtileBoundariesBefore(std::vector<int>& markerOffsets) const;
		imageTile(const imageTile& other);
		imageTile& operator=(const imageTile& other);
		imageTile(const QVector<QRgb>& colours);
		const unsigned char* data;
		mutable std::vector<int> rowIndices, columnIndices;
		//Vectors that tell us what sub tile row / column a certain row-column is contained in
		mutable std::vector<std::vector<int> > rowPartition, columnPartition;
		mutable rowMajorMatrix<QGraphicsPixmapItem*> pixMapItems;
		std::unique_ptr<QGraphicsItemGroup> groupItem;
		QGraphicsScene* graphicsScene;
		const QVector<QRgb>& colours;
	};
}
#endif
