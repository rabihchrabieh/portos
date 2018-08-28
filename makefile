#
# Portos v1.7.0
# Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
#

# Directories in the order they should be made
DIRS = tools src include test demo/modem demo/simple demo/timer demo/benchmark demo/dma doc htmlWork

# Start macro to execute commands in each directory define in $(DIR)
DIR_START = @ for d in $(DIRS) ; do (if test -d $$d; then cd $$d;

# End macro to execute commands in each directory define in $(DIR)
DIR_END = ; fi); done

all:
	mkdir -p lib
	$(DIR_START) make all $(DIR_END)

# Remove CRs
fixcr:
	$(DIR_START) make fixcr $(DIR_END)
	dos2unix *.txt makefile*

# Change prefixes and names
replace:
	$(DIR_START) make replace $(DIR_END)
	tools/replace_strings tools/cmd *.txt makefile*

clean:
	$(DIR_START) make clean $(DIR_END)
	rm -f *~
