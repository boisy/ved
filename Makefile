# Makefile for curses version of ved editor
# Created Feb 3, 1998
# Bob van der Poel

# Change the rule to compile a source file. All we've
# done is to add the $(OBJS) so than comilied files don't
# clutter our source tree. If not using GNU Make you'll
# have to delete the '-o $(OBJS)/$@. 

%.o : %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $(OBJS)/$@

#
# To enable assertions, uncomment the assert macro.
#

GPROF		=# -pg
DEBUG		= -g -DDEBUG
CC		= cc
ODIR		= .
OFILE		= ved

# If not using GNU Make or if you want the .o files in the same
# dir as your sources, use the following line:

#OBJS		= .

# This puts the .o files in their own dir. Nice and tidy (but needs GNU Make).!

OBJS		= objs

# Add the following to DEFINES if needed:
#	-DNEED_ASPRINTF	- compilies internal version of asprintf()
#	-DNEED_STRSEP	- compilies internal version of strsep() 

DEFINES		=  

CFLAGS		=  $(DEBUG) $(GPROF) -Wstrict-prototypes -Wmissing-prototypes \
			-Wno-missing-declarations  -Wall  -Winline -Wshadow -Wno-int-conversion \
			-Wno-implicit-function-declaration $(DEBUG) \
			-Wno-strict-prototypes -Wno-return-type -Wno-missing-prototypes \
			-Wno-format -Wno-pointer-sign -Wno-deprecated-non-prototype \
			-Wno-unused-but-set-variable -Wno-unsequenced \
			$(DEFINES)

# If using oldstyle curses, use the next line
#LIB		= -lcurses

# This is for ncurses. You might need to delete -lgpm???

LIB		=  -lncurses 
LFLAGS		=  


# this tells make to look in the objects directory
# for complied files. Note we need to do this after
# $(OBJS) is defined.

vpath %.o $(OBJS)

FILES=	attrs.o \
	bindings.o block.o buffer.o \
	case.o clipboard.o cmds.o cursor.o \
	death.o delete.o dialog.o dir.o display.o	\
	edit.o editmenu.o error.o expr.o \
	fallback.o file.o filemode.o file_utils.o find.o find_pattern.o \
	help.o history.o  \
	is.o \
	info.o \
	jump.o \
	keyboard.o \
	line_copy.o \
	main.o  menu.o misc.o mouse.o move.o   \
	options.o \
	quit.o \
	rc.o replace.o \
	scroll_ind.o shell.o  sigwinch.o spell.o status.o \
	tilde.o \
	version.o \
	windows.o wm.o wrap.o


######################################################
# This block actually compiles the programs.


$(OFILE): $(FILES)
	cd $(OBJS); $(CC) $(FILES) $(CFLAGS) $(LFLAGS) $(LIB) -o ../$(OFILE) $(DEBUG)


static: $(FILES)
	cd $(OBJS); $(CC) $(FILES) $(CFLAGS) $(LFLAGS) $(LIB) -o ../$(OFILE) $(DEBUG) -static
	
spell_maint: spell_maint.c
	$(CC) spell_maint.c -o spell_maint -O3

# This is a special compile line for filemode.c. Not really necessary
# except that the compile line for the other modules reports some
# errors. 

filemode.o:
	gcc -O2 -c -o $(OBJS)/$@ $*.c 
	
# The sed command converts .o to .c. Hence, we run makedepend
# on all the source files.

depend:
	@echo "makedepend issues warning about unknown directives and"
	@echo "that it is only tested with gcc 2.7.2.x. It is safe to ignore"
	@echo -ee "these warnings....\\n"
	@makedepend -- $(CFLAGS) -- -D__i386__  `echo $(FILES) | sed /\\\\.o'/s//.c'/g ` 


#######################################################
# We keep prototypes for all the global functions
# in proto.h. To create proto.h we use cproto.
# We could just do a cproto *.c -o file.h but
# that is slow since all the include files are
# parsed for each .c file. We speed things up
# a lot by concating all the files into a biggie
# and then running cproto on that. cproto is smart
# enuf NOT to include a file for a 2nd time.
# The sed command converts .o to .c

proto:
	ctemp=.cprotofiles.c ;\
	proto=proto.h ;\
	cat `echo $(FILES) | sed /\\\\.o'/s//.c'/g` >$$ctemp ;\
	cat /dev/null >$$proto ;\
	cproto -E 0 -f2 -o $$proto $$ctemp ;\
	#rm  $$ctemp 

	
clean:
	rm -f core gmon.out  $(OBJS)/*

# Create a tar file with the current state of the source and docs,
# for distribution.

tar:
	SUB=ved-`cat version.h | tr -d \"` ;\
	mkdir $$SUB ; \
	for a in  *.c *.h .indent.pro *.data make* vedrc *.txt [A-Z]* ; \
		do ln  $$a $$SUB/$$a ; done ; \
	mkdir $$SUB/objs ; \
	mkdir $$SUB/docs ; \
	mkdir $$SUB/docs/html ; \
	for  a in  docs/faq.ps docs/faq.html docs/ved.ps docs/html/*.html \
		docs/html/*.gif docs/html/*.css ;\
		do ln  $$a $$SUB/$$a ; done ;\
	tar czf $$SUB.tar.gz $$SUB ; \
	rm $$SUB -rf


# My backup disk

disk:
	read -p "Ready disk in drive 0." ;\
	tar czf /dev/fd0 [A-Z]* *.c *.h .indent.pro *.data make* vedrc *.txt \
		docs/[A-Z]*  docs/*tex  ;\
	echo "Standby...flushing buffers" ;\
	sync ;\
	echo "Backup done"

man:
	@makeman

all:
	-mkdir objs
	-mkdir docs
	make clean
	make depend
	make proto
	make $(OFILE)
	make man
	cd docs;make all


cmdtable.h: cmdtable.data
	makecmds
	make proto


# DO NOT DELETE

attrs.o: ved.h cmdtable.h proto.h attrs.h
bindings.o: ved.h cmdtable.h proto.h attrs.h
block.o: ved.h cmdtable.h proto.h
buffer.o: ved.h cmdtable.h proto.h attrs.h
case.o: ved.h cmdtable.h proto.h
clipboard.o: ved.h cmdtable.h proto.h
cmds.o: ved.h cmdtable.h proto.h
cursor.o: ved.h cmdtable.h proto.h attrs.h
death.o: ved.h cmdtable.h proto.h
delete.o: ved.h cmdtable.h proto.h
dialog.o: ved.h cmdtable.h proto.h attrs.h
dir.o: ved.h cmdtable.h proto.h attrs.h
display.o: ved.h cmdtable.h proto.h attrs.h
edit.o: ved.h cmdtable.h proto.h attrs.h
editmenu.o: ved.h cmdtable.h proto.h attrs.h
error.o: ved.h cmdtable.h proto.h version.h
expr.o: ved.h cmdtable.h proto.h
fallback.o: ved.h cmdtable.h proto.h
file.o: ved.h cmdtable.h proto.h attrs.h
file_utils.o: ved.h cmdtable.h proto.h
find.o: ved.h cmdtable.h proto.h
find_pattern.o: ved.h cmdtable.h proto.h
help.o: ved.h cmdtable.h proto.h attrs.h
history.o: ved.h cmdtable.h proto.h
is.o: ved.h cmdtable.h proto.h
info.o: ved.h cmdtable.h proto.h
jump.o: ved.h cmdtable.h proto.h
keyboard.o: ved.h cmdtable.h proto.h attrs.h
line_copy.o: ved.h cmdtable.h proto.h
main.o: ved.h cmdtable.h proto.h main.h attrs.h
menu.o: ved.h cmdtable.h proto.h attrs.h
misc.o: ved.h cmdtable.h proto.h
mouse.o: ved.h cmdtable.h proto.h
move.o: ved.h cmdtable.h proto.h attrs.h
options.o: ved.h cmdtable.h proto.h attrs.h
quit.o: ved.h cmdtable.h proto.h
rc.o: ved.h cmdtable.h proto.h
replace.o: ved.h cmdtable.h proto.h
scroll_ind.o: ved.h cmdtable.h proto.h
shell.o: ved.h cmdtable.h proto.h
sigwinch.o: ved.h cmdtable.h proto.h
spell.o: ved.h cmdtable.h proto.h spell.h attrs.h
status.o: ved.h cmdtable.h proto.h
tilde.o: ved.h cmdtable.h proto.h
version.o: ved.h cmdtable.h proto.h version.h
windows.o: ved.h cmdtable.h proto.h attrs.h
wm.o: ved.h cmdtable.h proto.h
wrap.o: ved.h cmdtable.h proto.h attrs.h
