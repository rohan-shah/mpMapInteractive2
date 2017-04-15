#include <Rcpp.h>
#include <QtWidgets/qapplication.h>
#include "qtPlot.h"
extern "C"
{
	RcppExport SEXP qtPlotMpMap2(SEXP mpcross__, SEXP auxillaryNumeric__)
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

		//check the auxillary numeric matrix
		double* auxillaryPointer = NULL;
		int auxRows = 0;
		Rcpp::RObject auxillaryNumeric_(auxillaryNumeric__);
		if(auxillaryNumeric_.sexp_type() != NILSXP)
		{
			Rcpp::NumericMatrix auxillaryNumeric;
			try
			{
				auxillaryNumeric = Rcpp::as<Rcpp::NumericMatrix>(auxillaryNumeric_);
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input auxillaryNumeric must be a numeric matrix");
			}
			Rcpp::IntegerVector auxDim;
			try
			{
				auxDim = Rcpp::as<Rcpp::IntegerVector>(auxillaryNumeric.attr("dim"));
			}
			catch(...)
			{
				throw Rcpp::not_compatible("Input auxillaryNumeric had dimensions of wrong type");
			}
			if(auxDim.length() != 2)
			{
				throw Rcpp::not_compatible("Input auxillaryNumeric had dimensions of wrong length");
			}
			if(auxDim[1] != markerNames.size())
			{
				throw Rcpp::not_compatible("Input auxillaryNumeric had wrong number of columns");
			}
			auxRows = auxillaryNumeric.nrow();
			auxillaryPointer = &(auxillaryNumeric(0,0));
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
		mpMapInteractive::qtPlot::imputeFunctionType imputeFunction = (bool (*)(unsigned char* theta, std::vector<double>& thetaLevels, double* lod, double* lkhd, std::vector<int>& markers, std::string& error, std::function<void(unsigned long, unsigned long)> statusFunction))imputeFunctionUntyped;

		mpMapInteractive::qtPlot plot(&(thetaData(0)), thetaLevelsVector, groups, markerNames, auxillaryPointer, auxRows, imputedRawImageData, imputeFunction);
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
