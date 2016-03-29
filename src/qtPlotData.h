#include <vector>
#include <string>
namespace mpMapInteractive
{
	struct qtPlotData
	{
		qtPlotData(const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames);
		int startOfGroup(int group);
		int endOfGroup(int group);
		const std::vector<int>& getCurrentPermutation() const;
		const std::vector<int>& getCurrentGroups() const;
		void applyPermutation(const std::vector<int>& permutation, const std::vector<int>& newGroups);
		const std::vector<std::string> getCurrentMarkerNames() const;
		void undo();
		bool singleGroup();
		int getMarkerCount() const;
	private:
		qtPlotData(){};
		
		std::vector<std::string> originalMarkerNames;
		std::vector<std::string> currentMarkerNames;
		
		std::vector<std::vector<int> > cumulativePermutations;
		std::vector<std::vector<int> > groups;

		std::vector<int> originalGroups;
		std::vector<int> identity;
	};
}
