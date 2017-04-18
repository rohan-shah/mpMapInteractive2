mpMapInteractive2 <- function(mpcross)
{
	if(!isS4(mpcross))
	{
		stop("Input mpcross must be an S4 object")
	}
	if(!inherits(mpcross, "mpcrossLG"))
	{
		stop("Input mpcross must inherit from class \"mpcrossLG\"")
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
	result <- .Call("mpMapInteractive2", mpcross, PACKAGE="mpMapInteractive2")
	markerNames <- result[[1]]
	groups <- result[[2]]
	names(groups) <- markerNames
	withoutLG <- as(mpcross, "mpcrossRF")
	subsetted <- subset(withoutLG, markers = markerNames)
	newObject <- new("mpcrossLG", subsetted, rf = subsetted@rf, lg = new("lg", groups = groups, allGroups = unique(groups)))
	return(list(object = newObject, permutations = result[[3]], groups = result[[4]]))
}
