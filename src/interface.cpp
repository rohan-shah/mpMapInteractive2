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
		Rcpp::Function markerNamesFunc("markers");
		std::vector<std::string> markerNames = Rcpp::as<std::vector<std::string> >(markerNamesFunc(mpcross));
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
		Rcpp::S4 theta;
		try
		{
			theta = Rcpp::as<Rcpp::S4>(rf.slot("theta"));
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@rf@theta must be a numeric matrix");
		}
		
		std::vector<int> groups;
		try
		{
			groups = Rcpp::as<std::vector<int> >(lg.slot("groups"));
		}
		catch(Rcpp::not_compatible&)
		{
			throw Rcpp::not_compatible("Input mpcross@lg@groups must be an integer vector");
		}
		
		Rcpp::RawVector thetaData;
		try
		{
			thetaData = theta.slot("data");
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@rf@theta@data must be a raw vector");
		}

		Rcpp::NumericVector thetaLevels;
		try
		{
			thetaLevels = theta.slot("levels");
		}
		catch(...)
		{
			throw Rcpp::not_compatible("Input mpcross@rf@theta@levels must be a numeric vector");
		}
		std::vector<double> thetaLevelsVector = Rcpp::as<std::vector<double> >(thetaLevels);

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
		//Show qt application
		int argc = 1;
		char* argv[3];
		argv[0] = new char[1];
		argv[1] = new char[1];
		argv[2] = new char[1];
		argv[0][0] = argv[1][0] = argv[2][0] = 0;
		QApplication app(argc, argv);
		mpMapInteractive::qtPlot plot(&(thetaData(0)), thetaLevelsVector, groups, markerNames, auxillaryPointer, auxRows);
		plot.show();
		app.exec();

		const mpMapInteractive::qtPlotData& outputData = plot.getData();
		std::vector<int> outputGroups = outputData.getCurrentGroups();
		std::vector<std::string> outputMarkerNames = outputData.getCurrentMarkerNames();

		delete[] argv[0];
		delete[] argv[1];
		delete[] argv[2];

		Rcpp::CharacterVector convertedOutputMarkerNames = Rcpp::wrap(outputMarkerNames);
		Rcpp::IntegerVector convertedOutputGroups = Rcpp::wrap(outputGroups);
		Rcpp::List retVal = Rcpp::List::create(convertedOutputMarkerNames, convertedOutputGroups);
		return retVal;
	END_RCPP
	}
}
