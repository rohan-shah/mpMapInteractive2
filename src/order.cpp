#include "order.h"
#include <map>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <Rcpp.h>
namespace mpMapInteractive
{
	void order(unsigned char* originalRawData, std::vector<double>& levels, int nOriginalMarkers, const std::vector<int>& permutation, int startIndex, int endIndex, std::vector<int>& resultingPermutation)
	{
		int nSubMarkers = endIndex - startIndex;
		Rcpp::NumericMatrix subMatrix(nSubMarkers, nSubMarkers);

		Rcpp::Function asDist("as.dist"), seriate("seriate"), getOrder("get_order"), criterion("criterion");
		
		double* mem = &(subMatrix(0,0));
		//Determine whether or not the submatrix is zero, as seriation seems to screw up for all-zero matrices.
		bool nonZero = false;
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
		if(nonZero)
		{
			Rcpp::RObject distSubMatrix = asDist(Rcpp::Named("m") = subMatrix);

			Rcpp::List nRepsControl = Rcpp::List::create(Rcpp::Named("nreps") = 5);
			Rcpp::RObject result = seriate(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("method") = "ARSA", Rcpp::Named("control") = nRepsControl);
			std::vector<int> arsaResult = Rcpp::as<std::vector<int> >(getOrder(result));
			double arsaCriterionResult = Rcpp::as<double>(criterion(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("order") = result, Rcpp::Named("method") = "AR_events"));

			//now consider identity permutation
			double identityCriterionResult = Rcpp::as<double>(criterion(Rcpp::Named("x") = distSubMatrix, Rcpp::Named("method") = "AR_events"));

			if(arsaCriterionResult < identityCriterionResult)
			{
				resultingPermutation.swap(arsaResult);
			}
			else
			{
				std::vector<int> identity; 
				for(int i = 0; i < nSubMarkers; i++) identity.push_back(i+1);
				resultingPermutation.swap(identity);
			}
			//Permutation stuff in R is indexed at base 1, but we want it indexed starting at 0.
			for(int i = 0; i < nSubMarkers; i++) resultingPermutation[i] -= 1;
		}
		else
		{
			resultingPermutation.resize(nSubMarkers);
			for(int i = 0; i < nSubMarkers; i++) resultingPermutation[i] = i;
		}
	}
}
