/*
 * Copyright (c) 1997 Massachusetts Institute of Technology
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to use, copy, modify, and distribute the Software without
 * restriction, provided the Software, including any modified copies made
 * under this license, is not distributed for a fee, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE MASSACHUSETTS INSTITUTE OF TECHNOLOGY BE LIABLE
 * FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 * Except as contained in this notice, the name of the Massachusetts
 * Institute of Technology shall not be used in advertising or otherwise
 * to promote the sale, use or other dealings in this Software without
 * prior written authorization from the Massachusetts Institute of
 * Technology.
 *  
 */

/* fftw.h -- system-wide definitions */
/* $Id: fftw.h,v 1.13 1997/03/19 22:59:46 fftw Exp $ */

#include <stdlib.h>

/* our real numbers */
typedef double REAL;

/*********************************************
 * Complex numbers and operations 
 *********************************************/
typedef struct {
     REAL re, im;
} COMPLEX;

#define c_re(c)  ((c).re)
#define c_im(c)  ((c).im)

typedef enum { FFTW_FORWARD = -1, FFTW_BACKWARD = 1 } fftw_direction;

/*********************************************
 *              Basic blocks 
 *********************************************/
/*
 * There are three kind of basic blocks:
 *
 * BASE blocks compute the FFT of a certain size
 *
 * BFLY blocks combine smaller FFT's to solve a bigger
 *      problem.  They do the same computations as the
 *      base blocks, except that 1) the input is not
 *      necessarily in consecutive array positions, and 2)
 *      they take care of twiddle factors
 * 
 */

typedef void (base_block) (const COMPLEX *, COMPLEX *, int, int);
typedef void (bfly_block) (COMPLEX *, int, const COMPLEX *, 
			   int, int, int);
typedef void (generic_bfly) (int, int, COMPLEX *, const COMPLEX *,
			     int, int, int, int, int);

/*********************************************
 *     Configurations
 *********************************************/
/*
 * A configuration is a database of all known basic
 * blocks
 */

typedef struct config_base {
     int size;			/* size of the problem */
     base_block *solver;	/* 
				 * pointer to the block that solves the 
				 * problem 
				 */
} config_base;

extern config_base fftw_config_base[];
extern config_base fftwi_config_base[];
extern char *fftw_version;

/*
 * bfly blocks combine a problem of size n into r problems of size m =
 * n/r.  The `size' of the block is understood to be `r'.  
 */

typedef struct config_bfly {
     int size;			/* size of the butterfly */
     bfly_block *conquerer;
} config_bfly;

extern config_bfly fftw_config_bfly[];
extern config_bfly fftwi_config_bfly[];

/*****************************
 *        Plans
 *****************************/
/*
 * A plan is a sequence of reductions to compute a FFT of
 * a given size.  At each step, the FFT algorithm will
 * either
 *
 * 1) solve a base problem, or
 * 2) recurse
 */

enum instr {
     FFTW_UNKNOWN, FFTW_SOLVE, FFTW_RECURSE, FFTW_GENERIC
};

/* structure that contains twiddle factors */
typedef struct fftw_twiddle {
     int n;
     COMPLEX *twarray;
     struct fftw_twiddle *next;
     int refcnt;
} fftw_twiddle;
     
/* structure that holds all the data needed for a given step */
typedef struct fftw_instr {
     enum instr action;
     int size;
     int length;
     fftw_twiddle *tw;
     bfly_block *conquerer;
     base_block *solver;
     generic_bfly *gen_conquerer;
     double cost;
     int flags;

     /* linkage information */
     int n;
     struct fftw_instr *next; 
} fftw_instr;

/* a plan is just an array of instructions */
typedef fftw_instr *fftw_plan;

/* flags for the planner */
#define  FFTW_ESTIMATE (0)
#define  FFTW_MEASURE  (1)

/* these two features are intentionally undocumented */
#define  FFTW_OPTIMAL  (2)
#define  FFTW_OPTIMAL_ENABLE_HACKS  (4)

#define  FFTW_IN_PLACE (256)
#define  FFTW_OUT_OF_PLACE (0)

#define FFTW_MAXPLANLEN 64 /* enough for up to 2^64 */
extern fftw_plan fftw_create_plan(int n, fftw_direction dir, int flags);
extern fftw_twiddle *fftw_create_twiddle(int n);
extern void fftw_destroy_twiddle(fftw_twiddle *tw);
extern void fftw_print_plan(fftw_plan plan);
extern void fftw_destroy_plan(fftw_plan plan);
extern void fftw_naive(int n, COMPLEX *in, COMPLEX *out);
extern void fftwi_naive(int n, COMPLEX *in, COMPLEX *out);
void fftw(fftw_instr *plan, int howmany, COMPLEX *in, int istride, 
	  int idist, COMPLEX *out, int ostride, int odist);
extern void fftw_init(void);
extern double fftw_measure_runtime(fftw_plan plan, int n);
extern void fftw_die(char *s);
extern generic_bfly fftw_bfly_generic;
extern generic_bfly fftwi_bfly_generic;
extern void *fftw_malloc(size_t n);

/*****************************
 *    N-dimensional code
 *****************************/
typedef struct {
     int is_in_place;		/* 1 if for in-place FFT's, 0 otherwise */
     int rank;			/* 
				 * the rank (number of dimensions) of the
				 * array to be FFT'ed
				 */
     int *n;			/*
				 * the dimensions of the array to the
				 * FFT'ed 
				 */
     int *n_before;		/*
				 * n_before[i] = product of n[j] for j < i 
				 */
     int *n_after;		/* n_after[i] = product of n[j] for j > i */
     fftw_plan *plans;		/* fftw plans for each dimension */
     COMPLEX *work;		/* 
				 * work array for FFT when doing
				 * "in-place" FFT 
				 */
} fftwnd_aux_data;

typedef fftwnd_aux_data *fftwnd_plan;


/* Initializing the FFTWND Auxiliary Data */
fftwnd_plan fftw2d_create_plan(int nx, int ny, fftw_direction dir, int flags);
fftwnd_plan fftw3d_create_plan(int nx, int ny, int nz, 
			       fftw_direction dir, int flags);
fftwnd_plan fftwnd_create_plan(int rank, int *n, fftw_direction dir,
			       int flags);

/* Freeing the FFTWND Auxiliary Data */
void fftwnd_destroy_plan(fftwnd_plan plan);

/* Computing the N-Dimensional FFT */
void fftwnd(fftwnd_plan plan, int howmany,
	    COMPLEX *in, int istride, int idist,
	    COMPLEX *out, int ostride, int odist);

/*************************************
 *  Timers, etc
 *************************************/
/* 
 * Here you can use all the nice timers available
 * in your machine
 */

#ifdef SOLARIS

/* we use the nanosecond virtual timer */
#include <sys/time.h>

typedef hrtime_t fftw_time;

#define fftw_get_time() gethrtime()
#define fftw_time_to_sec(t) ((double) (t) / 1.0e9)

/* the clock should be precise enough for this number of iterations */
#define FFTW_PLAN_ITER 10000

#endif

#if defined(MAC) || defined(macintosh)
double get_Mac_microseconds();

typedef double fftw_time;

#define fftw_get_time() get_Mac_microseconds()
#define fftw_time_to_sec(t) (t)

/* the clock should be precise enough for this number of iterations */
#define FFTW_PLAN_ITER 100000
#endif

/*
 * last resort: good old Unix clock()
 */
#ifndef fftw_get_time
#include <time.h>

typedef clock_t fftw_time;

#ifndef CLOCKS_PER_SEC
#ifdef sun 
/* stupid sunos4 prototypes */
#define CLOCKS_PER_SEC 1000000
extern long clock(void);
#else
#error Please define CLOCKS_PER_SEC
#endif
#endif

#define fftw_get_time() clock()
#define fftw_time_to_sec(t) ((double) (t) / CLOCKS_PER_SEC)


#define FFTW_PLAN_ITER 1000000
#endif
