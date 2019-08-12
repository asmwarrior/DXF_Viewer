// PolyLine.cpp: implementation of the cLWPolyLine class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "dxf.h"
#include "PolyLine.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

namespace dxfv
{

cLWPolyLine::cLWPolyLine()
: cDXFGraphObject("LWPOLYLINE",cDXFGraphObject::eType::lwpolyline )
{
    m_VertexCount = 0;
    myfClosed = false;
    m_Layer = "0";
    m_Select = FALSE;
    m_Nest = FALSE;
}

cLWPolyLine::cLWPolyLine( cCodeValue& cv )
    : cLWPolyLine()
{
    myfValid =( cv.myValue == myCode );
}

cLWPolyLine::~cLWPolyLine()
{

}

cPolyLine::cPolyLine()
{
    myCode = "POLYLINE";
    myType = cDXFGraphObject::eType::polyline;
}

cPolyLine::cPolyLine( cCodeValue& cv )
    : cPolyLine()
{
    myfValid =( cv.myValue == myCode );
}

cPolyLine::~cPolyLine()
{

}

bool cLWPolyLine::Append(  cvit_t& cvit )
{
    int point_index = 0;
    while( true )
    {
        cvit++;
        switch( cvit->Code() )
        {
        case 0:
            // a new object
            cvit--;
            return false;
        case 8:
            m_Layer = cvit->myCode;
            break;
        case 90:
            m_VertexCount = atoi(cvit->myValue.c_str());
            break;
        case 70:
            myfClosed = atoi(cvit->myValue.c_str());
            break;
        case 10:
            x[point_index] = atof(cvit->myValue.c_str());
            break;
        case 20:
            y[point_index++] = atof(cvit->myValue.c_str());
            break;
        }
    }
    return true;
}

bool cPolyLine::Append(  cvit_t& cvit )
{
    int point_index = 0;
    bool fVertex = false;
    while( true )
    {
        cvit++;
        switch( cvit->Code() )
        {
        case 0:
            // a new object
            if( cvit->myValue == "VERTEX" ) {
                    fVertex = true;
                break;
            }
            m_VertexCount = point_index;
            cvit--;
            return false;
        case 8:
            m_Layer = cvit->myCode;
            break;
        case 70:
            myfClosed = atoi(cvit->myValue.c_str());
            break;
        case 10:
            if( fVertex )
                x[point_index] = atof(cvit->myValue.c_str());
            break;
        case 20:
            if( fVertex )
                y[point_index++] = atof(cvit->myValue.c_str());
            break;
        }
    }

    return true;
}

void cLWPolyLine::Update( cBoundingRectangle& rect )
{
    for( int k = 0; k < (int)m_VertexCount; k++ )
    {
        rect.Update( x[k], y[k] );
    }
}


bool cLWPolyLine::getDraw( s_dxf_draw& draw )
{
    if( 0 > draw.index || draw.index == (int)m_VertexCount )
        return false;
    if( draw.index == (int)m_VertexCount  )
        return false;
    if( draw.index == (int)m_VertexCount-1  )
    {
        if( ! myfClosed )
        {
            return false;
        }
        else
        {
            draw.x1 = x[draw.index];
            draw.y1 = y[draw.index];
            draw.x2 = x[0];
            draw.y2 = y[0];
            draw.index++;
        }
    }
    else
    {
        draw.x1 = x[draw.index];
        draw.y1 = y[draw.index];
        draw.index++;
        draw.x2 = x[draw.index];
        draw.y2 = y[draw.index];
    }
    draw.rect->ApplyScale( draw.x1, draw.y1 );
    draw.rect->ApplyScale( draw.x2, draw.y2 );

     return true;
}
}

