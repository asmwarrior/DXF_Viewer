// Arc.cpp: implementation of the CArc class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "dxf.h"
#include "Arc.h"

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

CArc::CArc()
    : cDXFGraphObject("ARC",cDXFGraphObject::eType::arc)
{
    m_Layer = "0";
    m_Select = FALSE;
    m_Nest = FALSE;
}

CArc::CArc( cCodeValue& cv )
    : CArc()
{
    myfValid =( cv.myValue == myCode );
}

CArc::~CArc()
{

}
void CArc::Update( cBoundingRectangle& rect )
{
    rect.Update( x, y+r );
    rect.Update( x, y-r );
    rect.Update( x+r, y );
    rect.Update( x-r, y );
}
bool CArc::getDraw( s_dxf_draw& draw )
{
    draw.x1 = x - r;
    draw.y1 = y + r;
    draw.rect->ApplyScale( draw.x1, draw.y1);
    draw.r =  2 * r / draw.rect->myScale;
    draw.sa = sa;
    draw.ea = ea;
    if( ea < sa )
    {
        // required to draw in clockwise direction
        // work arround for wxWidgets bug http://trac.wxwidgets.org/ticket/4437
        draw.ea += 360;
    }
    return true;
}

bool CArc::Append(  cvit_t& cvit )
{
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
            m_Layer = cvit->myValue;
            break;
        case 10:
            x = atof(cvit->myValue.c_str());
            break;
        case 20:
            y = atof(cvit->myValue.c_str());
            break;
        case 40:
            r = atof(cvit->myValue.c_str());
            break;
        case 50:
            sa = atof(cvit->myValue.c_str());
            break;
        case 51:
            ea = atof(cvit->myValue.c_str());
            break;
        }
    }
    return true;
}

}


