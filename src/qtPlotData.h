#ifndef QT_PLOT_DATA_HEADER_GUARD
#define QT_PLOT_DATA_HEADER_GUARD
#include <vector>
#include <string>
#include <functional>
#include <QVector>
#include <QColor>
namespace mpMapInteractive
{
	struct qtPlotData
	{
		typedef bool (*imputeFunctionType)(const unsigned char* theta, unsigned char* imputedTheta, std::vector<double>& thetaLevels, double* lod, double* lkhd, std::vector<int>& markers, std::string& error, std::function<void(unsigned long, unsigned long)> statusFunction);
		qtPlotData(const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames);
		qtPlotData(const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames, std::vector<std::vector<int> >&& cumulativePermutations, std::vector<std::vector<int> >&& groups);
		int startOfGroup(int group) const;
		int endOfGroup(int group) const;
		const std::vector<int>& getCurrentPermutation() const;
		const std::vector<int>& getCurrentGroups() const;
		void applyPermutation(const std::vector<int>& permutation, const std::vector<int>& newGroups);
		const std::vector<std::string> getCurrentMarkerNames() const;
		void undo();
		int stackLength() const;
		bool singleGroup() const;
		int getMarkerCount() const;
		int getOriginalMarkerCount() const;
		const std::vector<std::vector<int> >& getCumulativePermutations() const;
		const std::vector<std::vector<int> >& getCumulativeGroups() const;
		unsigned char* rawImageData;
		const unsigned char* auxiliaryData;

		std::vector<double> levels;
		unsigned char* imputedRawImageData;
		imputeFunctionType imputeFunction;
		
		std::vector<std::string> originalMarkerNames;
		std::vector<std::string> currentMarkerNames;
		
		std::vector<std::vector<int> > cumulativePermutations;
		std::vector<std::vector<int> > groups;

		std::vector<int> originalGroups;
		std::vector<int> identity;
		QVector<QRgb> auxColours;
	private:
		qtPlotData(){}
	};
}
#endif
