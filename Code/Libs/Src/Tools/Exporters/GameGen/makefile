#!smake
#
# system definitions from /usr/include/make
#

#include <commondefs>
#include <commonrules>

API_LEVEL = API_LEV2

# -fullwarn //generates remarks
# CFLAGS=-mips3 -g3 -O -experimental
CFLAGS= -fullwarn -O2 -o32
DFLAGS=-D$(API_LEVEL)
LDFLAGS= 

#LIBLM = /usr/lib/lmgr.a
LIBLM=

LIBDIR = /usr/local/MultiGen/lib
INCDIR = /usr/local/MultiGen/API/inc

#LIB = -lmalloc -lds -lgutil -lc -lm -lexc -lmld 
LIB = -lds -lgutil -lc -lm -lexc -lmld 
FLTDATALIB = ${LIBDIR}/libfltdata.so
EASDATALIB = ${LIBDIR}/libeasdata.so
DDDATALIB = ${LIBDIR}/libdd.so
APILIB = ${LIBDIR}/libmgapi.so
IMGLIB = ${LIBDIR}/libimgfmt.so

TARGETS = \
	egcolor1  \
	egcolor2  \
	egdof1	\
	egdof2	\
	eggetattr1  \
	egimageio1	\
	egio1	\
	eglight1  \
	egmaterial1  \
	egmaterial2  \
	egnew1  \
	egsetattr1  \
	egstruct1	\
	egstruct2	\
	egswitch1	\
	egtexture1  \
	egtexture2  \
	egtexture3  \
	egtexture4  \
	egtexture5  \
	egtexture6  \
	egwalk1  \
	egxform1  \
	egxref1
	

.c.o:
	$(CC) -o $@ -I. -I$(INCDIR) \
	$(CFLAGS) $(DFLAGS) -c $<

.cpp.o:
	$(C++) -o $@ -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c $<

.c++.o:
	$(C++) -o $@ -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c $<

utf.o:	utf.c++ typedefs.h sgi_utf.h persistcompound.h cmp.h 3db.h
	$(C++) -o utf.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c utf.c++

3db.o:	3db.c++ 3db.h vector.h matrix.h persistmath.h extents.h minsphr.h faceprop.h rgbutils.h persist*.h vector4.h matrix4.h eigen.h minbox3.h imagefilter.h volint.h misc.h mtl_txt.h names.h
	$(C++) -o 3db.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c 3db.c++

matrix4.o:	matrix4.c++ matrix4.h vector4.h
	$(C++) -o matrix4.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c matrix4.c++

cmp.o:	cmp.c++ typedefs.h sgi_utf.h persistcompound.h cmp.h 3db.h persist*.h
	$(C++) -o cmp.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c cmp.c++

rgbutils.o:	rgbutils.c++ rgbutils.h
	$(C++) -o rgbutils.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c rgbutils.c++

cmesh.o:	cmesh.c++ cmesh.h
	$(C++) -o cmesh.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c cmesh.c++

cylinder.o:	cylinder.c++ cylinder.h
	$(C++) -o cylinder.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c cylinder.c++

extents.o:	extents.c++ extents.h
	$(C++) -o extents.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c extents.c++

minsphr.o:	minsphr.c++ minsphr.h
	$(C++) -o minsphr.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c minsphr.c++

volint.o:	volint.c++ volint.h
	$(C++) -o volint.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c volint.c++

eigen.o:	eigen.c++ eigen.h
	$(C++) -o eigen.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c eigen.c++

minbox3.o:	minbox3.c++ minbox3.h
	$(C++) -o minbox3.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c minbox3.c++

misc.o:	misc.c++ misc.h
	$(C++) -o misc.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c misc.c++

mtl_txt.o:	mtl_txt.c++ mtl_txt.h
	$(C++) -o mtl_txt.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c mtl_txt.c++

sortq.o:	sortq.c++
	$(C++) -o sortq.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c sortq.c++

tga_unix.o:	tga_unix.c++
	$(C++) -o tga_unix.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -DTGA_2_TXM -c tga_unix.c++

imagefilter.o:	imagefilter.c++ imagefilter.h
	$(C++) -o imagefilter.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c imagefilter.c++

flt23db.o:	flt23db.c++ 3db.c++ imagefilter.c++ mtl_txt.c++ 3db.h vector.h matrix.h persistmath.h extents.h minsphr.h faceprop.h rgbutils.h cmp.h cmesh.h cylinder.h persist*.h matrix4.h eigen.h minbox3.h imagefilter.h volint.h misc.h mtl_txt.h names.h 
	$(C++) -o flt23db.o -I. -I$(INCDIR) \
	$(CFLAGS) -DAPI_LEV2 -DLONGLONG -DSGI -DGAMEGEN -c flt23db.c++

flt23db: flt23db.o 3db.o cmp.o utf.o rgbutils.o cmesh.o cylinder.o extents.o minsphr.o \
	 matrix4.o eigen.o minbox3.o volint.o misc.o mtl_txt.o sortq.o imagefilter.o
	$(C++) -o32 -woff 85 flt23db.o 3db.o cmp.o utf.o rgbutils.o cmesh.o \
        cylinder.o extents.o minsphr.o matrix4.o eigen.o minbox3.o volint.o \
        misc.o mtl_txt.o sortq.o imagefilter.o \
        $(APILIB) $(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@
#-lmalloc -lds -lgutil -lc -lm -lexc -lmld $(FLTDATALIB)

animlib: animlib.o utf.o 3db.o rgbutils.o
	$(C++) -o32 -woff 85 animlib.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

tga2txm: tga2txm.o xvtarga.o 3db.o rgbutils.o utf.o
	$(C++) -o32 -woff 85 tga2txm.o xvtarga.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

pix2txm: pix2txm.o 3db.o rgbutils.o utf.o
	$(C++) -o32 -woff 85 pix2txm.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

txmlib: txmlib.o utf.o 3db.o rgbutils.o tga_unix.o bmp.o cmesh.o cylinder.o \
	extents.o minsphr.o matrix4.o eigen.o minbox3.o volint.o misc.o mtl_txt.o \
	sortq.o imagefilter.o
	$(C++) -o32 -woff 85 txmlib.o tga_unix.o bmp.o 3db.o rgbutils.o utf.o \
        cmesh.o cylinder.o extents.o minsphr.o matrix4.o eigen.o minbox3.o volint.o \
        misc.o mtl_txt.o sortq.o imagefilter.o \
	-lmalloc -lm $(LDFLAGS) -o $@

addnorm: addnorm.o utf.o 3db.o rgbutils.o
	$(C++) -o32 -woff 85 addnorm.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

touchutf: touchutf.o utf.o 3db.o rgbutils.o
	$(C++) -o32 -woff 85 touchutf.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

update3db: update3db.o utf.o 3db.o rgbutils.o
	$(C++) -o32 -woff 85 update3db.o 3db.o rgbutils.o utf.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

fixmtl: fixmtl.o utf.o 3db.o cmp.o rgbutils.o cmesh.o cylinder.o
	$(C++) -o32 -woff 85 fixmtl.o 3db.o cmp.o rgbutils.o utf.o cmesh.o \
        cylinder.o -lmalloc -lm $(LDFLAGS) -o $@
	#$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

lod: lod.o utf.o 3db.o rgbutils.o
	$(C++) -o32 -woff 85 lod.o 3db.o rgbutils.o utf.o \
	$(FLTDATALIB) $(DDDATALIB) -lglut -lGLU -lGL -lXmu  -lm -lXi -lX11 -lXext -lmalloc -lm $(LDFLAGS) -o $@

readcmp: readcmp.o utf.o cmp.o 3db.o rgbutils.o cmesh.o cylinder.o extents.o minsphr.o misc.o
	$(C++) -o32 -woff 85 readcmp.o cmp.o 3db.o rgbutils.o utf.o cmesh.o \
        cylinder.o extents.o minsphr.o misc.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

readutf: readutf.o utf.o misc.o
	$(C++) -o32 -woff 85 readutf.o utf.o misc.o $(APILIB) \
	$(FLTDATALIB) $(IMGLIB) $(DDDATALIB) -lmalloc -lm $(LDFLAGS) -o $@

all: $(TARGETS)

$(TARGETS): ${.TARGET}.o
	$(C++) ${.TARGET}.o $(APILIB) $(FLTDATALIB) $(IMGLIB) $(DDDATALIB) \
	 $(LIB) $(LDFLAGS) -o ${.TARGET}

$(ETARGETS): ${.TARGET}.o
	$(C++) ${.TARGET}.o $(APILIB) $(EASDATALIB) $(FLTDATALIB) $(IMGLIB) $(DDDATALIB) \
	 $(LIB) $(LDFLAGS) -o ${.TARGET}

