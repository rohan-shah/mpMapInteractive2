#include "imageTileWithAux.h"
namespace mpMapInteractive
{
	imageTileWithAux::~imageTileWithAux()
	{
		delete aux;
	}
	imageTileWithAux::imageTileWithAux(unsigned char* data, const unsigned char* auxData, int dataRows, int rowGroup, int columnGroup, const std::vector<int>& rowIndices, const std::vector<int>& columnIndices, QGraphicsScene* graphicsScene, const QVector<QRgb>& colours, const QVector<QRgb>& auxColours)
		: rowGroup(rowGroup), columnGroup(columnGroup), theta(data, dataRows, rowIndices, columnIndices, graphicsScene, colours), aux(NULL)
	{
		if(auxData != NULL) aux = new imageTile(auxData, dataRows, rowIndices, columnIndices, graphicsScene, auxColours);
	}
	imageTileWithAux::imageTileWithAux(imageTileWithAux&& other)
		: rowGroup(other.rowGroup), columnGroup(other.columnGroup), theta(std::move(other.theta))
	{
		aux = other.aux;
		other.aux = NULL;
	}
	imageTileWithAux& imageTileWithAux::operator=(imageTileWithAux&& other)
	{
		rowGroup = other.rowGroup;
		columnGroup = other.columnGroup;
		theta = std::move(theta);
		aux = other.aux;
		other.aux = NULL;
	}
	int imageTileWithAux::getRowGroup() const
	{
		return rowGroup;
	}
	int imageTileWithAux::getColumnGroup() const
	{
		return columnGroup;
	}
	QGraphicsItemGroup* imageTileWithAux::getItem() const
	{
		return theta.getItem();
	}
	void imageTileWithAux::shiftMarkers(int cutStartIndex, int cutEndIndex, int startIndex) const
	{
		//This tile has to be symmetric, otherwise throw an error
		if(rowGroup != columnGroup)
		{
			throw std::runtime_error("Cannot call shiftMarkers except on a symmetric imageTile object");
		}
		theta.shiftMarkers(cutStartIndex, cutEndIndex, startIndex);
		if(aux != NULL) aux->shiftMarkers(cutStartIndex, cutEndIndex, startIndex);
	}
	std::set<imageTileWithAux>::const_iterator imageTileWithAux::find(const std::set<imageTileWithAux, imageTileComparer>& collection, int rowGroup, int columnGroup)
	{
		QVector<QRgb> colours;
		imageTileWithAux toFind(colours);
		toFind.rowGroup = rowGroup;
		toFind.columnGroup = columnGroup;
		return collection.find(toFind);
	}
	bool imageTileWithAux::checkIndices(const std::vector<int>& otherRowIndices, const std::vector<int>& otherColumnIndices) const
	{
		bool result = theta.checkIndices(otherRowIndices, otherColumnIndices);
		if(aux != NULL) aux->checkIndices(otherRowIndices, otherColumnIndices);
		return result;
	}
	void imageTileWithAux::deleteMarker(int marker) const
	{
		theta.deleteMarker(marker);
		if(aux != NULL) aux->deleteMarker(marker);
	}
	imageTileWithAux::imageTileWithAux(const QVector<QRgb>& colours)
		:theta(colours), aux(NULL)
	{}
}
