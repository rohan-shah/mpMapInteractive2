context("Test mpMapInteractive2")
test_that("Check that non-contiguous groups result in an error",
	{
		f2Pedigree <- f2Pedigree(10)
		map <- sim.map(len = rep(100, 2), n.mar = 11, anchor.tel=TRUE, include.x=FALSE, eq.spacing=TRUE)
		cross <- simulateMPCross(map=map, pedigree=f2Pedigree, mapFunction = haldane)

		rf <- estimateRF(cross, keepLod = TRUE)
		grouped <- formGroups(rf, groups = 2)
		grouped <- mpMap2::subset(grouped, markers = c(22, 1:11, 12:21))

		#Have to turn the warning into an error, so the graphical bit doesn't actually show
		opt <- options()
		options(warn = 2)
		expect_that(mpMapInteractive2(grouped), throws_error("non-contiguous"))

		expect_that(.Call("mpMapInteractive2", grouped, NULL, PACKAGE="mpMapInteractive2"), throws_error("non-contiguous"))
		options(warn = opt$warn)
	})
