##############################################################################
# File:    Makefile
# Author:  Daniel Luchaup
# Date:    29 June 2014
# Copyright 2007-2014 Daniel Luchaup luchaup@cs.wisc.edu
#
# This file contains unpublished confidential proprietary work of
# Daniel Luchaup, Department of Computer Sciences, University of
# Wisconsin--Madison.  No use of any sort, including execution, modification,
# copying, storage, distribution, or reverse engineering is permitted without
# the express written consent (for each kind of use) of Daniel Luchaup.
#
##############################################################################

MAKEFILENAME=$(lastword $(MAKEFILE_LIST))

where=$(shell uname -a | awk '{print $$2}' )

.PHONY:	all etags tags depend clean showtrex showrank new newtest mod

tests:
	@./scripts/fa_intersect.sh

clean:
		@rm -rf 	tmp

cleanall: clean
