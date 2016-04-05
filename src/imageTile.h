#ifndef IMAGE_TILE_HEADER_GUARD
#define IMAGE_TILE_HEADER_GUARD
#include "imageTileComparer.h"
#include <set>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <vector>
namespace mpMapInteractive
{
	class imageTile
	{
	public:
		imageTile(std::vector<uchar>& data, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene);
		~imageTile();
		imageTile(imageTile&& other);
		const std::vector<int>& getRowIndices() const;
		const std::vector<int>& getColumnIndices() const;
		int getRowGroup() const;
		int getColumnGroup() const;
		static std::set<imageTile, imageTileComparer>::const_iterator find(const std::set<imageTile, imageTileComparer>& collection, int rowGroup, int columnGroup);
		QGraphicsItemGroup* getItem() const;
		bool checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const;
	private:
		static const int subTileSize = 500;
		imageTile(const imageTile& other);
		imageTile& operator=(const imageTile& other);
		imageTile();
		std::vector<int> rowIndices, columnIndices;
		int rowGroup, columnGroup;
		//Vectors that tell us what sub tile row / column a certain row-column is contained in
		std::vector<int> rowSubTileIndices, columnSubTileIndices;
		std::vector<QGraphicsPixmapItem*> pixMapItems;
		QSharedPointer<QGraphicsItemGroup> groupItem;
	};
}
#endif
