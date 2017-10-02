mpMapInteractive2 <- function(mpcross, permutations = NULL, groups = NULL, auxiliaryData = NULL)
{
	if(!isS4(mpcross))
	{
		stop("Input mpcross must be an S4 object")
	}
	if(!inherits(mpcross, "mpcrossLG"))
	{
		stop("Input mpcross must inherit from class \"mpcrossLG\"")
	}
	nMarkers <- nMarkers(mpcross)
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
	if(!is.null(permutations))
	{
		lapply(permutations, function(x)
		{
			r <- range(x)
			if(r[1] < 0 || r[2] >= nMarkers) stop("Input permutations contained invalid values")
		})
	}
	if(!is.null(auxiliaryData))
	{
		if(!is.numeric(auxiliaryData))
		{
			stop("Input auxiliaryData must be NULL or a numeric matrix")
		}
		if(nrow(auxiliaryData) != nMarkers(mpcross) || ncol(auxiliaryData) != nMarkers(mpcross))
		{
			stop("Input auxiliaryData must have number of rows and columns equal to the number of markers")
		}
		if(any(rownames(auxiliaryData) != markers(mpcross)) || any(colnames(auxiliaryData) != markers(mpcross)))
		{
			stop("Input auxiliaryData must have marker names as its row and column names")
		}
	}
	if(!contiguous)
	{
		warning("Markers for at least one linkage group were non-contiguous. Markers will be re-ordered so that linkage groups are in contiguous chunks")
		mpcross <- subset(mpcross, markers = order(mpcross@lg@groups))
	}
	result <- .Call("mpMapInteractive2", mpcross, permutations, groups, auxiliaryData, PACKAGE="mpMapInteractive2")
	markerNames <- result[[1]]
	groups <- result[[2]]
	names(groups) <- markerNames
	withoutLG <- as(mpcross, "mpcrossRF")
	subsetted <- subset(withoutLG, markers = markerNames)
	newObject <- new("mpcrossLG", subsetted, rf = subsetted@rf, lg = new("lg", groups = groups, allGroups = unique(groups)))
	return(list(object = newObject, permutations = result[[3]], groups = result[[4]]))
}
