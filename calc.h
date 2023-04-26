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

#ifndef __CALC_H__
#define __CALC_H__
#include <math.h>

/* 计算两点连线倾斜角 */
double calc_angle(double x1, double y1, double x2, double y2);
double calc_dist(double x1, double y1, double x2, double y2);

/* ACB 计算点C为中心点的夹角 */
double calc_angle(double ax, double ay, double bx, double by, double cx, double cy);
/* 计算三角形的三个夹角 */
void calc_angle(double ax, double ay, double bx, double by, double cx, double cy, double& A, double& B, double& C);

/* 计算点到线段或其延长线的垂直距离 */
double calc_p2line_dist(double x1, double y1, double x2, double y2, double x, double y);

/* 计算过点(x y)到线段的垂线跟线段的交点 */
bool calc_p2line_intersection(double x1, double y1, double x2, double y2, double x, double y, double& ix, double& iy);

/* 计算两条平行线段的交叠区域 */
bool calc_parallel_lines_overlap(double ax1, double ay1, double ax2, double ay2,
                                    double bx1, double by1, double bx2, double by2,
                                    double& aox1, double& aoy1, double& aox2, double& aoy2,
                                    double& box1, double& boy1, double& box2, double& boy2);
                       
/* 计算两条平行线段的交叠区域长度 */             
double calc_parallel_lines_overlap_len(double ax1, double ay1, double ax2, double ay2,
                                    double bx1, double by1, double bx2, double by2);


void calc_arc_center_radius(double x1, double y1, double x2, double y2, double x3, double y3, double& x, double& y, double& radius);

/* (x1, y1)起点 (x2, y2)中点 (x3, y3)终点 (x, y)圆心 radius半径*/
void calc_arc_angle(double x1, double y1, double x2, double y2, double x3, double y3, double x, double y, double radius, double& angle);
float calc_arc_len(float radius, float angle);
    
#endif