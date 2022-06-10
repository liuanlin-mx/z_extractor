#ifndef __KICAD_PCB_H__
#define __KICAD_PCB_H__
#include <cstdint>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <set>
#include <math.h>
#include <opencv2/opencv.hpp>
#include "Z0_calc.h"

class kicad_pcb_sim
{
public:
    struct pcb_point
    {
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
    
    struct segment
    {
        pcb_point start;
        pcb_point end;
        float width;
        std::string layer_name;
        std::uint32_t net;
        std::string tstamp;
    };
    
    struct via
    {
        pcb_point at;
        float size;
        float drill;
        std::list<std::string> layers;
        std::uint32_t net;
        std::string tstamp;
    };
    
    struct net
    {
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
        std::string reference_value;
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
        std::string name;
        std::string type;
        float thickness;
        float epsilon_r = 0;
    };
    
    struct cond
    {
        kicad_pcb_sim::pcb_point start;
        kicad_pcb_sim::pcb_point end;
        float w;
        float h;
    };
public:
    kicad_pcb_sim();
    ~kicad_pcb_sim();
    
public:
    bool parse(const char *str);
    std::list<segment> get_segments(std::uint32_t net_id);
    std::list<pad> get_pads(std::uint32_t net_id);
    std::list<via> get_vias(std::uint32_t net_id);
    std::list<zone> get_zones(std::uint32_t net_id);
    std::vector<std::list<kicad_pcb_sim::segment> > get_segments_sort(std::uint32_t net_id);
    std::string get_net_name(std::uint32_t net_id);
    std::uint32_t get_net_id(std::string name);
    bool gen_subckt(std::uint32_t net_id, std::string& ckt, std::set<std::string>& reference_value, std::string& call);
    
    bool gen_subckt(std::vector<std::uint32_t> net_ids, std::vector<std::set<std::string> > mutual_ind_tstamp,
            std::string& ckt, std::set<std::string>& reference_value, std::string& call);
    
    bool gen_subckt_zo(std::uint32_t net_id, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value, std::string& call, float& td_sum);
                        
    bool gen_subckt_coupled_tl(std::uint32_t net_id0, std::uint32_t net_id1, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value, std::string& call);
    void set_coupled_max_d(float dist) { _coupled_max_d = dist; }
    void set_coupled_min_len(float len) { _coupled_min_len = len; }
    void set_conductivity(float conductivity) { _conductivity = conductivity; } 
    void enable_lossless_tl(bool b) { _lossless_tl = b; }
    void enable_ltra_model(bool b) { _ltra_model = b; }
    
    std::string gen_zone_fasthenry(std::uint32_t net_id, std::set<kicad_pcb_sim::pcb_point>& points);
    void dump();
    
    static std::string format_net_name(const std::string& net_name) { return _format_net_name(net_name); }
    static std::string gen_pad_net_name(const std::string& reference_value, const std::string& net_name)
    {
        return _gen_pad_net_name(reference_value, net_name);
    }
    
private:

    const char *_parse_label(const char *str, std::string& label);
    const char *_skip(const char *str);
    const char *_parse_zone(const char *str, zone& z);
    const char *_parse_filled_polygon(const char *str, zone& z);
    const char *_parse_net(const char *str, std::uint32_t& id, std::string& name);
    const char *_parse_segment(const char *str, segment& s);
    const char *_parse_via(const char *str, via& v);
    const char *_parse_number(const char *str, float &num);
    const char *_parse_string(const char *str, std::string& text);
    const char *_parse_postion(const char *str, float &x, float& y);
    const char *_parse_tstamp(const char *str, std::string& tstamp);
    const char *_parse_layers(const char *str, std::list<std::string>& layers);
    const char *_parse_footprint(const char *str);
    const char *_parse_at(const char *str, float &x, float& y, float& angle);
    const char *_parse_reference(const char *str, std::string& footprint_name);
    const char *_parse_pad(const char *str, pad& v);
    
    const char *_parse_setup(const char *str);
    const char *_parse_stackup(const char *str);
    const char *_parse_stackup_layer(const char *str);
    const char *_parse_edge(const char *str);
    
    
    void _get_pad_pos(pad& p, float& x, float& y);
    std::string _get_tstamp_short(const std::string& tstamp);
    std::string _pos2net(float x, float y, const std::string& layer);
    static std::string _format_net_name(const std::string& net_name);
    std::string _format_layer_name(std::string layer_name);
    static std::string _gen_pad_net_name(const std::string& reference_value, const std::string& net_name);
    
    std::vector<std::string> _get_all_cu_layer();
    std::vector<std::string> _get_all_dielectric_layer();
    std::vector<std::string> _get_via_layers(const via& v);
    
    float _get_layer_distance(const std::string& layer_name1, const std::string& layer_name2);
    float _get_layer_thickness(const std::string& layer_name);
    float _get_layer_z_axis(const std::string& layer_name);
    float _get_layer_epsilon_r(const std::string& layer_name);
    float _get_layer_epsilon_r(const std::string& layer_start, const std::string& layer_end);
    float _get_board_thickness();
    float _get_cu_min_thickness();
    
    
    bool _float_equal(float a, float b);
    bool _point_equal(float x1, float y1, float x2, float y2);
    
    /* 找到下一个连接到(x, y)的走线 */
    bool _segments_get_next(std::list<segment>& segments, kicad_pcb_sim::segment& s, float x, float y, const std::string& layer_name);
    
    
    /* 单位 欧 */
    float _calc_segment_r(const segment& s);
    /* 单位 nH */
    float _calc_segment_l(const segment& s);
    /* 单位 nH */
    float _calc_via_l(const via& s, const std::string& layer_name1, const std::string& layer_name2);
    
    void _get_zone_cond(const kicad_pcb_sim::zone& z, std::list<cond>& conds, std::set<kicad_pcb_sim::pcb_point>& points);
    
    void _create_refs_mat(std::vector<std::uint32_t> refs_id, std::map<std::string, cv::Mat>& refs_mat);
    
    /* 提取走线附近的参考平面横界面参数 */
    std::list<std::pair<float, float> > _get_mat_line(const cv::Mat& img, float x1, float y1, float x2, float y2);
    std::string _gen_segment_zo_ckt(const std::string& cir_name, kicad_pcb_sim::segment& s, std::map<std::string, cv::Mat>& refs_mat, float& td);
    std::string _gen_segment_zo_ckt_omp(const std::string& cir_name, kicad_pcb_sim::segment& s, std::map<std::string, cv::Mat>& refs_mat);
    std::string _gen_segment_coupled_zo_ckt(const std::string& cir_name, kicad_pcb_sim::segment& s0, kicad_pcb_sim::segment& s1, std::map<std::string, cv::Mat>& refs_mat);
    
    std::string _gen_segment_zo_ckt(const std::string& cir_name, kicad_pcb_sim::via& v, const std::string& start, const std::string& end, std::map<std::string, cv::Mat>& refs_mat, float& td);
    
    /* 计算两点连线倾斜角 */
    float _calc_angle(float x1, float y1, float x2, float y2)
    {
        return atan2(y2 - y1, x2 - x1);
    }

    float _calc_dist(float x1, float y1, float x2, float y2)
    {
        return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    }
    
    /* ACB 计算点C为中心点的夹角 */
    float _calc_angle(float ax, float ay, float bx, float by, float cx, float cy);
    /* 计算点到线段或其延长线的垂直距离 */
    float _calc_p2line_dist(float x1, float y1, float x2, float y2, float x, float y);
    
    /* 计算过点(x y)到线段的垂线跟线段的交点 */
    bool _calc_p2line_intersection(float x1, float y1, float x2, float y2, float x, float y, float& ix, float& iy);
    
    /* 计算两条平行线段的交叠区域 */
    bool _calc_parallel_lines_overlap(float ax1, float ay1, float ax2, float ay2,
                                        float bx1, float by1, float bx2, float by2,
                                        float& aox1, float& aoy1, float& aox2, float& aoy2,
                                        float& box1, float& boy1, float& box2, float& boy2);
                           
    /* 计算两条平行线段的交叠区域长度 */             
    float _calc_parallel_lines_overlap_len(float ax1, float ay1, float ax2, float ay2,
                                        float bx1, float by1, float bx2, float by2);
                                        
    bool _is_coupled(const kicad_pcb_sim::segment& s1, const kicad_pcb_sim::segment& s2, float coupled_max_d, float coupled_min_len);
    void _split_segment(const kicad_pcb_sim::segment& s, std::list<kicad_pcb_sim::segment>& ss, float x1, float y1, float x2, float y2);
    
    float _cvt_img_x(float x) { return round((x - _pcb_left) * _img_ratio); }
    float _cvt_img_y(float y) { return round((y - _pcb_top) * _img_ratio); }
    float _cvt_img_len(float len) { return round(len * _img_ratio); }
    float _get_pcb_img_cols() { return round((_pcb_right - _pcb_left) * _img_ratio); }
    float _get_pcb_img_rows() { return round((_pcb_bottom - _pcb_top) * _img_ratio); }
    
private:
    std::map<std::uint32_t, std::string> _nets;
    std::multimap<std::uint32_t, segment> _segments;
    std::multimap<std::uint32_t, via> _vias;
    std::multimap<std::uint32_t, pad> _pads;
    std::multimap<std::uint32_t, zone> _zones;
    
    std::vector<layer> _layers;
    
    float _Z0_setup;
    float _Z0_w_ratio;
    float _Z0_h_ratio;
    float _coupled_max_d;
    float _coupled_min_len;
    
    /* 无损传输线 */
    bool _lossless_tl;
    bool _ltra_model;
    
    float _img_ratio;
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
    std::shared_ptr<Z0_calc> _Z0_calc;
    //std::shared_ptr<Z0_calc> _Z0_calc1;
    //std::shared_ptr<Z0_calc> _Z0_calc_coupled;
    const float _resistivity = 0.0172;
    float _conductivity;
    
};

#endif