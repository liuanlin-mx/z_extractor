#include <float.h>
#include <cstdint>
#include <vector>
#include "calc.h"

static float _float_epsilon = 0.00005;

float calc_angle(float x1, float y1, float x2, float y2)
{
    y1 = -y1;
    y2 = -y2;
    return atan2(y2 - y1, x2 - x1);
}

float calc_dist(float x1, float y1, float x2, float y2)
{
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}


float calc_angle(float ax, float ay, float bx, float by, float cx, float cy)
{
    float a = hypot(bx - cx, by - cy);
    float b = hypot(ax - cx, ay - cy);
    float c = hypot(ax - bx, ay - by);
    
    return acos((a * a + b * b - c * c) / (2. * a * b));
}


void calc_angle(float ax, float ay, float bx, float by, float cx, float cy, float& A, float& B, float& C)
{
    float a = hypot(bx - cx, by - cy);
    float b = hypot(ax - cx, ay - cy);
    float c = hypot(ax - bx, ay - by);
    
    A = acos((b * b + c * c - a * a) / (2. * b * c));
    B = acos((a * a + c * c - b * b) / (2. * a * c));
    C = acos((a * a + b * b - c * c) / (2. * a * b));
}


float calc_p2line_dist(float x1, float y1, float x2, float y2, float x, float y)
{
    float angle = calc_angle(x2, y2, x, y, x1, y1);
    
    float d = hypot(x - x1, y - y1);
    if (angle > (float)M_PI_2)
    {
        angle = (float)M_PI - angle;
    }
    if (fabs(angle - (float)M_PI_2) < FLT_EPSILON)
    {
        return d;
    }
    return sin(angle) * d;
}


bool calc_p2line_intersection(float x1, float y1, float x2, float y2, float x, float y, float& ix, float& iy)
{
    float A = 0.;
    float B = 0.;
    float C = 0.;
    
    calc_angle(x1, y1, x2, y2, x, y, A, B, C);
    if (A > (float)M_PI_2 || B > (float)M_PI_2)
    {
        return false;
    }
    
    if (fabs(A - (float)M_PI_2) < FLT_EPSILON)
    {
        ix = x1;
        iy = y1;
        return true;
    }
    else if (fabs(B - (float)M_PI_2) < FLT_EPSILON)
    {
        ix = x2;
        iy = y2;
        return true;
    }
    
    float line_len = hypot(x2 - x1, y2 - y1);
    float line_angle = calc_angle(x1, y1, x2, y2);
    float d = hypot(x - x1, y - y1);
    float len = cos(A) * d;
    if (len > line_len)
    {
        ix = x2;
        iy = y2;
    }
    else
    {
        ix = x1 + len * cos(line_angle);
        iy = -(-y1 + len * sin(line_angle));
    }
    
    return true;
}

bool calc_parallel_lines_overlap(float ax1, float ay1, float ax2, float ay2,
                                                float bx1, float by1, float bx2, float by2,
                                                float& aox1, float& aoy1, float& aox2, float& aoy2,
                                                float& box1, float& boy1, float& box2, float& boy2)
{
    std::vector<std::pair<float, float> > ao;
    std::vector<std::pair<float, float> > bo;
    
    float x;
    float y;
    
    /* 计算过点(ax1, ay1)到线段b垂线的交点 */
    if (calc_p2line_intersection(bx1, by1, bx2, by2, ax1, ay1, x, y))
    {
        ao.push_back(std::pair<float, float>(ax1, ay1));
        bo.push_back(std::pair<float, float>(x, y));
    }
    

    if (calc_p2line_intersection(bx1, by1, bx2, by2, ax2, ay2, x, y))
    {
        ao.push_back(std::pair<float, float>(ax2, ay2));
        bo.push_back(std::pair<float, float>(x, y));
    }
    
    /* 计算过点(bx1, by1)到线段a垂线的交点 */
    if (calc_p2line_intersection(ax1, ay1, ax2, ay2, bx1, by1, x, y))
    {
        bo.push_back(std::pair<float, float>(bx1, by1));
        ao.push_back(std::pair<float, float>(x, y));
    }
        
    if (calc_p2line_intersection(ax1, ay1, ax2, ay2, bx2, by2, x, y))
    {
        bo.push_back(std::pair<float, float>(bx2, by2));
        ao.push_back(std::pair<float, float>(x, y));
    }
    
    for (std::uint32_t i = 0; i < ao.size(); i++)
    {
        for (std::uint32_t j = i + 1; j < ao.size();)
        {
            float d = hypot(ao[i].first - ao[j].first, ao[i].second - ao[j].second);
            if (d < _float_epsilon)
            {
                ao[j] = ao.back();
                ao.pop_back();
            }
            else
            {
                j++;
            }
        }
    }
    
    for (std::uint32_t i = 0; i < bo.size(); i++)
    {
        for (std::uint32_t j = i + 1; j < bo.size();)
        {
            float d = hypot(bo[i].first - bo[j].first, bo[i].second - bo[j].second);
            if (d < _float_epsilon)
            {
                bo[j] = bo.back();
                bo.pop_back();
            }
            else
            {
                j++;
            }
        }
    }
    
    if (ao.size() != 2 || bo.size() != 2)
    {
        return false;
    }
    aox1 = ao[0].first;
    aoy1 = ao[0].second;
    aox2 = ao[1].first;
    aoy2 = ao[1].second;
    
    box1 = bo[0].first;
    boy1 = bo[0].second;
    box2 = bo[1].first;
    boy2 = bo[1].second;
    float alen = hypot(aox2 - aox1, aoy2 - aoy1);
    float blen = hypot(box2 - box1, boy2 - boy1);
    return fabs(alen - blen) < _float_epsilon;
}
               
    
float calc_parallel_lines_overlap_len(float ax1, float ay1, float ax2, float ay2,
                                                    float bx1, float by1, float bx2, float by2)
{
    float aox1;
    float aoy1;
    float aox2;
    float aoy2;
    float box1;
    float boy1;
    float box2;
    float boy2;
    if (calc_parallel_lines_overlap(ax1, ay1, ax2, ay2,
                                    bx1, by1, bx2, by2,
                                    aox1, aoy1, aox2, aoy2,
                                    box1, boy1, box2, boy2))
    {
        return hypot(aox2 - aox1, aoy2 - aoy1);
    }
    return 0;
}

void calc_arc_center_radius(float x1, float y1, float x2, float y2, float x3, float y3, float& x, float& y, float& radius)
{
    y1 = -y1;
    y2 = -y2;
    y3 = -y3;
    
    float a = x1 - x2;
    float b = y1 - y2;
    float c = x1 - x3;
    float d = y1 - y3;
    float e = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) * 0.5;
    float f = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) * 0.5;
    float det = b * c - a * d;
    if (fabs(det) < 1e-5)
    {
        radius = -1;
        x = 0;
        y = 0;
    }

    x = -(d * e - b * f) / det;
    y = -(a * f - c * e) / det;
    radius = hypot(x1 - x, y1 - y);
    y = -y;
}

/* (x1, y1)起点 (x2, y2)中点 (x3, y3)终点 (x, y)圆心 radius半径*/
void calc_arc_angle(float x1, float y1, float x2, float y2, float x3, float y3, float x, float y, float radius, float& angle)
{
    y1 = -y1;
    y2 = -y2;
    y3 = -y3;
    y = -y;
    
    float a = hypot(x2 - x1, y2 - y1) * 0.5;
    float angle1 = asin(a / radius) * 2;
    
    a = hypot(x3 - x2, y3 - y2) * 0.5;
    float angle2 = asin(a / radius) * 2;
    angle = angle1 + angle2;
    
    /* <0 为顺时针方向 */
    if ((x2 - x1) * (y3 - y2) - (y2 - y1) * (x3 - x2) < 0)
    {
        angle = -angle;
    }
}

float calc_arc_len(float radius, float angle)
{
    return radius * fabs(angle);
}

