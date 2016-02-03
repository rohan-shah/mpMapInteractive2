mpMapInteractive2 <- function(mpcross, auxillaryNumeric = NULL)
{
	if(!isS4(mpcross))
	{
		stop("Input mpcross must be an S4 object")
	}
	if(!inherits(mpcross, "mpcrossLG"))
	{
		stop("Input mpcross must inherit from class \"mpcrossLG\"")
	}
	if(!is.null(auxillaryNumeric))
	{
		if(storage.mode(auxillaryNumeric) == "integer")
		{
			storage.mode(auxillaryNumeric) <- "numeric"
		}
		if(storage.mode(auxillaryNumeric) != "double")
		{
			stop("Input auxillaryNumeric must be a numeric matrix")
		}
		if(ncol(auxillaryNumeric) != nFounders(mpcross))
		{
			stop("Input auxillaryNumeric had the wrong number of columns")
		}
		markers <- colnames(auxillaryNumeric)
		if(length(markers) != nMarkers(mpcross) || !(all(markers %in% markers(mpcrosss))))
		{
			stop("Column names of auxillaryNumeric did not match up with marker names of mpcross")
		}
	}
	contiguous <- TRUE
	for(group in mpcross@lg@allGroups)
	{
		groupMarkers <- which(mpcross@lg@groups == group)
		if(any(diff(sort(groupMarkers)) != 1))
		{
			contiguous <- FALSE
			break
		}
	}
	if(!contiguous)
	{
		warning("Markers for at least one linkage group were non-contiguous. Markers will be re-ordered so that linkage groups are in contiguous chunks")
		mpcross <- subset(mpcross, markers = order(mpcross@lg@groups))
	}
	result <- .Call("mpMapInteractive2", mpcross, auxillaryNumeric, PACKAGE="mpMapInteractive2")
	markerNames <- result[[1]]
	groups <- result[[2]]
	names(groups) <- markerNames
	withoutLG <- as(mpcross, "mpcrossRF")
	subsetted <- subset(withoutLG, markers = markerNames)
	return(new("mpcrossLG", subsetted, rf = subsetted@rf, lg = new("lg", groups = groups, allGroups = unique(groups))))
}
