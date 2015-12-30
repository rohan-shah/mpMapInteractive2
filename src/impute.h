#ifndef IMPUTE_HEADER_GUARD
#define IMPUTE_HEADER_GUARD
#include <sstream>
#include "impute.h"
#include <limits>
#include <vector>
bool impute(unsigned char* theta, std::vector<double>& thetaLevels, double* lod, double* lkhd, int nMarkers, std::vector<int>& groups, int group, std::string& error);
#endif
