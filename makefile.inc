#
# Portos v1.7.0
# Copyright (c) 2003-2014 by Rabih Chrabieh. All rights reserved.
#

# Preprocess Portos directives and compile
$(DIR_OBJ)/%.o: %.c
	@ echo Preprocessing $*.c
	@ mkdir -p $(DIR_OBJ)
	$(CC) $(CFLAGS) -E $*.c -o $(DIR_OBJ)/$*.e.c
	$(DIR_TOP)/bin/po_preprocess $(DIR_OBJ)/$*.e.c $(DIR_OBJ)/$*.c.c
	@ echo Compiling $*.c
	$(CC) $(CFLAGS) -c $(DIR_OBJ)/$*.c.c -o $@

$(DIR_OBJ)/%.exe: $(OBJS) $(PORTOS_LIB)
	$(CC) $(LDFLAGS) $(OBJS) $(PORTOS_LIB) -o $(DIR_OBJ)/$*.exe
	#objdump -Sdl $(DIR_OBJ)/$*.exe > $(DIR_OBJ)/$*.lst

# Remove CRs
fixcr:
	dos2unix $(ALLFILES)

# Change prefixes and names
replace:
	$(DIR_TOOLS)/replace_strings $(DIR_TOOLS)/cmd $(ALLFILES)

# Clean object files
clean:
	rm -rf obj *~ */*~ *.stackdump makefile.err *.dep

# Rule for building dependency lists, and writing them to a file
# named "makefile.dep". A warning is printed by makedepend which
# we don't care about. We don't care for the dependencies on standard
# includes such as stdio.h, stddef.h, etc.
depend:
	makedepend -f- -Y -- $(CFLAGS) -- $(SRCS) -p$(DIR_OBJ)/ > makefile_$(TARGET)_$(VERSION).dep 2>makefile.err

makefile_$(TARGET)_$(VERSION).dep: depend

# Insert dependency list.
include makefile_$(TARGET)_$(VERSION).dep
