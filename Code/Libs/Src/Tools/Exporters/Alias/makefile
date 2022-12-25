#
#  Directory: Site/dso/GetUTF
#
#  This directory builds GetUTF.
#
#
#  To compile with debug, use the env var CPP_DEBUG:
#
#	setenv CPP_DEBUG -g
#
#
#  To customize the include path or compile flags,
#  use these macros located below in this file:
#
#	LC++INCS	to modify include path
#	LC++OPTS	to modify compile flags
#
#
#  This Makefile still has these following external references:
#
#	/remote/dolphin/swre3/Server/uimx_2.6/bin/uxcgen
#	../../include/
#



#  SITEROOT defines the "Site" directory of the development tree.
#  It is a relative path from this current directory. All other
#  variables in this Makefile should be relative to SITEROOT.
#
SITEROOT	= $(GAMES_LOCATION)
INCDIR		= -I$(SITEROOT)/include -I$(ALIAS_LOCATION)/ODS/Common/include -I$(ALIAS_LOCATION)/ODS/Games/include
DTOBJS      = $(GAMES_LOCATION)/lib/libDtOMio.so
#              $(GAMES_LOCATION)/lib/libDtIMF.so \
#              $(GAMES_LOCATION)/lib/libDtUt.so





#  Include SGI common definitions.
#
include	/usr/include/make/commondefs



#  Source files for this directory.
#
CFILES =	

C++FILES =  getutf.c++ rgbutils.c++ utfbulletinboard.c++ utfconverter.c++ \
            # utfbulletinboardui.c++ unimplemented.c++


#  User Interface files for this directory.
#
UIFILES =	

UISOURCES =	$(UIFILES:.if=.c)
UIOBJECTS =	$(UISOURCES:.c=.o)

OBJ1 = 		getutf.o rgbutils.o utfbulletinboard.o utfconverter.o \
            utfbulletinboardui.o unimplemented.o utf.o 3db.o cmp.o cmesh.o \
            cylinder.o extents.o minsphr.o matrix4.o eigen.o minbox3.o volint.o \
            misc.o mtl_txt.o sortq.o imagefilter.o

OBJ2 = 	

OBJ3 = 	

#  Files created by this Makefile which
#  need to be removed by clean & clobber.
#
#LDIRT =		Options.h \
#		VMapper.h \
#		$(UIFILES:.if=.c) \
#		so_locations \
#		*.rf

LDIRT =		so_locations \
		GetUTF


#  Local macros used on the compile line. Use the env var
#  CPP_DEBUG to turn on debug (setenv CPP_DEBUG -g).
#
LC++INCS	= -I. $(INCDIR)
LC++OPTS	= ${CPP_DEBUG} -fullwarn -D_SVR4_SOURCE -D_LONGLONG -DSGI -D_SGI_SOURCE -DMOTIF -DXT_CODE

LCINCS		= -I. $(INCDIR)
LCOPTS		= ${CPP_DEBUG} -fullwarn -D_SVR4_SOURCE -D_LONGLONG -DSGI -D_SGI_SOURCE -DMOTIF -DXT_CODE



#  Targets for this Makefile.
#
TARGETS	=	GetUTF

default:	$(TARGETS)



#  Include SGI common rules. This line must be after
#  the first rule in this Makefile. Add our own rules
#  after the SGI rules.
#
include $(COMMONRULES)

CPPFLAGS = -I. -I/usr/aw/alias/ODS/Games/include -I/usr/aw/alias/ODS/Common/include -I/usr/aw/alias/ODS/Games/include -fullwarn -D_SVR4_SOURCE -D_LONGLONG -DSGI -D_SGI_SOURCE -DMOTIF -DXT_CODE -nostdinc -I/usr/include/CC -I/usr/include -o32 -O2 -MDupdate Makedepend

#  Convert a C++ source file (.c++) to an object file (.o)
#
.c++.o:
	$(C++) -D_LONGLONG -DSGI -DALIAS $(CPPFLAGS) -c -o $@ $<
#	echo $(C++FLAGS)


#  Convert a C source file (.c) to an object file (.o)
#
.c.o:
	$(CC) -c $(CFLAGS) -Xcpluscomm -o $@ $<


#  Convert a UIM/X interface file (.if) to an object file (.o)
#
UXCGEN		= $(SITEROOT)/bin/uxcgen
UXCFLAGS	= -a -h -x

.SUFFIXES:	.if

#.if.c:
#	@echo "\nProcessing $*.if"
#	$(UXCGEN) $(UXCFLAGS) $*.if
#	@echo "\nPassing $*.h through ExternC.awk"
#	awk -f ExternC.awk $*.h > $*.h,new
#	mv $*.h,new $*.h




#  Link the GetUTF DSO.
#
GetUTF: $(OBJ1) $(UIOBJECTS) $(DTOBJS)
#	$(C++) -shared -o $@ $(OBJ1) $(UIOBJECTS)
	CC -o32 -shared -o $@ $(OBJ1) $(UIOBJECTS) $(DTOBJS) -L/lib -L/usr/lib -L/usr/lib/internal /usr/lib/libvkEZ.a /usr/lib/libvk.a -lSgm -lXm -lXt -lX11 -lgen



