#include <Rcpp.h>
#include <QtWidgets/qapplication.h>
#include "qtPlot.h"
extern "C"
{
	RcppExport SEXP qtPlotMpMap2(SEXP mpcross__, SEXP cumulativePermutations_sexp, SEXP cumulativeGroups_sexp, SEXP auxiliaryData_sexp)
	{
	BEGIN_RCPP

		Rcpp::S4 mpcross, rf, lg;
		try
		{
			mpcross = mpcross__;
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross must be an S4 object");
		}
		if(!mpcross.is("mpcrossLG"))
		{
			throw Rcpp::not_compatible("Input mpcross must extend class mpcrossLG");
		}
		Rcpp::Function validObject("validObject");
		try
		{
			Rcpp::LogicalVector result = validObject(mpcross, Rcpp::Named("complete") = Rcpp::wrap(true));
			if(!Rcpp::as<bool>(result))
			{
				throw Rcpp::exception("Invalid mpcrossLG object. Please ensure that validObject returns TRUE.");
			}
		}
		catch(...)
		{
			std::stringstream ss;
			ss << "Invalid mpcrossLG object. Please ensure that validObject returns TRUE." << std::endl;
			throw Rcpp::exception(ss.str().c_str());
		}

		try
		{
			lg = Rcpp::as<Rcpp::S4>(mpcross.slot("lg"));
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@lg must be an S4 object");
		}
		if(!lg.is("lg"))
		{
			throw Rcpp::not_compatible("Input mpcross@lg must extend class lg");
		}

		Rcpp::Function markerNamesFunc("markers");
		std::vector<std::string> markerNames = Rcpp::as<std::vector<std::string> >(markerNamesFunc(mpcross));
		unsigned long long nMarkers = markerNames.size();

		const unsigned char* auxiliaryPointer = NULL;
		std::vector<unsigned char> auxiliaryData;
		if(!Rcpp::as<Rcpp::RObject>(auxiliaryData_sexp).isNULL())
		{
			Rcpp::NumericMatrix auxiliaryDataSymmetric = auxiliaryData_sexp;
			auto minmax = std::minmax_element(auxiliaryDataSymmetric.begin(), auxiliaryDataSymmetric.end());
			double min = *minmax.first;
			double max = *minmax.second;
			if(min != min || max != max) throw std::runtime_error("Auxiliary data cannot have missing values");
			double range = max - min;
			double incrementSize = range / 255;

			auxiliaryData.resize(nMarkers * (nMarkers + 1ULL) / 2ULL);
			for(unsigned long long column = 0; column < nMarkers; column++)
			{
				for(unsigned long long row = 0; row <= column; row++)
				{
					auxiliaryData[(column * (column + 1ULL)) / 2ULL + row] = (unsigned char)std::floor((auxiliaryDataSymmetric(row, column) - min) / incrementSize);
				}
			}
			auxiliaryPointer = &(auxiliaryData[0]);
		}

		bool hasRF;
		Rcpp::S4 theta;
		Rcpp::RawVector thetaData;
		Rcpp::NumericVector thetaLevels;
		std::vector<double> thetaLevelsVector;
		if(Rcpp::as<Rcpp::RObject>(mpcross.slot("rf")).isNULL())
		{
			hasRF = false;
		}
		else
		{
			try
			{
				rf = Rcpp::as<Rcpp::S4>(mpcross.slot("rf"));
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input mpcross@rf must be an S4 object");
			}
			if(!rf.is("rf"))
			{
				throw Rcpp::not_compatible("Input mpcross@rf must extend class rf");
			}
			try
			{
				theta = Rcpp::as<Rcpp::S4>(rf.slot("theta"));
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input mpcross@rf@theta must be a numeric matrix");
			}

			try
			{
				thetaData = theta.slot("data");
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input mpcross@rf@theta@data must be a raw vector");
			}

			try
			{
				thetaLevels = theta.slot("levels");
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input mpcross@rf@theta@levels must be a numeric vector");
			}
			thetaLevelsVector = Rcpp::as<std::vector<double> >(thetaLevels);
			hasRF = true;
		}
		
		std::vector<int> groups;
		try
		{
			groups = Rcpp::as<std::vector<int> >(lg.slot("groups"));
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@lg@groups must be an integer vector");
		}

		std::vector<int> allGroups;
		try
		{
			allGroups = Rcpp::as<std::vector<int> >(lg.slot("allGroups"));
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@lg@allGroups must be an integer vector");
		}

		Rcpp::RObject cumulativePermutations_robject = cumulativePermutations_sexp, cumulativeGroups_robject = cumulativeGroups_sexp;
		if(cumulativePermutations_robject.isNULL() ^ cumulativeGroups_robject.isNULL())
		{
			throw std::runtime_error("Inputs cumulativePermutations and cumulativeGroups must both be specified");
		}

		unsigned char* imputedRawImageData = NULL;
		if(allGroups.size() == 1)
		{
			Rcpp::RObject imputedThetaObj = lg.slot("imputedTheta");
			if(!imputedThetaObj.isNULL())
			{
				try
				{
					Rcpp::List imputedTheta = Rcpp::as<Rcpp::List>(imputedThetaObj);
					theta = Rcpp::as<Rcpp::S4>(imputedTheta(0));
					thetaData = Rcpp::as<Rcpp::RawVector>(theta.slot("data"));
					thetaLevelsVector = Rcpp::as<std::vector<double> >(theta.slot("levels"));
					imputedRawImageData = new unsigned char[thetaData.size()];
					memcpy(imputedRawImageData, &(thetaData(0)), sizeof(unsigned char)*thetaData.size());
				}
				catch(...)
				{
					delete[] imputedRawImageData;
					throw std::runtime_error("Error while reading the imputed data in mpcross@lg@imputedTheta");
				}
			}
			else if(!hasRF)
			{
				throw std::runtime_error("Input mpcross@rf must be present");
			}
		}
		else if(!hasRF)
		{
			throw std::runtime_error("Input mpcross@rf must be present if there is more than one group");
		}

		//Check that every group is represented as a contiguous chunk of markers.
		for(std::vector<int>::iterator currentGroup = allGroups.begin(); currentGroup != allGroups.end(); currentGroup++)
		{
			std::vector<int>::iterator startOfGroup = std::find(groups.begin(), groups.end(), *currentGroup);
			std::vector<int>::reverse_iterator endOfGroupReverse = std::find(groups.rbegin(), groups.rend(), *currentGroup);
			if(startOfGroup != groups.end() && endOfGroupReverse != groups.rend())
			{
				std::vector<int>::iterator endOfGroup = endOfGroupReverse.base();
				for(std::vector<int>::iterator current = startOfGroup; current != endOfGroup; current++)
				{
					if(*current != *currentGroup)
					{
						throw std::runtime_error("Markers for at least one linkage group were non-contiguous");
					}
				}
			}
		}

		//Show qt application
		int argc = 1;
		char* argv[3];
		argv[0] = new char[1];
		argv[1] = new char[1];
		argv[2] = new char[1];
		argv[0][0] = argv[1][0] = argv[2][0] = 0;
		QApplication app(argc, argv);

		DL_FUNC imputeFunctionUntyped = R_GetCCallable("mpMap2", "impute");
		if (imputeFunctionUntyped == NULL) throw std::runtime_error("Unable to access imputation function of package mpMap2");
		mpMapInteractive::qtPlotData::imputeFunctionType imputeFunction = (mpMapInteractive::qtPlotData::imputeFunctionType)imputeFunctionUntyped;

		QSharedPointer<mpMapInteractive::qtPlotData> inputData;
		if(cumulativePermutations_robject.isNULL() && cumulativeGroups_robject.isNULL())
		{
			inputData.reset(new mpMapInteractive::qtPlotData(groups, markerNames));
		}
		else
		{
			Rcpp::List cumulativePermutations_list = Rcpp::as<Rcpp::List>(cumulativePermutations_robject);
			Rcpp::List cumulativeGroups_list = Rcpp::as<Rcpp::List>(cumulativeGroups_robject);
			if(cumulativePermutations_list.size() != cumulativeGroups_list.size())
			{
				throw std::runtime_error("Inputs cumulativePermutations and cumulativeGroups should be lists of the same length");
			}
			std::vector<std::vector<int> > cumulativePermutations, cumulativeGroups;
			for(int i = 0; i < cumulativePermutations_list.size(); i++)
			{
				std::vector<int> currentPermutationEntry = Rcpp::as<std::vector<int> >(cumulativePermutations_list(i));
				//sort the permutation entry, check that values are unique
				std::sort(currentPermutationEntry.begin(), currentPermutationEntry.end());
				if(std::unique(currentPermutationEntry.begin(), currentPermutationEntry.end()) != currentPermutationEntry.end())
				{
					throw std::runtime_error("Input permutation data cannot have repeated markers");
				}
				//Check the range of the values
				if(*std::max_element(currentPermutationEntry.begin(), currentPermutationEntry.end()) >= nMarkers || *std::min_element(currentPermutationEntry.begin(), currentPermutationEntry.end()) < 0)
				{
					throw std::runtime_error("Input permutation data contained values outside the allowed range");
				}
				//Actually add the permutation
				cumulativePermutations.push_back(Rcpp::as<std::vector<int> >(cumulativePermutations_list(i)));
				//Also add the corresponding groups change. 
				cumulativeGroups.push_back(Rcpp::as<std::vector<int> >(cumulativeGroups_list(i)));
				if(cumulativePermutations.back().size() != cumulativeGroups.back().size())
				{
					throw std::runtime_error("Every vector in input groups must have the same length as the corresponding entry in permutations");
				}
			}
			inputData.reset(new mpMapInteractive::qtPlotData(groups, markerNames, std::move(cumulativePermutations), std::move(cumulativeGroups)));
		}
		inputData->auxiliaryData = auxiliaryPointer;
		inputData->auxColours.resize(256);
		for(int i = 0; i < 256; i++)
		{
			QColor colour;
			colour.setHsvF(i* (1.0f/6.0f)/255.0, 1.0, 1.0);
			inputData->auxColours[i] = colour.rgb();
		}

		inputData->rawImageData = &(thetaData(0));
		inputData->levels = thetaLevelsVector;
		inputData->imputedRawImageData = imputedRawImageData;
		inputData->imputeFunction = imputeFunction;

		mpMapInteractive::qtPlot plot(inputData);
		plot.show();
		app.exec();

		const mpMapInteractive::qtPlotData& outputData = plot.getData();
		std::vector<int> outputGroups = outputData.getCurrentGroups();
		std::vector<std::string> outputMarkerNames = outputData.getCurrentMarkerNames();
		const std::vector<std::vector<int> >& cumulativePermutations = outputData.getCumulativePermutations();
		const std::vector<std::vector<int> >& cumulativeGroups = outputData.getCumulativeGroups();

		delete[] argv[0];
		delete[] argv[1];
		delete[] argv[2];

		Rcpp::CharacterVector convertedOutputMarkerNames = Rcpp::wrap(outputMarkerNames);
		Rcpp::IntegerVector convertedOutputGroups = Rcpp::wrap(outputGroups);

		Rcpp::List convertedCumulativePermutations(cumulativePermutations.size());
		for(int i = 0 ; i < cumulativePermutations.size(); i++) convertedCumulativePermutations(i) = Rcpp::wrap(cumulativePermutations[i]);

		Rcpp::List convertedCumulativeGroups(cumulativeGroups.size());
		for(int i = 0 ; i < cumulativeGroups.size(); i++) convertedCumulativeGroups(i) = Rcpp::wrap(cumulativeGroups[i]);

		Rcpp::List retVal = Rcpp::List::create(convertedOutputMarkerNames, convertedOutputGroups, convertedCumulativePermutations, convertedCumulativeGroups);
		return retVal;
	END_RCPP
	}
}
