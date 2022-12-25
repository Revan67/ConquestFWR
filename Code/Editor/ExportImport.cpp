//--------------------------------------------------------------------------//
//                                                                          //
//                                System.cpp                                //
//                                                                          //
// implments the SYSTEM component with is like the SysMap from CQ           //
//                                                                          //
//--------------------------------------------------------------------------//
 
#include "stdafx.h"
#include "globals.h"

#include "SystemStructs.h" 
#include "ExportImport.h" // to include ExportImport, must put SystemStructs.h first

#include "CQTrace.h"
#include "tinyxml\tinyxml.h"
#include "Object.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void Export::XML::rect( RECT& _rect, TiXmlNode& _node )
{
	TiXmlElement rect("RECT");
		rect.SetAttribute("left", _rect.left );
		rect.SetAttribute("top",  _rect.top );
		rect.SetAttribute("right", _rect.right );
		rect.SetAttribute("bottom", _rect.bottom );
	_node.InsertEndChild( rect );
}

void Export::XML::frect( FRect& _frect, TiXmlNode& _node )
{
	TiXmlElement rect("FRECT");
		rect.SetAttribute("ulX", _frect.UpperLeftCorner.X );
		rect.SetAttribute("ulY", _frect.UpperLeftCorner.Y );
		rect.SetAttribute("lrX", _frect.LowerRightCorner.X );
		rect.SetAttribute("lrY", _frect.LowerRightCorner.Y );
	_node.InsertEndChild( rect );
}

void Export::XML::jlist( JList& _list, TiXmlNode& _node )
{
	TiXmlElement jumpPonitList("JUMPPOINTLIST");
	jumpPonitList.SetAttribute("size", _list.GetSize() );

	for( int i = 0; i < _list.GetSize(); i++ )
	{
		JumpPoint& p = _list.GetAt(i);

		TiXmlElement point("JUMPPOINT");
			point.SetAttribute( "archname", p.archname );
			point.SetAttribute( "bJumpAllowed", p.bJumpAllowed );
			point.SetAttribute( "cPoint_x", p.cPoint.x );
			point.SetAttribute( "cPoint_y", p.cPoint.y );
			point.SetAttribute( "fPoint_X", p.fPoint.X );
			point.SetAttribute( "fPoint_Y", p.fPoint.Y );
			point.SetAttribute( "id", p.id );
			point.SetAttribute( "destWormholeID", p.destWormholeID );
			point.SetAttribute( "destSystemID", p.destSystemID );
			point.SetAttribute( "partName", p.partName );
			point.SetAttribute( "startX", p.startX );
			point.SetAttribute( "startY", p.startY );
			point.SetAttribute( "x", p.x );
			point.SetAttribute( "y", p.y );
		jumpPonitList.InsertEndChild( point );
	}

	_node.InsertEndChild( jumpPonitList );
}

void Export::XML::transform( Transform& _xform, TiXmlNode& _node )
{
	TiXmlElement data("TRANSFORM");

		data.SetAttribute("d00", _xform.d[0][0] );
		data.SetAttribute("d10", _xform.d[1][0] );
		data.SetAttribute("d20", _xform.d[2][0] );

		data.SetAttribute("d01", _xform.d[0][1] );
		data.SetAttribute("d11", _xform.d[1][1] );
		data.SetAttribute("d21", _xform.d[2][1] );

		data.SetAttribute("d02", _xform.d[0][2] );
		data.SetAttribute("d12", _xform.d[1][2] );
		data.SetAttribute("d22", _xform.d[2][2] );

		data.SetAttribute("x", _xform.translation.x );
		data.SetAttribute("y", _xform.translation.y );
		data.SetAttribute("z", _xform.translation.z );

	_node.InsertEndChild( data );
}

void Export::XML::objectData( ObjectData& _data, TiXmlNode& _node )
{
	TiXmlElement data("OBJECTDATA");
		data.SetAttribute("archetype", _data.archetype);
		data.SetAttribute("id", _data.id);
		Export::XML::transform( _data.xform, data );
	_node.InsertEndChild( data );
}

//-----------------------------------------------------------------------------------------------------
// Import XML
//-----------------------------------------------------------------------------------------------------

void Import::XML::rect( RECT& _rect, TiXmlNode* _node )
{
	if( _node && !strcmp(_node->Value(),"RECT") )
	{
		TiXmlElement* e = _node->ToElement();
		if( e )
		{
			_rect.left	 = atoi( e->Attribute("left") );
			_rect.top	 = atoi( e->Attribute("top") );
			_rect.right	 = atoi( e->Attribute("right") );
			_rect.bottom = atoi( e->Attribute("bottom") );
		}
	}
}

void Import::XML::frect( FRect& _frect, TiXmlNode* _node )
{
	if( _node && !strcmp(_node->Value(),"FRECT") )
	{
		TiXmlElement* e = _node->ToElement();
		if( e )
		{
			_frect.UpperLeftCorner.X  = atof( e->Attribute("ulX") );
			_frect.UpperLeftCorner.Y  = atof( e->Attribute("ulY") );
			_frect.LowerRightCorner.X = atof( e->Attribute("lrX") );
			_frect.LowerRightCorner.Y = atof( e->Attribute("lrY") );
		}
	}
}

void Import::XML::jlist( JList& _list, TiXmlNode* _node )
{
	if( _node && !strcmp(_node->Value(),"JUMPPOINTLIST") )
	{
		TiXmlElement* e = _node->ToElement();
		if( !e ) return;

		TiXmlNode* point = e->FirstChild("JUMPPOINT");
		while( point )
		{
			TiXmlElement* pnt = point->ToElement();
			if( pnt )
			{
				JumpPoint p;

				p.archname		 = pnt->Attribute("archname");
				p.bJumpAllowed	 = pnt->GetAttributeUnsignedLong("bJumpAllowed");
				p.cPoint.x		 = pnt->GetAttributeLong("cPoint_x");
				p.cPoint.y		 = pnt->GetAttributeLong("cPoint_y");
				p.fPoint.X		 = pnt->GetAttributeFloat("fPoint_X");
				p.fPoint.Y		 = pnt->GetAttributeFloat("fPoint_Y");
				p.id			 = pnt->GetAttributeUnsignedLong("id");
				p.destWormholeID = pnt->GetAttributeUnsignedLong("destWormholeID");
				p.destSystemID	 = pnt->GetAttributeUnsignedLong("destSystemID");
				p.partName		 = pnt->Attribute("partName");
				p.startX		 = pnt->GetAttributeLong("startX");
				p.startY		 = pnt->GetAttributeLong("startY");
				p.x				 = pnt->GetAttributeLong("x");
				p.y				 = pnt->GetAttributeLong("y");

				_list.Add(p);
			}
			point = point->NextSibling();
		}
	}
}

void Import::XML::transform( Transform& _xform, TiXmlNode* _node )
{
	if( _node && !strcmp(_node->Value(),"TRANSFORM") )
	{
		TiXmlElement* data = _node->ToElement();
		if( data )
		{
			data->QueryFloatAttribute("d00", &_xform.d[0][0] );
			data->QueryFloatAttribute("d10", &_xform.d[1][0] );
			data->QueryFloatAttribute("d20", &_xform.d[2][0] );

			data->QueryFloatAttribute("d01", &_xform.d[0][1] );
			data->QueryFloatAttribute("d11", &_xform.d[1][1] );
			data->QueryFloatAttribute("d21", &_xform.d[2][1] );

			data->QueryFloatAttribute("d02", &_xform.d[0][2] );
			data->QueryFloatAttribute("d12", &_xform.d[1][2] );
			data->QueryFloatAttribute("d22", &_xform.d[2][2] );

			data->QueryFloatAttribute("x", &_xform.translation.x );
			data->QueryFloatAttribute("y", &_xform.translation.y );
			data->QueryFloatAttribute("z", &_xform.translation.z );
		}
	}
}

void Import::XML::objectData( ObjectData& _data, TiXmlNode* _node )
{
	if( _node && !strcmp(_node->Value(),"OBJECTDATA") )
	{
		TiXmlElement* e = _node->ToElement();
		if( e )
		{
			_data.archetype = e->Attribute("archetype");
			e->QueryUnsignedIntValue("id", &_data.id);
			Import::XML::transform( _data.xform, e);
		}
	}
}

void Import::XML::widestring( wchar_t* _buffer, int _buffSize, const char* _name, TiXmlNode* _node )
{
	TiXmlElement* e = _node->ToElement();

	if( e->Attribute(_name) )
	{
		ZeroMemory( _buffer, sizeof(wchar_t) * _buffSize );
		const char* string = e->Attribute(_name);
		::MultiByteToWideChar(CP_ACP, 0, string, strlen(string), _buffer, _buffSize );
	}
}
