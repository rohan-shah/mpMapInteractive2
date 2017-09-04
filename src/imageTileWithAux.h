#ifndef IMAGE_TILE_WITH_AUX_HEADER_GUARD
#define IMAGE_TILE_WITH_AUX_HEADER_GUARD
#include "imageTile.h"
#include "imageTileComparer.h"
namespace mpMapInteractive
{
	class imageTileWithAux
	{
	public:
		~imageTileWithAux();
		imageTileWithAux(imageTileWithAux&& other);
		imageTileWithAux& operator=(imageTileWithAux&& other);
		imageTileWithAux(unsigned char* data, const unsigned char* auxData, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene, const QVector<QRgb>& colours, const QVector<QRgb>& auxColours);
		int getRowGroup() const;
		int getColumnGroup() const;
		static std::set<imageTileWithAux, imageTileComparer>::const_iterator find(const std::set<imageTileWithAux, imageTileComparer>& collection, int rowGroup, int columnGroup);
		bool checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const;
		void deleteMarker(int marker) const;
		void shiftMarkers(int cutStartIndex, int cutEndIndex, int startIndex) const;
		QGraphicsItemGroup* getItem() const;
	private:
		imageTileWithAux(const imageTileWithAux& other);
		imageTileWithAux(const QVector<QRgb>& other);
		int rowGroup, columnGroup;
		imageTile theta;
		imageTile* aux;
	};
}
#endif
