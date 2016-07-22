#ifndef MPMAPINTERACTIVE2_ARSA_ARGS
#define MPMAPINTERACTIVE2_ARSA_ARGS
#include <vector>
#include <functional>
struct arsaRawArgs
{
	public:
		arsaRawArgs(std::vector<double>& levels, std::vector<int>& permutation)
			:n(-1), rawDist(NULL), cool(0.5), temperatureMin(0.1), nReps(1), randomStart(true), maxMove(0), effortMultiplier(1), levels(levels), permutation(permutation)
		{}
		long n;
		Rbyte* rawDist;
		double cool;
		double temperatureMin;
		long nReps;
		std::function<void(unsigned long,unsigned long)> progressFunction;
		bool randomStart;
		int maxMove;
		double effortMultiplier;
		std::vector<double>& levels;
		std::vector<int>& permutation;
};
struct arsaArgs
{
	public:
		R_xlen_t n;
		double* dist;
		int nReps;
		double temperatureMin, cool;
		double effortMultiplier;
		bool randomStart;
		int maxMove;
		std::vector<int> bestPermutationAllReps;
		std::function<void(unsigned long,unsigned long)> progressFunction;
};
#endif
