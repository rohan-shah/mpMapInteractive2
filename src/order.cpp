#include "order.h"
#include <map>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <Rcpp.h>
namespace mpMapInteractive
{
	void rawSymmetricMatrixToDense(double* mem, unsigned char* originalRawData, std::vector<double>& levels, int nOriginalMarkers, const std::vector<int>& permutation, int startIndex, int endIndex, bool& nonZero)
	{
		int nSubMarkers = endIndex - startIndex;
		for(int i = 0; i < nSubMarkers; i++)
		{
			for(int j = 0; j < nSubMarkers; j++)
			{
				double* dest = mem + i + j * nSubMarkers;
				int row = permutation[i + startIndex], column = permutation[j + startIndex];
				if(row > column) std::swap(row, column);
				*dest = levels[originalRawData[(column * (column + 1))/2 + row]];
				nonZero |= (*dest != 0);
			}
		}
	}
}
