// Spline.cpp: implementation of the CSpline class.
//
//////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include <cmath>
#include "dxf.h"
#include "Spline.h"

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

CSpline::CSpline()
    : cDXFGraphObject("SPLINE",cDXFGraphObject::eType::spline)
    , m_Layer("0")
    , m_FitPointCount( 0 )
    , m_ControlPointCount( 0 )
    , m_Select( false )
    , m_Nest( false )
    , myfwxwidgets( false )
{
}

CSpline::CSpline( cCodeValue& cv )
    : CSpline()
{
    myfValid =( cv.myValue == myCode );
}

CSpline::~CSpline()
{

}

bool CSpline::Append(  cvit_t& cvit )
{
    int point_index = 0;
    while( true )
    {
        cvit++;
        //std::cout << "spline " << cvit->myCode <<", " << cvit->myValue << "\n";
        switch( cvit->Code() )
        {
        case 0:
            // a new object
            //std::cout << "spline points " << m_FitPointCount  <<" "<< m_ControlPointCount<<" "<< point_index << "\n";
            if( m_FitPointCount == 0 && m_ControlPointCount == 0 )
                throw std::runtime_error("Spline has no points");
            if( point_index != m_FitPointCount && point_index != m_ControlPointCount )
                throw std::runtime_error("Spline has unexpected number of points");
            Generate();
            cvit--;
            return false;

        case 8:
            m_Layer = cvit->myValue;
            break;

        case 74:
            m_FitPointCount = atoi(cvit->myValue.c_str());
            if( point_index >= MAXPOINTS )
                throw std::runtime_error("Too many spline points");
            if( m_ControlPointCount )
                throw std::runtime_error("Spline has both fit AND control points");
            break;
        case 11:
            x[point_index] = atof(cvit->myValue.c_str());
            break;
        case 21:
            if( ! m_FitPointCount )
                throw std::runtime_error("Spline has unexpected fit point");
            y[point_index++] = atof(cvit->myValue.c_str());
            break;


        case 73:
            m_ControlPointCount = atoi(cvit->myValue.c_str());
            if( point_index >= MAXPOINTS )
                throw std::runtime_error("Too many spline points");
            if( m_FitPointCount )
                throw std::runtime_error("Spline has both fit AND control points");
            break;
        case 10:
            x[point_index] = atof(cvit->myValue.c_str());
            break;
        case 20:
            if( ! m_ControlPointCount )
                throw std::runtime_error("Spline has unexpected control point");
            y[point_index++] = atof(cvit->myValue.c_str());
            break;

        }
    }
    return true;
}

void CSpline::Options( CDxf * dxf )
{
    myfwxwidgets = dxf->wxwidgets();
}

void CSpline::Update( cBoundingRectangle& rect )
{
    int count = m_FitPointCount;
    if( ! count )
        count = m_ControlPointCount;
    for( int k = 0; k < count; k++ )
    {
        rect.Update( x[k], y[k] );
    }
}
void CSpline::Generate()
{
    if( ! m_FitPointCount )
        return;

    float AMag, AMagOld;
    vector<float> k;
    vector< vector<float> > Mat( 3, vector<float>(m_FitPointCount) );

    // vector A
    for(int i= 0 ; i<=(int)m_FitPointCount-2 ; i++ )
    {
        Ax.push_back( x[i+1] - x[i] );
        Ay.push_back( y[i+1] - y[i] );
    }
    // k
    AMagOld = (float)sqrt(Ax[0]*Ax[0] + Ay[0]*Ay[0]);
    for( int i=0 ; i<=(int)m_FitPointCount-3 ; i++)
    {
        AMag = (float)sqrt(Ax[i+1]*Ax[i+1] + Ay[i+1]*Ay[i+1]);
        k.push_back( AMagOld / AMag );
        AMagOld = AMag;
    }
    k.push_back( 1.0f );

    // Matrix
    for( int i=1; i<=(int)m_FitPointCount-2; i++)
    {
        Mat[0][i] = 1.0f;
        Mat[1][i] = 2.0f*k[i-1]*(1.0f + k[i-1]);
        Mat[2][i] = k[i-1]*k[i-1]*k[i];
    }
    Mat[1][0] = 2.0f;
    Mat[2][0] = k[0];
    Mat[0][m_FitPointCount-1] = 1.0f;
    Mat[1][m_FitPointCount-1] = 2.0f*k[m_FitPointCount-2];

    //
    Bx.push_back( 3.0f*Ax[0] );
    By.push_back( 3.0f*Ay[0] );
    for(int i=1; i<=(int)m_FitPointCount-2; i++)
    {
        Bx.push_back( 3.0f*(Ax[i-1] + k[i-1]*k[i-1]*Ax[i]) );
        By.push_back( 3.0f*(Ay[i-1] + k[i-1]*k[i-1]*Ay[i]) );
    }
    Bx.push_back( 3.0f*Ax[m_FitPointCount-2] );
    By.push_back( 3.0f*Ay[m_FitPointCount-2] );

    //
    MatrixSolve(Bx, Mat );
    MatrixSolve(By, Mat );

    for( int i=0 ; i<=(int)m_FitPointCount-2 ; i++ )
    {
        Cx.push_back( k[i]*Bx[i+1] );
        Cy.push_back( k[i]*By[i+1] );
    }

}

void CSpline::MatrixSolve(
    vector<float>& B,
    vector< vector<float> >& Mat )
{
    float* Work = new float[m_FitPointCount];
    float* WorkB = new float[m_FitPointCount];
    for(int i=0; i<=m_FitPointCount-1; i++)
    {
        Work[i] = B[i] / Mat[1][i];
        WorkB[i] = Work[i];
    }

    for(int j=0 ; j<10 ; j++)
    {
        ///  need convergence judge
        Work[0] = (B[0] - Mat[2][0]*WorkB[1])/Mat[1][0];
        for(int i=1; i<m_FitPointCount-1 ; i++ )
        {
            Work[i] = (B[i]-Mat[0][i]*WorkB[i-1]-Mat[2][i]*WorkB[i+1])
                      /Mat[1][i];
        }
        Work[m_FitPointCount-1] = (B[m_FitPointCount-1] - Mat[0][m_FitPointCount-1]*WorkB[m_FitPointCount-2])/Mat[1][m_FitPointCount-1];

        for( int i=0 ; i<=m_FitPointCount-1 ; i++ )
        {
            WorkB[i] = Work[i];
        }
    }
    for(int i=0 ; i<=m_FitPointCount-1 ; i++ )
    {
        B[i] = Work[i];
    }
    delete[] Work;
    delete[] WorkB;
}

bool CSpline::getDraw( cDrawPrimitiveData& draw )
{
    if( ! m_FitPointCount)
    {
        return getDrawControlPoint( draw );
    }
    //adjust this factor to adjust the curve smoothness
    // a smaller value will result in a smoother display
    const float DIV_FACTOR = 10.0;

    int Ndiv = (int)(max(abs((int)Ax[draw.index]),abs((int)Ay[draw.index]))/DIV_FACTOR);
    if( ! Ndiv )
        return false;

    if( draw.index_curve == 0 && draw.index == 0 )
    {
        //The first point in the spline
        draw.x1 = x[draw.index];
        draw.y1 = y[draw.index];
        draw.rect->ApplyScale(draw.x1,draw.y1);
    }
    else if( draw.index_curve == Ndiv )
    {
        // No more points in current curve
        if( draw.index == m_FitPointCount-2 )
        {
            // No more fittine points
            return false;
        }

        // Draw the line between the last point of the previous curve
        // and the next fitting point
        draw.index++;
        draw.index_curve = 0;
        draw.x1 = draw.x2;
        draw.y1 = draw.y2;
        draw.x2 = x[draw.index];
        draw.y2 = y[draw.index];
        draw.rect->ApplyScale(draw.x2,draw.y2);
        return true;

    }
    else
    {
        // In the middle of drawing the curve between two fitting points
        // Start from the end of the previous line
        draw.x1 = draw.x2;
        draw.y1 = draw.y2;
    }

    // Calulate the next point in the curve between two fitting points
    float  t,f,g,h;
    t = 1.0f / (float)Ndiv * (float)draw.index_curve;
    f = t*t*(3.0f-2.0f*t);
    g = t*(t-1.0f)*(t-1.0f);
    h = t*t*(t-1.0f);
    draw.x2 = (int)(x[draw.index] + Ax[draw.index]*f + Bx[draw.index]*g + Cx[draw.index]*h);
    draw.y2 = (int)(y[draw.index] + Ay[draw.index]*f + By[draw.index]*g + Cy[draw.index]*h);

    draw.rect->ApplyScale(draw.x2,draw.y2);

    draw.index_curve++;

    return true;
}

bool CSpline::getDrawControlPoint( cDrawPrimitiveData& draw )
{
    // check if using wxwidgets spline method
    if( myfwxwidgets )
    {
        // check if more points are available
        if( draw.index == m_ControlPointCount )
        {
            if( draw.index_curve )
                return false;
            draw.index_curve = 1;
            return true;
        }

        draw.x1 = x[draw.index];
        draw.y1 = y[draw.index];
        draw.rect->ApplyScale(draw.x1,draw.y1);
        draw.x2 = x[draw.index+1];
        draw.y2 = y[draw.index+1];
        draw.rect->ApplyScale(draw.x2,draw.y2);
        draw.index++;
        draw.index_curve = 0;
        return true;
    }

    int Ndiv = 100;

    // check if more points are available
    if( draw.index == Ndiv - 1 )
        return false;

    getBezierPoint( draw.x1, draw.y1, ((float)draw.index)/Ndiv );
    getBezierPoint( draw.x2, draw.y2, ((float)(draw.index+1))/Ndiv );
    draw.rect->ApplyScale(draw.x1,draw.y1);
    draw.rect->ApplyScale(draw.x2,draw.y2);
    draw.index++;
    draw.index_curve = 0;
    return true;

}

struct vec2 {
    double x, y;
    vec2() {}
    vec2(double x, double y) : x(x), y(y) {}
};

vec2 operator + (vec2 a, vec2 b) {
    return vec2(a.x + b.x, a.y + b.y);
}

vec2 operator - (vec2 a, vec2 b) {
    return vec2(a.x - b.x, a.y - b.y);
}

vec2 operator * (double s, vec2 a) {
    return vec2(s * a.x, s * a.y);
}

void CSpline::getBezierPoint( double& ox, double& oy, double t )
{
   // https://stackoverflow.com/a/21642962/16582

    std::vector<vec2> tmp;
    for( int k=0; k < m_ControlPointCount; k++ )
        tmp.push_back( vec2( x[k], y[k] ) );
    int i = m_ControlPointCount - 1;
    while (i > 0) {
        for (int k = 0; k < i; k++)
            tmp[k] = tmp[k] + t * ( tmp[k+1] - tmp[k] );
        i--;
    }
    ox = tmp[0].x;
    oy = tmp[0].y;
}

}


