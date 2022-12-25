
/////////////////////////////////////////////////////////////
//
// Source file for UTFBulletinBoardUI
//
//    This class implements the user interface created in 
//    RapidApp.
//
//    Restrict changes to those sections between
//    the "//--- Start/End editable code block" markers
//
//    This will allow RapidApp to integrate changes more easily
//
//    This class is a ViewKit user interface "component".
//    For more information on how components are used, see the
//    "ViewKit Programmers' Manual", and the RapidApp
//    User's Guide.
//
//
/////////////////////////////////////////////////////////////


#include "utfbulletinboardui.h" // Generated header file for this class

#include <Xm/BulletinB.h> 
#include <Xm/Label.h> 
#include <Xm/ToggleB.h> 
#include <Xm/TextF.h> 
#include <Vk/VkResource.h>
//---- Start editable code block: headers and declarations


//---- End editable code block: headers and declarations


// These are default resources for widgets in objects of this class
// All resources will be prepended by *<name> at instantiation,
// where <name> is the name of the specific instance, as well as the
// name of the baseWidget. These are only defaults, and may be overriden
// in a resource file by providing a more specific resource name

String  UTFBulletinBoardUI::_defaultUTFBulletinBoardUIResources[] = {
//        "*animation.labelString:  Output Animation",
//        "*indexCounters.labelString:  Output Index Counters",
        "*label.labelString:  3DB Export options",
        "*label1.labelString:  Deformable mesh options",
//        "*label2.labelString:  Hierarchy options",
//        "*label3.labelString:  Animation options",
//        "*outputMaterials.labelString:  Output Materials",
//        "*outputHierarchy.labelString:  Output Hierarchy",
//        "*outputTransformations.labelString:  Output Transformation",
//        "*polygonNormals.labelString:  Output Polygon Normals",
        "*outputTextures.labelString:  Output Textures",
//        "*vertexColors.labelString:  Output Vertex Colors",
//        "*vertexNormals.labelString:  Output Vertex Normals",

        //---- Start editable code block: UTFBulletinBoardUI Default Resources


        //---- End editable code block: UTFBulletinBoardUI Default Resources

        (char*)NULL
};

UTFBulletinBoardUI::UTFBulletinBoardUI ( const char *name ) : VkComponent ( name ) 
{ 
    // No widgets are created by this constructor.
    // If an application creates a component using this constructor,
    // It must explictly call create at a later time.
    // This is mostly useful when adding pre-widget creation
    // code to a derived class constructor.

    //---- Start editable code block: UTFBulletinBoard constructor 2


    //---- End editable code block: UTFBulletinBoard constructor 2


}    // End Constructor




UTFBulletinBoardUI::UTFBulletinBoardUI ( const char *name, Widget parent ) : VkComponent ( name ) 
{ 
    //---- Start editable code block: UTFBulletinBoard pre-create


    //---- End editable code block: UTFBulletinBoard pre-create



    // Call creation function to build the widget tree.

     create ( parent );

    //---- Start editable code block: UTFBulletinBoard constructor


    //---- End editable code block: UTFBulletinBoard constructor


}    // End Constructor


UTFBulletinBoardUI::~UTFBulletinBoardUI() 
{
    // Base class destroys widgets

    //---- Start editable code block: UTFBulletinBoardUI destructor


    //---- End editable code block: UTFBulletinBoardUI destructor
}    // End destructor



void UTFBulletinBoardUI::create ( Widget parent )
{
    /* Cardinal count; */
    /* count = 0; */

    // Load any class-defaulted resources for this object

    setDefaultResources ( parent, _defaultUTFBulletinBoardUIResources  );


    // Create an unmanaged widget as the top of the widget hierarchy

    _baseWidget = _rTGBulletinBoard = XtVaCreateWidget ( _name,
                                                         xmBulletinBoardWidgetClass,
                                                         parent, 
                                                         XmNresizePolicy, XmRESIZE_GROW, 
                                                         (XtPointer) NULL ); 

    // install a callback to guard against unexpected widget destruction

    installDestroyHandler();


    // Create widgets used in this component
    // All variables are data members of this class

/*
    _label3 = XtVaCreateManagedWidget  ( "label3",
                                          xmLabelWidgetClass,
                                          _baseWidget, 
                                          XmNlabelType, XmSTRING, 
                                          XmNx, 10, 
                                          XmNy, 260, 
                                          XmNwidth, 134, 
                                          XmNheight, 20, 
                                          (XtPointer) NULL ); 
*/


/*
    _outputMaterials = XtVaCreateManagedWidget  ( "outputMaterials",
                                                         xmToggleButtonWidgetClass,
                                                         _baseWidget, 
                                                         XmNalignment, XmALIGNMENT_BEGINNING, 
                                                         XmNlabelType, XmSTRING, 
                                                         XmNx, 10, 
                                          XmNy, 100, 
                                          XmNwidth, 160, 
                                          XmNheight, 26, 
                                          (XtPointer) NULL ); 
*/


/*
    _animation = XtVaCreateManagedWidget  ( "animation",
                                             xmToggleButtonWidgetClass,
                                             _baseWidget, 
                                             XmNalignment, XmALIGNMENT_BEGINNING, 
                                             XmNlabelType, XmSTRING, 
                                             XmNx, 10, 
                                             XmNy, 290, 
                                             XmNwidth, 150, 
                                             XmNheight, 26, 
                                             (XtPointer) NULL ); 
*/


/*
    _label2 = XtVaCreateManagedWidget  ( "label2",
                                          xmLabelWidgetClass,
                                          _baseWidget, 
                                          XmNlabelType, XmSTRING, 
                                          XmNx, 10, 
                                          XmNy, 190, 
                                          XmNwidth, 128, 
                                          XmNheight, 20, 
                                          (XtPointer) NULL ); 
*/

    _label = XtVaCreateManagedWidget  ( "label",
                                         xmLabelWidgetClass,
                                         _baseWidget, 
                                         XmNlabelType, XmSTRING, 
                                         XmNx, 10, 
                                         XmNy, 0, 
                                         XmNwidth, 200, 
                                         XmNheight, 20, 
                                         (XtPointer) NULL ); 


    _label1 = XtVaCreateManagedWidget  ( "label1",
                                          xmLabelWidgetClass,
                                          _baseWidget, 
                                          XmNlabelType, XmSTRING, 
                                          XmNx, 10, 
                                          XmNy, 175, 
                                          XmNwidth, 245, 
                                          XmNheight, 20, 
                                          (XtPointer) NULL ); 

/*
    _vertexColors = XtVaCreateManagedWidget  ( "vertexColors",
                                                xmToggleButtonWidgetClass,
                                                _baseWidget, 
                                                XmNalignment, XmALIGNMENT_BEGINNING, 
                                                XmNlabelType, XmSTRING, 
                                                XmNx, 219, 
                                                XmNy, 71, 
                                                XmNwidth, 170, 
                                                         XmNheight, 26, 
                                                         (XtPointer) NULL ); 
*/

/*
    _outputTransformations = XtVaCreateManagedWidget  ( "outputTransformations",
                                                         xmToggleButtonWidgetClass,
                                                         _baseWidget, 
                                                         XmNalignment, XmALIGNMENT_BEGINNING, 
                                                         XmNlabelType, XmSTRING, 
                                                         XmNx, 220, 
                                                         XmNy, 220, 
                                                         XmNwidth, 168, 
                                                         XmNheight, 20, 
                                                         (XtPointer) NULL ); 
*/


/*
    _outputHierarchy = XtVaCreateManagedWidget  ( "outputHierarchy",
                                                 xmToggleButtonWidgetClass,
                                                 _baseWidget, 
                                                 XmNalignment, XmALIGNMENT_BEGINNING, 
                                                 XmNlabelType, XmSTRING, 
                                                 XmNx, 10, 
                                                 XmNy, 220, 
                                                 XmNwidth, 202, 
                                                 XmNheight, 26, 
                                                 (XtPointer) NULL ); 
*/

/*
    _indexCounters = XtVaCreateManagedWidget  ( "indexCounters",
                                                 xmToggleButtonWidgetClass,
                                                 _baseWidget, 
                                                 XmNalignment, XmALIGNMENT_BEGINNING, 
                                                 XmNlabelType, XmSTRING, 
                                                 XmNindicatorOn, True, 
                                                 XmNx, 10, 
                                                 XmNy, 131, 
                                                 XmNwidth, 168, 
                                                 XmNheight, 20, 
                                                 (XtPointer) NULL ); 
*/

/*
    _polygonNormals = XtVaCreateManagedWidget  ( "polygonNormals",
                                                  xmToggleButtonWidgetClass,
                                                  _baseWidget, 
                                                  XmNalignment, XmALIGNMENT_BEGINNING, 
                                                  XmNlabelType, XmSTRING, 
                                                  XmNx, 219, 
                                                  XmNy, 100, 
                                                  XmNwidth, 179, 
                                                  XmNheight, 26, 
                                                  (XtPointer) NULL ); 
*/
    _outputTextures = XtVaCreateManagedWidget  ( "outputTextures",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 40, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 

#if 0
#include <Xm/Scale.h>
static float D3DOption_Scalefactor = 1.0;
Boolean ColorAllocResult = False;
Arg args[10];
int argcount = 0;
XmString label;
static Widget D3DEWIDGET_scalefactor = NULL;
     // Add scalefactor widget
     if (D3DEWIDGET_scalefactor == NULL) {
          label = XmStringCreateLtoR("Scale Factor",XmSTRING_DEFAULT_CHARSET);
          argcount = 0;
/*
          if(ColorAllocResult) {
           XtSetArg(args[argcount],XmNbackground,BackgroundColor);argcount++;
           XtSetArg(args[argcount],XmNtopShadowColor,TopShadowColor);argcount++;
           XtSetArg(args[argcount],XmNbottomShadowColor,BottomShadowColor);argcount
++;
           XtSetArg(args[argcount],XmNforeground,ForegroundColor);argcount++;
          }
*/
          XtSetArg(args[argcount],XmNorientation, XmHORIZONTAL); argcount++;
          XtSetArg(args[argcount],XmNminimum,1); argcount++;
          XtSetArg(args[argcount],XmNmaximum,1000000); argcount++;
          XtSetArg(args[argcount],XmNdecimalPoints,3); argcount++;
          XtSetArg(args[argcount],XmNshowValue,True); argcount++;
          XtSetArg(args[argcount],XmNscaleMultiple,1000); argcount++;
          XtSetArg(args[argcount],XmNtitleString,label); argcount++;
          XtSetArg(args[argcount],XmNwidth,200); argcount++;
          XtSetArg(args[argcount],XmNx,0); argcount++;
          XtSetArg(args[argcount],XmNvalue,(int)(D3DOption_Scalefactor*1000.0)); argcount++;
          D3DEWIDGET_scalefactor = XmCreateScale(slidelayout,"Scalefactor",args,argcount);
          XtAddCallback(D3DEWIDGET_scalefactor,XmNvalueChangedCallback, D3DE_GuiCB, (XtPointer)D3DE_GUISCALE);
          XmStringFree(label);
     }
     XtManageChild(D3DEWIDGET_scalefactor);
#endif


    _outputMIPmaps = XtVaCreateManagedWidget  ( "NO MIP maps",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 70, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 

    _outputConvexHull = XtVaCreateManagedWidget  ( "Convex Hull",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 100, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 

    _removeComstantChannels = XtVaCreateManagedWidget  ( "NO static animations",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 130, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 
    _outputMesh = XtVaCreateManagedWidget  ( "Mesh",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 200, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 

    _outputAnimation = XtVaCreateManagedWidget  ( "Animation",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 230, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 
/*
    _outputSkeleton = XtVaCreateManagedWidget  ( "Skeleton",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 260, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 
*/

/*
    _outputCombined = XtVaCreateManagedWidget  ( "Combined",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 270, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 
*/

/*
    _OldFormat = XtVaCreateManagedWidget  ( "Old 3DB Format",
                                                      xmToggleButtonWidgetClass,
                                                      _baseWidget, 
                                                      XmNalignment, XmALIGNMENT_BEGINNING, 
                                                      XmNlabelType, XmSTRING, 
                                                      XmNx, 10, 
                                                      XmNy, 130, 
                                                      XmNwidth, 201, 
                                                      XmNheight, 26, 
                                                      (XtPointer) NULL ); 
*/

/*
    _vertexNormals = XtVaCreateManagedWidget  ( "vertexNormals",
                                                 xmToggleButtonWidgetClass,
                                                 _baseWidget, 
                                                 XmNalignment, XmALIGNMENT_BEGINNING, 
                                                 XmNlabelType, XmSTRING, 
                                                 XmNset, False, 
                                                 XmNx, 10, 
                                                 XmNy, 70, 
                                                 XmNwidth, 169, 
                                                 XmNheight, 26, 
                                                 (XtPointer) NULL ); 
*/



    //---- Start editable code block: UTFBulletinBoardUI create


    //---- End editable code block: UTFBulletinBoardUI create
}

const char * UTFBulletinBoardUI::className()
{
    return ("UTFBulletinBoardUI");
}    // End className()




/////////////////////////////////////////////////////////////// 
// The following functions are called from the menu items 
// in this window.
/////////////////////////////////// 



//---- Start editable code block: End of generated code


//---- End editable code block: End of generated code
