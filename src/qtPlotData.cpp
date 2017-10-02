#include "qtPlotData.h"
#include <algorithm>
#include <stdexcept>
namespace mpMapInteractive
{
	int qtPlotData::getOriginalMarkerCount() const
	{
		return (int)originalGroups.size();
	}
	int qtPlotData::stackLength() const
	{
		return cumulativePermutations.size();
	}
	int qtPlotData::getMarkerCount() const
	{
		if(cumulativePermutations.size() == 0)
		{
			return (int)originalMarkerNames.size();
		}
		return (int)cumulativePermutations.rbegin()->size();
	}
	const std::vector<int>& qtPlotData::getCurrentGroups() const
	{
		if(groups.size() == 0) return originalGroups;
		return groups[groups.size()-1];
	}
	int qtPlotData::startOfGroup(int group) const
	{
		const std::vector<int>& currentGroups = getCurrentGroups();
		return std::distance(currentGroups.begin(), std::find(currentGroups.begin(), currentGroups.end(), group));
	}
	bool qtPlotData::singleGroup() const
	{
		const std::vector<int>& currentGroups = getCurrentGroups();
		int group = currentGroups[0];
		for(std::vector<int>::const_iterator i = currentGroups.begin(); i != currentGroups.end(); i++)
		{
			if(*i != group) return false;
		}
		return true;
	}
	int qtPlotData::endOfGroup(int group) const
	{
		const std::vector<int>& currentGroups = getCurrentGroups();
		return std::distance(currentGroups.begin(), std::find(currentGroups.rbegin(), currentGroups.rend(), group).base());
	}
	const std::vector<std::string> qtPlotData::getCurrentMarkerNames() const
	{
		return currentMarkerNames;
	}
	qtPlotData::qtPlotData(const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames)
		:originalMarkerNames(originalMarkerNames), currentMarkerNames(originalMarkerNames),originalGroups(originalGroups)
	{
		if(originalGroups.size() != originalMarkerNames.size()) throw std::runtime_error("Internal error");
		//set up identity permutation initially
		for(int i = 0; i < (int)originalGroups.size(); i++) identity.push_back(i);
	}
	const std::vector<int>& qtPlotData::getCurrentPermutation() const
	{
		if(cumulativePermutations.size() == 0)
		{
			return identity;
		}
		return *cumulativePermutations.rbegin();
	}
	void qtPlotData::undo()
	{
		if(cumulativePermutations.size() != 0)
		{
			cumulativePermutations.pop_back();
			groups.pop_back();
			
			const std::vector<int>& currentCumulativePermutation = getCurrentPermutation();
			currentMarkerNames.clear();
			currentMarkerNames.resize(currentCumulativePermutation.size());
			for(std::size_t i = 0; i < currentCumulativePermutation.size(); i++)
			{
				currentMarkerNames[i] = originalMarkerNames[currentCumulativePermutation[i]];
			}
		}
	}
	void qtPlotData::applyPermutation(const std::vector<int>& permutation, const std::vector<int>& newGroups)
	{
		const std::vector<int>& currentCumulativePermutation = getCurrentPermutation();
		std::vector<int> newCumulativePermutation; 
		newCumulativePermutation.resize(permutation.size());

		currentMarkerNames.resize(permutation.size());

		for(std::size_t i = 0; i < permutation.size(); i++)
		{
			newCumulativePermutation[i] = currentCumulativePermutation[permutation[i]];
			currentMarkerNames[i] = originalMarkerNames[newCumulativePermutation[i]];
		}
		cumulativePermutations.push_back(newCumulativePermutation);
		groups.push_back(newGroups);
	}
	const std::vector<std::vector<int> >& qtPlotData::getCumulativePermutations() const
	{
		return cumulativePermutations;
	}
	const std::vector<std::vector<int> >& qtPlotData::getCumulativeGroups() const
	{
		return groups;
	}
	qtPlotData::qtPlotData(const std::vector<int>& originalGroups, const std::vector<std::string>& originalMarkerNames, std::vector<std::vector<int> >&& cumulativePermutations, std::vector<std::vector<int> >&& groups)
		: rawImageData(NULL), auxiliaryData(NULL), imputedRawImageData(NULL), originalMarkerNames(originalMarkerNames), originalGroups(originalGroups), cumulativePermutations(std::move(cumulativePermutations)), groups(std::move(groups))
	{
		if(originalGroups.size() != originalMarkerNames.size()) throw std::runtime_error("Internal error");
		//set up identity permutation initially
		for(int i = 0; i < (int)originalGroups.size(); i++) identity.push_back(i);

		const std::vector<int>& currentCumulativePermutation = getCurrentPermutation();
		currentMarkerNames.resize(currentCumulativePermutation.size());
		for(std::size_t i = 0; i < currentCumulativePermutation.size(); i++)
		{
			currentMarkerNames[i] = originalMarkerNames[currentCumulativePermutation[i]];
		}
	}
}
