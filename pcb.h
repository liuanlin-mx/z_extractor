/*****************************************************************************
*                                                                            *
*  Copyright (C) 2023 Liu An Lin <liuanlin-mx@qq.com>                        *
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

#ifndef __PCB_H__
#define __PCB_H__

#include <cstdint>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <math.h>
#include <opencv2/opencv.hpp>

class pcb
{
public:
    struct point
    {
        point(): x(0), y(0) { }
        point(float x_, float y_): x(x_), y(y_) { }
        float x;
        float y;
        bool operator < (const point& p) const {
            return x < p.x || (!(p.x < x) && y < p.y);
        }
    };

    struct gr
    {
        enum
        {
            GR_POLY = 0,
            GR_ARC,
            GR_CIRCLE,
            GR_LINE,
            GR_RECT,
            GR_TEXT
        };
        
        enum
        {
            FILL_NONE,
            FILL_SOLID
        };
        
        enum
        {
            STROKE_NONE,
            STROKE_SOLID
        };
        
        gr()
            : gr_type(GR_LINE)
            , fill_type(FILL_SOLID)
            , stroke_width(0)
            , stroke_type(STROKE_SOLID)
        {
            
        }
        
        std::string tstamp;
        std::string layer_name;
        
        std::int32_t gr_type;
        std::int32_t fill_type;
        
        std::list<point> pts;
        
        point start; //start or center
        point mid;
        point end;
        
        //float width;
        
        float stroke_width;
        std::int32_t stroke_type;
    };
    
    struct zone
    {
        std::list<point> pts;
        std::string layer_name;
        std::uint32_t net;
        std::string tstamp;
    };
    
    struct segment
    {
        segment(): width(0), net(0) {}
        bool is_arc() const { return mid.x != 0 || mid.y != 0; }
        
        point start;
        point mid;
        point end;
        float width;
        std::string layer_name;
        std::uint32_t net;
        std::string tstamp;
    };
    
    struct via
    {
        via(): size(0), drill(0), net(0) {}
        point at;
        float size;
        float drill;
        std::list<std::string> layers;
        std::uint32_t net;
        std::string tstamp;
    };
    
    struct net
    {
        net(): id(0) {}
        std::uint32_t id;
        std::string name;
    };
    
    enum
    {
        PAD_THRU_HOLE_RECT,
        PAD_SMD_ROUNDRECT,
    };
    
    enum
    {
        LAYER_TYPE_COPPER = 0,
        LAYER_TYPE_CORE
    };
    
    struct pad
    {
        enum
        {
            SHAPE_RECT,
            SHAPE_CIRCLE,
            SHAPE_ROUNDRECT,
            SHAPE_OVAL,
            SHAPE_TRAPEZOID,
        };
        enum
        {
            TYPE_SMD,
            TYPE_THRU_HOLE,
            TYPE_CONNECT,
        };
        
        pad()
            : type(TYPE_SMD), shape(SHAPE_RECT), net(0), ref_at_angle(0), at_angle(0)
            , size_w(0)
            , size_h(0)
            , drill(0)
            {}
        std::string footprint;
        std::string pad_number;
        std::uint32_t type;
        std::uint32_t shape;
        std::uint32_t net;
        std::string net_name;
        
        point ref_at;
        float ref_at_angle;
        
        point at;
        float at_angle;
        float size_w;
        float size_h;
        float drill;
        std::list<std::string> layers;
        std::string tstamp;
    };
    
    struct footprint
    {
        footprint()
            : at_angle(0)
        {
        }
        std::string layer;
        std::string tstamp;
        std::string reference;
        std::string value;
        point at;
        float at_angle;
        std::vector<gr> grs;
        std::vector<pad> pads;
    };
    
    struct layer
    {
        enum
        {
            COPPER,
            DIELECTRIC,
            CORE = DIELECTRIC,
            PREPREG = DIELECTRIC,
            TOP_SOLDER_MASK,
            BOTTOM_SOLDER_MASK
        };
        
        layer(): type(COPPER), thickness(0), epsilon_r(1), loss_tangent(0) {}
        
        std::string name;
        std::string aname;
        std::uint32_t type;
        float thickness;
        float epsilon_r;
        float loss_tangent;
    };
    
public:
    pcb();
    ~pcb();
    
public:
    
    bool add_net(std::uint32_t id, std::string name);
    bool add_segment(const segment& s);
    bool add_via(const via& v);
    bool add_zone(const zone& z);
    bool add_footprint(const footprint& f);
    bool add_pad(const pad& p);
    bool add_layer(const layer& l);
    bool add_gr(const gr& g);
    void set_edge(float top, float bottom, float left, float right);
    
    void ignore_cu_thickness(bool b) { _ignore_cu_thickness = b; }
    void dump();
    cv::Mat draw(const std::string& layer_name, float pix_unit = 0.1);
    cv::Mat draw_zone(const std::string& layer_name, std::uint32_t net_id, float pix_unit = 0.1);
    
    void clean_segment();
    void clean_segment(std::uint32_t net_id);
    void clean_segment(const std::list<std::string>& nets);
    
    float get_edge_top() { return _pcb_top; }
    float get_edge_bottom() { return _pcb_bottom; }
    float get_edge_left() { return _pcb_left; }
    float get_edge_right() { return _pcb_right; }
    float get_edge_size_x() { return fabs(_pcb_right - _pcb_left); }
    float get_edge_size_y() { return fabs(_pcb_bottom - _pcb_top); }
    
    std::vector<layer> get_layers() { return _layers; }
    std::list<segment> get_segments(std::uint32_t net_id);
    const std::vector<footprint>& get_footprints();
    std::list<pad> get_pads(std::uint32_t net_id);
    bool get_footprint(const std::string& fp_ref, footprint& fp);
    bool get_pad(const std::string& footprint, const std::string& pad_number, pcb::pad& pad);
    std::list<via> get_vias(std::uint32_t net_id);
    std::list<via> get_vias(const std::vector<std::uint32_t>& net_ids);
    std::list<zone> get_zones(std::uint32_t net_id);
    std::vector<std::list<pcb::segment> > get_segments_sort(std::uint32_t net_id);
    std::string get_net_name(std::uint32_t net_id);
    std::uint32_t get_net_id(std::string name);
    
    
    void get_pad_pos(const pad& p, float& x, float& y);
    void get_rotation_pos(const point& c, float rotate_angle, point& p);
    std::string get_tstamp_short(const std::string& tstamp);
    static std::string format_net(const std::string& name);
    std::string pos2net(float x, float y, const std::string& layer);
    static std::string format_net_name(const std::string& net_name);
    std::string format_layer_name(std::string layer_name);
    static std::string gen_pad_net_name(const std::string& footprint, const std::string& net_name);
    
    std::vector<std::string> get_all_cu_layer();
    std::vector<std::string> get_all_dielectric_layer();
    std::vector<std::string> get_all_mask_layer();
    std::vector<std::string> get_via_layers(const via& v);
    std::vector<std::string> get_via_conn_layers(const via& v);
    float get_via_conn_len(const pcb::via& v);
    
    bool is_cu_layer(const std::string& layer);
    
    std::vector<std::string> get_pad_conn_layers(const pad& p);
    std::vector<std::string> get_pad_layers(const pad& p);
    
    
    std::string get_layer_name(const std::string& aname);
    float get_layer_distance(const std::string& layer_name1, const std::string& layer_name2);
    float get_layer_thickness(const std::string& layer_name);
    float get_layer_z_axis(const std::string& layer_name);
    float get_layer_epsilon_r(const std::string& layer_name);
    float get_cu_layer_epsilon_r(const std::string& layer_name);
    float get_layer_epsilon_r(const std::string& layer_start, const std::string& layer_end);
    float get_layer_loss_tangent(const std::string& layer_name);
    float get_board_thickness();
    float get_cu_min_thickness();
    float get_min_thickness(std::uint32_t layer_type);
    bool cu_layer_is_outer_layer(const std::string& layer_name);
    
    
    /* 检测是否有未连接的走线 */
    bool check_segments(std::uint32_t net_id);
    void get_no_conn_segments(std::uint32_t net_id, std::list<std::pair<std::uint32_t/*1 start未连接 2 end,3 all*/, pcb::segment> >& no_conn, std::list<pcb::segment>& conn);
    
    /* 获取走线长度 */
    float get_segment_len(const pcb::segment& s);
    /* 获取从起点向终点前进指定offset后的坐标 */
    void get_segment_pos(const pcb::segment& s, float offset, float& x, float& y);
    
    /* 获取过从起点向终点前进指定offset后的点且垂直与走线的一条线段 长度为 w 线段中点与走线相交 */
    void get_segment_perpendicular(const pcb::segment& s, float offset, float w, float& x_left, float& y_left, float& x_right, float& y_right);
    
    /* 找到下一个连接到(x, y)的走线 */
    bool segments_get_next(std::list<segment>& segments, pcb::segment& s, float x, float y, const std::string& layer_name);

    
    /* 判断走线是否在焊盘内， 返回1 start端在焊盘内， 2 end端在焊盘内, 3两端都在盘焊内, 0两端都不在焊盘内 */
    std::uint32_t segment_is_inside_pad(const pcb::segment& s, const pcb::pad& pad);
    
    
    
    float cvt_img_x(float x, float pix_unit) { return _cvt_img_x(x, pix_unit); }
    float cvt_img_y(float y, float pix_unit) { return _cvt_img_y(y, pix_unit); }
    float cvt_img_len(float len, float pix_unit) { return _cvt_img_len(len, pix_unit); }
    
    float cvt_pcb_x(float x, float pix_unit) { return _cvt_pcb_x(x, pix_unit); }
    float cvt_pcb_y(float y, float pix_unit) { return _cvt_pcb_y(y, pix_unit); }
    float cvt_pcb_len(float len, float pix_unit) { return _cvt_pcb_len(len, pix_unit); }
    
    float get_pcb_img_cols(float pix_unit) { return _get_pcb_img_cols(pix_unit); }
    float get_pcb_img_rows(float pix_unit) { return _get_pcb_img_rows(pix_unit); }
    
private:
    bool _float_equal(float a, float b);
    bool _point_equal(float x1, float y1, float x2, float y2);
    
    float _cvt_img_x(float x, float pix_unit) { return round((x - _pcb_left) / pix_unit); }
    float _cvt_img_y(float y, float pix_unit) { return round((y - _pcb_top) / pix_unit); }
    float _cvt_img_len(float len, float pix_unit) { return round(len / pix_unit); }
    
    float _cvt_pcb_x(float x, float pix_unit) { return x * pix_unit + _pcb_left; }
    float _cvt_pcb_y(float y, float pix_unit) { return y * pix_unit + _pcb_top; }
    float _cvt_pcb_len(float len, float pix_unit) { return len * pix_unit; }
    
    float _get_pcb_img_cols(float pix_unit) { return round((_pcb_right - _pcb_left) / pix_unit); }
    float _get_pcb_img_rows(float pix_unit) { return round((_pcb_bottom - _pcb_top) / pix_unit); }
    
    
    void _draw_segment(cv::Mat& img, const pcb::segment& s, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    void _draw_via(cv::Mat& img, const pcb::via& v, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    void _draw_zone(cv::Mat& img, const pcb::zone& z, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    void _draw_gr(cv::Mat& img, const pcb::gr& gr, pcb::point at, float angle, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    void _draw_fp(cv::Mat& img, const pcb::footprint& fp, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    void _draw_pad(cv::Mat& img, const pcb::footprint& fp, const pcb::pad& p, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit);
    
    
    void _clean_segment(std::uint32_t net_id);
private:
    std::map<std::uint32_t, std::string> _nets;
    std::multimap<std::uint32_t, segment> _segments;
    std::multimap<std::uint32_t, via> _vias;
    std::multimap<std::uint32_t, pad> _pads;
    std::multimap<std::uint32_t, zone> _zones;
    
    std::vector<gr> _grs;
    std::vector<layer> _layers;
    std::vector<footprint> _footprints;
    
    
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
    
    bool _ignore_cu_thickness;
    
    /* 仅仅是坐标精度 */
    const float _float_epsilon;
};

#endif
