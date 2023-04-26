/*****************************************************************************
*                                                                            *
*  Copyright (C) 2022-2023 Liu An Lin <liuanlin-mx@qq.com>                   *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/

#include <float.h>
#include <cstdint>
#include <vector>
#include "calc.h"

static double _float_epsilon = 0.00005;

double calc_angle(double x1, double y1, double x2, double y2)
{
    y1 = -y1;
    y2 = -y2;
    return atan2(y2 - y1, x2 - x1);
}

double calc_dist(double x1, double y1, double x2, double y2)
{
    return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}


double calc_angle(double ax, double ay, double bx, double by, double cx, double cy)
{
    double a = hypot(bx - cx, by - cy);
    double b = hypot(ax - cx, ay - cy);
    double c = hypot(ax - bx, ay - by);
    
    return acos((a * a + b * b - c * c) / (2. * a * b));
}


void calc_angle(double ax, double ay, double bx, double by, double cx, double cy, double& A, double& B, double& C)
{
    double a = hypot(bx - cx, by - cy);
    double b = hypot(ax - cx, ay - cy);
    double c = hypot(ax - bx, ay - by);
    
    A = acos((b * b + c * c - a * a) / (2. * b * c));
    B = acos((a * a + c * c - b * b) / (2. * a * c));
    C = acos((a * a + b * b - c * c) / (2. * a * b));
}


double calc_p2line_dist(double x1, double y1, double x2, double y2, double x, double y)
{
    double angle = calc_angle(x2, y2, x, y, x1, y1);
    
    double d = hypot(x - x1, y - y1);
    if (angle > (double)M_PI_2)
    {
        angle = (double)M_PI - angle;
    }
    if (fabs(angle - (double)M_PI_2) < FLT_EPSILON)
    {
        return d;
    }
    return sin(angle) * d;
}


bool calc_p2line_intersection(double x1, double y1, double x2, double y2, double x, double y, double& ix, double& iy)
{
    double A = 0.;
    double B = 0.;
    double C = 0.;
    
    calc_angle(x1, y1, x2, y2, x, y, A, B, C);
    if ((A > M_PI_2 && fabs(A - M_PI_2) > FLT_EPSILON)
        || (B > M_PI_2 && B - M_PI_2 > FLT_EPSILON))
    {
        return false;
    }
    
    if (fabs(A - (double)M_PI_2) < FLT_EPSILON)
    {
        ix = x1;
        iy = y1;
        return true;
    }
    else if (fabs(B - (double)M_PI_2) < FLT_EPSILON)
    {
        ix = x2;
        iy = y2;
        return true;
    }
    
    double line_len = hypot(x2 - x1, y2 - y1);
    double line_angle = calc_angle(x1, y1, x2, y2);
    double d = hypot(x - x1, y - y1);
    double len = cos(A) * d;
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

bool calc_parallel_lines_overlap(double ax1, double ay1, double ax2, double ay2,
                                                double bx1, double by1, double bx2, double by2,
                                                double& aox1, double& aoy1, double& aox2, double& aoy2,
                                                double& box1, double& boy1, double& box2, double& boy2)
{
    std::vector<std::pair<double, double> > ao;
    std::vector<std::pair<double, double> > bo;
    
    double x;
    double y;
    
    /* 计算过点(ax1, ay1)到线段b垂线的交点 */
    if (calc_p2line_intersection(bx1, by1, bx2, by2, ax1, ay1, x, y))
    {
        ao.push_back(std::pair<double, double>(ax1, ay1));
        bo.push_back(std::pair<double, double>(x, y));
    }
    

    if (calc_p2line_intersection(bx1, by1, bx2, by2, ax2, ay2, x, y))
    {
        ao.push_back(std::pair<double, double>(ax2, ay2));
        bo.push_back(std::pair<double, double>(x, y));
    }
    
    /* 计算过点(bx1, by1)到线段a垂线的交点 */
    if (calc_p2line_intersection(ax1, ay1, ax2, ay2, bx1, by1, x, y))
    {
        bo.push_back(std::pair<double, double>(bx1, by1));
        ao.push_back(std::pair<double, double>(x, y));
    }
        
    if (calc_p2line_intersection(ax1, ay1, ax2, ay2, bx2, by2, x, y))
    {
        bo.push_back(std::pair<double, double>(bx2, by2));
        ao.push_back(std::pair<double, double>(x, y));
    }
    
    for (std::uint32_t i = 0; i < ao.size(); i++)
    {
        for (std::uint32_t j = i + 1; j < ao.size();)
        {
            double d = hypot(ao[i].first - ao[j].first, ao[i].second - ao[j].second);
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
            double d = hypot(bo[i].first - bo[j].first, bo[i].second - bo[j].second);
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
    double alen = hypot(aox2 - aox1, aoy2 - aoy1);
    double blen = hypot(box2 - box1, boy2 - boy1);
    return fabs(alen - blen) < _float_epsilon;
}
               
    
double calc_parallel_lines_overlap_len(double ax1, double ay1, double ax2, double ay2,
                                                    double bx1, double by1, double bx2, double by2)
{
    double aox1;
    double aoy1;
    double aox2;
    double aoy2;
    double box1;
    double boy1;
    double box2;
    double boy2;
    if (calc_parallel_lines_overlap(ax1, ay1, ax2, ay2,
                                    bx1, by1, bx2, by2,
                                    aox1, aoy1, aox2, aoy2,
                                    box1, boy1, box2, boy2))
    {
        return hypot(aox2 - aox1, aoy2 - aoy1);
    }
    return 0;
}

void calc_arc_center_radius(double x1, double y1, double x2, double y2, double x3, double y3, double& x, double& y, double& radius)
{
    y1 = -y1;
    y2 = -y2;
    y3 = -y3;
    
    double a = x1 - x2;
    double b = y1 - y2;
    double c = x1 - x3;
    double d = y1 - y3;
    double e = ((x1 * x1 - x2 * x2) + (y1 * y1 - y2 * y2)) * 0.5;
    double f = ((x1 * x1 - x3 * x3) + (y1 * y1 - y3 * y3)) * 0.5;
    double det = b * c - a * d;
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
void calc_arc_angle(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y, double radius, double& angle)
{
    y1 = -y1;
    y2 = -y2;
    y3 = -y3;
    y = -y;
    
    double a = hypot(x2 - x1, y2 - y1) * 0.5;
    double angle1 = asin(a / radius) * 2;
    
    a = hypot(x3 - x2, y3 - y2) * 0.5;
    double angle2 = asin(a / radius) * 2;
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
