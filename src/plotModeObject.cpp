#include "plotModeObject.h"
namespace mpMapInteractive
{
	void plotModeObject::doImputation(int group)
	{
		int nOriginalMarkers = data.getOriginalMarkerCount();
		//Construct a vector of linkage groups (newGroups) which assigns a group to EVERY MARKER ORIGINALLY PRESENT
		const std::vector<int>& oldGroups = data.getCurrentGroups();
		int additionalGroupNumber = *std::max_element(oldGroups.begin(), oldGroups.end()) + 1;
		std::vector<int> newGroups(nOriginalMarkers, additionalGroupNumber);
		const std::vector<int>& currentPermutation = data.getCurrentPermutation();
		for(size_t i = 0; i < currentPermutation.size(); i++) newGroups[currentPermutation[i]] = oldGroups[i];


		std::vector<int> markersInRelevantGroup;
		for (std::size_t i = 0; i < newGroups.size(); i++)
		{
			if (newGroups[i] == group) markersInRelevantGroup.push_back((int)i);
		}

		std::string error;
		std::function<void(unsigned long, unsigned long)> progressFunction = [](unsigned long, unsigned long){};
		bool ok = imputeFunction(*imputedRawData, levels, NULL, NULL, markersInRelevantGroup, error, progressFunction);
		if(!ok) throw std::runtime_error("Imputation failed!");
	}
}
