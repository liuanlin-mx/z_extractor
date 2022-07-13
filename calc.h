#ifndef __CALC_H__
#define __CALC_H__
#include <math.h>

/* 计算两点连线倾斜角 */
float calc_angle(float x1, float y1, float x2, float y2);
float calc_dist(float x1, float y1, float x2, float y2);

/* ACB 计算点C为中心点的夹角 */
float calc_angle(float ax, float ay, float bx, float by, float cx, float cy);
/* 计算三角形的三个夹角 */
void calc_angle(float ax, float ay, float bx, float by, float cx, float cy, float& A, float& B, float& C);

/* 计算点到线段或其延长线的垂直距离 */
float calc_p2line_dist(float x1, float y1, float x2, float y2, float x, float y);

/* 计算过点(x y)到线段的垂线跟线段的交点 */
bool calc_p2line_intersection(float x1, float y1, float x2, float y2, float x, float y, float& ix, float& iy);

/* 计算两条平行线段的交叠区域 */
bool calc_parallel_lines_overlap(float ax1, float ay1, float ax2, float ay2,
                                    float bx1, float by1, float bx2, float by2,
                                    float& aox1, float& aoy1, float& aox2, float& aoy2,
                                    float& box1, float& boy1, float& box2, float& boy2);
                       
/* 计算两条平行线段的交叠区域长度 */             
float calc_parallel_lines_overlap_len(float ax1, float ay1, float ax2, float ay2,
                                    float bx1, float by1, float bx2, float by2);


void calc_arc_center_radius(float x1, float y1, float x2, float y2, float x3, float y3, float& x, float& y, float& radius);

/* (x1, y1)起点 (x2, y2)中点 (x3, y3)终点 (x, y)圆心 radius半径*/
void calc_arc_angle(float x1, float y1, float x2, float y2, float x3, float y3, float x, float y, float radius, float& angle);
float calc_arc_len(float radius, float angle);
    
#endif