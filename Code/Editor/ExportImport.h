//--------------------------------------------------------------------------//
//                                                                          //
//                                ExportImport.h                            //
//                                                                          //
// implments the SYSTEM component with is like the SysMap from CQ           //
//                                                                          //
//--------------------------------------------------------------------------//
 
#ifndef EXPORT_IMPORT_H_FILE
#define EXPORT_IMPORT_H_FILE

class Transform;
struct SYSTEM_DATA;
struct ObjectDat;

namespace Export
{
	namespace XML
	{
		void rect( RECT& _rect, class TiXmlNode& _node );

		void frect( FRect& _frect, class TiXmlNode& _node );

		void jlist( JList& _list, class TiXmlNode& _node );

		void transform( Transform& _xform, class TiXmlNode& _node );

		void objectData( ObjectData& _data, class TiXmlNode& _node );
	}
}

namespace Import
{
	namespace XML
	{
		void rect( RECT& _rect, class TiXmlNode* _node );

		void frect( FRect& _frect, class TiXmlNode* _node );

		void jlist( JList& _list, class TiXmlNode* _node );

		void systemData( SYSTEM_DATA& _data, class TiXmlNode* _node );

		void transform( Transform& _xform, class TiXmlNode* _node );

		void objectData( ObjectData& _data, class TiXmlNode* _node );

		void widestring( wchar_t* _buffer, int _buffSize, const char* _name, class TiXmlNode* _node );
	}
}

#endif