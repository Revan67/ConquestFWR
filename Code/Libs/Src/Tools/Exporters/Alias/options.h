#ifndef __OPTIONS__UTF__BOARD
#define __OPTIONS__UTF__BOARD

//#ifdef DEFINE_UTF
//#define DEFVAR  extern
//#define DEFINIT(x) = x
//#else
//#define DEFVAR /**/
//#define DEFINIT(x) /**/
//#endif

#define DEFVAR  extern
#define DEFINIT(x) /**/

DEFVAR int              utf_output_materials  DEFINIT( TRUE );
DEFVAR int              utf_no_mipmaps  DEFINIT( FALSE );
DEFVAR int              utf_convex_hull  DEFINIT( FALSE );
DEFVAR int              utf_remove_constant_channels  DEFINIT( FALSE );

DEFVAR int              utf_mesh  DEFINIT( FALSE );
//DEFVAR int              utf_skeleton  DEFINIT( FALSE );
DEFVAR int              utf_animation  DEFINIT( FALSE );
DEFVAR int              utf_combined  DEFINIT( FALSE );

DEFVAR int              utf_old_format  DEFINIT( FALSE );
DEFVAR int              utf_output_textures   DEFINIT( TRUE );
DEFVAR int              utf_output_hierarchy  DEFINIT( FALSE );
DEFVAR int              utf_output_animation  DEFINIT( FALSE );

DEFVAR int              utf_output_vert_norms DEFINIT( FALSE );
DEFVAR int              utf_output_vert_colors DEFINIT( FALSE );
DEFVAR int              utf_output_poly_norms DEFINIT( FALSE );
DEFVAR int              utf_show_index_counters DEFINIT( FALSE );
DEFVAR int              utf_output_pivots     DEFINIT( FALSE );
DEFVAR int              utf_output_transforms DEFINIT(  FALSE );
DEFVAR int              utf_output_tex_coords DEFINIT(  FALSE );

#endif
