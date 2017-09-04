#include "imageTileComparer.h"
#include "imageTileWithAux.h"
namespace mpMapInteractive
{
	bool imageTileComparer::operator()(const imageTileWithAux& first, const imageTileWithAux& second) const
	{
		int firstRowGroup = first.getRowGroup(), secondRowGroup = second.getRowGroup();
		if(firstRowGroup < secondRowGroup) return true;
		if(secondRowGroup < firstRowGroup) return false;
		int firstColumnGroup = first.getColumnGroup(), secondColumnGroup = second.getColumnGroup();
		return firstColumnGroup < secondColumnGroup;
	}
}
