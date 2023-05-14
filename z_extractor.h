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

#ifndef __Z_EXTRACTOR_H__
#define __Z_EXTRACTOR_H__
#include <cstdint>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <math.h>
#include <opencv2/opencv.hpp>
#include "Z0_calc.h"
#include "pcb.h"

class fasthenry;
class z_extractor
{
public:
    struct pcb_point
    {
        pcb_point(): x(0), y(0) { }
        float x;
        float y;
        bool operator < (const pcb_point& p) const {
            return x < p.x || (!(p.x < x) && y < p.y);
        }
    };

    struct zone
    {
        std::list<pcb_point> pts;
        std::string layer_name;
        std::uint32_t net;
        std::string tstamp;
    };
    
    
    struct via
    {
        via(): size(0), drill(0), net(0) {}
        pcb_point at;
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
        pad()
            : type(0), net(0), ref_at_angle(0), at_angle(0)
            , size_w(0)
            , size_h(0)
            {}
        std::string footprint;
        std::string pad_number;
        std::uint32_t type;
        std::uint32_t net;
        std::string net_name;
        
        pcb_point ref_at;
        float ref_at_angle;
        
        pcb_point at;
        float at_angle;
        float size_w;
        float size_h;
        std::list<std::string> layers;
        std::string tstamp;
    };
    
    struct layer
    {
        layer(): thickness(0), epsilon_r(0) {}
        
        std::string name;
        std::string type;
        float thickness;
        float epsilon_r;
    };
    
    struct cond
    {
        cond(): w(0), h(0) {}
        z_extractor::pcb_point start;
        z_extractor::pcb_point end;
        float w;
        float h;
    };
public:
    z_extractor(std::shared_ptr<pcb>& pcb);
    ~z_extractor();
    
public:
    bool gen_subckt_rl(const std::string& footprint1, const std::string& footprint1_pad_number,
                        const std::string& footprint2, const std::string& footprint2_pad_number,
                        std::string& ckt, std::string& call, float& r, float& l);
    bool gen_subckt(std::uint32_t net_id, std::string& ckt, std::set<std::string>& footprint, std::string& call);
    
    /*bool gen_subckt(std::vector<std::uint32_t> net_ids, std::vector<std::set<std::string> > mutual_ind_tstamp,
            std::string& ckt, std::set<std::string>& footprint, std::string& call);*/
    
    bool gen_subckt_zo(std::uint32_t net_id, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& footprint, std::string& call, float& Z0_avg, float& td_sum, float& velocity_avg);
                        
    bool gen_subckt_coupled_tl(std::uint32_t net_id0, std::uint32_t net_id1, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& footprint, std::string& call,
                        float Z0_avg[2], float td_sum[2], float velocity_avg[2], float& Zodd_avg, float& Zeven_avg);
    
    std::string gen_zone_fasthenry(std::uint32_t net_id, std::set<z_extractor::pcb_point>& points);
    
    
    void set_freq(float freq) { _freq = freq; }
    void set_calc(std::uint32_t type = Z0_calc::Z0_CALC_MMTL);
    void set_step(float step) { _Z0_step = step; }
    void set_coupled_max_gap(float dist) { _coupled_max_gap = dist; }
    void set_coupled_min_len(float len) { _coupled_min_len = len; }
    void set_conductivity(float conductivity) { _conductivity = conductivity; }
    void enable_lossless_tl(bool b) { _lossless_tl = b; }
    void enable_ltra_model(bool b) { _ltra_model = b; }
    void enable_via_tl_mode(bool b) { _via_tl_mode = b; }
    void enable_openmp(bool b) { _enable_openmp = b; }
    
    
    static std::string format_net_name(const std::string& net_name) { return _format_net_name(net_name); }
    static std::string gen_pad_net_name(const std::string& footprint, const std::string& net_name)
    {
        return _gen_pad_net_name(footprint, net_name);
    }
    
private:
    std::string _get_tstamp_short(const std::string& tstamp);
    static std::string _format_net(const std::string& name);
    std::string _pos2net(float x, float y, const std::string& layer);
    static std::string _format_net_name(const std::string& net_name);
    std::string _format_layer_name(std::string layer_name);
    static std::string _gen_pad_net_name(const std::string& footprint, const std::string& net_name);
    
    
    
    void _get_zone_cond(std::uint32_t net_id, const std::map<std::string, cv::Mat>& zone_mat, std::map<std::string, std::list<cond> >& conds, float& grid_size);
    //void _get_zone_cond(const z_extractor::zone& z, std::list<cond>& conds, std::set<z_extractor::pcb_point>& points);
    void _add_zone(fasthenry& henry, std::uint32_t net_id, const std::map<std::string, cv::Mat>& zone_mat, std::map<std::string, std::list<cond> >& conds, float& grid_size);
    void _conn_to_zone(fasthenry& henry, float x, float y, std::map<std::string, cv::Mat>& zone_mat, const std::string& layer_name, std::map<std::string, std::list<cond> >& conds, float grid_size);
    
    void _draw_segment(cv::Mat& img, pcb::segment& s, std::uint8_t b, std::uint8_t g, std::uint8_t r);
    
    void _create_refs_mat(std::vector<std::uint32_t> refs_id, std::map<std::string, cv::Mat>& refs_mat, bool use_segment = true, bool clean_segment = false);
    
    /* 提取走线附近的参考平面横界面参数 */
    std::list<std::pair<float, float> > _get_mat_line(const cv::Mat& img, float x1, float y1, float x2, float y2);
    
    /* 获取走线在offset位置处的参考平面的截面 */
    std::list<std::pair<float/*中心点*/, float/*宽度*/> > _get_segment_ref_plane(const pcb::segment& s, const cv::Mat& ref, float offset, float w);
    
    /* 获取过孔反焊盘直径 */
    float _get_via_anti_pad_diameter(const pcb::via& v, const std::map<std::string, cv::Mat>& refs_mat, std::string layer);
    
    
    
    
    /* 单位 欧 */
    float _calc_segment_r(const pcb::segment& s);
    /* 单位 nH */
    float _calc_segment_l(const pcb::segment& s);
    /* 单位 nH */
    float _calc_via_l(const via& s, const std::string& layer_name1, const std::string& layer_name2);
    
    
    bool _is_coupled(const pcb::segment& s1, const pcb::segment& s2, float coupled_max_gap, float coupled_min_len);
    void _split_segment(const pcb::segment& s, std::list<pcb::segment>& ss, float x1, float y1, float x2, float y2);
    
    std::string _gen_segment_Z0_ckt_openmp(const std::string& cir_name, pcb::segment& s, const std::map<std::string, cv::Mat>& refs_mat,
                                            std::vector<std::pair<float, float> >& v_Z0_td);
    std::string _gen_segment_coupled_Z0_ckt_openmp(const std::string& cir_name, pcb::segment& s0, pcb::segment& s1, const std::map<std::string, cv::Mat>& refs_mat,
                                                    std::vector<std::pair<float, float> > v_Z0_td[2],
                                                    std::vector<std::pair<float, float> >& v_Zodd_td,
                                                    std::vector<std::pair<float, float> >& v_Zeven_td);
    
    std::string _gen_via_Z0_ckt(pcb::via& v, std::map<std::string, cv::Mat>& refs_mat, const std::vector<std::uint32_t>& refs_id, std::string& call, float& td);
    std::string _gen_via_model_ckt(pcb::via& v, std::map<std::string, cv::Mat>& refs_mat, std::string& call, float& td);
    
    
    float _cvt_img_x(float x) { return round((x - _pcb->get_edge_left()) * _img_ratio); }
    float _cvt_img_y(float y) { return round((y - _pcb->get_edge_top()) * _img_ratio); }
    float _cvt_img_len(float len) { return round(len * _img_ratio); }
    float _cvt_pcb_x(float x) { return x / _img_ratio + _pcb->get_edge_left(); }
    float _cvt_pcb_y(float y) { return y / _img_ratio + _pcb->get_edge_top(); }
    float _cvt_pcb_len(float len) { return len / _img_ratio; }
    float _get_pcb_img_cols() { return round((_pcb->get_edge_right() - _pcb->get_edge_left()) * _img_ratio); }
    float _get_pcb_img_rows() { return round((_pcb->get_edge_bottom() - _pcb->get_edge_top()) * _img_ratio); }
    
private:
    float _Z0_step;
    float _Z0_w_ratio;
    float _Z0_h_ratio;
    float _coupled_max_gap;
    float _coupled_min_len;
    
    /* 无损传输线 */
    bool _lossless_tl;
    bool _ltra_model;
    bool _via_tl_mode;
    bool _enable_openmp;
    
    float _img_ratio;
    
    //std::shared_ptr<Z0_calc> _Z0_calc;
    
    std::vector<std::shared_ptr<Z0_calc> > _Z0_calc;
    
    const float _resistivity = 0.0172;
    /* 小于这个长度的走线不计算阻抗 使用0欧电阻连接 */
    const float _segment_min_len = 0.01;
    /* 如果计算得到的阻抗差小于于该值 则认为阻抗没有变化*/
    const float _Z0_threshold = 0.5;
    /* td小于该值的传输线 只导出无损模型 */
    const float _td_threshold = 0.001;
    /* 仅仅是坐标精度 */
    const float _float_epsilon = 0.00005;
    float _conductivity;
    float _freq;
    
    std::shared_ptr<pcb> _pcb;
};

#endif