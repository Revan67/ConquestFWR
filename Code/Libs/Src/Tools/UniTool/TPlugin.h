#ifndef TPLUGIN_H
#define TPLUGIN_H
//
// TPlugIn.h - Template classes and macros for easily building Unitool compatible plugins
//

//
// Design Notes:
//      This header defines a template based implementation of IEventSource and IEventSink. These, along with some
// table defining macros, make it easier to enhance plugin objects via inheritance.
//      It is important to note that these only work for enhancement via inheritance, so they only work within a single
// plugin module. Enhancement via aggregation must be implemented by hand, for now.
// ...
//      ACK!!! Due to the variable size of a parameter spec for a method, this really can't be done cleanly with either
// templates or macros. The actual implementation code can be done with templates, but the table definitions are cleaner
// when compiled from an interface definition. This is easy to write in Perl.
//

#endif