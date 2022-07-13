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
    
    struct segment
    {
        segment(): width(0), net(0) {}
        bool is_arc() const { return mid.x != 0 || mid.y != 0; }
        
        pcb_point start;
        pcb_point mid;
        pcb_point end;
        float width;
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
    z_extractor();
    ~z_extractor();
    
public:
    bool add_net(std::uint32_t id, std::string name);
    bool add_segment(const segment& s);
    bool add_via(const via& v);
    bool add_zone(const zone& z);
    bool add_pad(const pad& p);
    bool add_layers(const layer& l);
    void set_edge(float top, float bottom, float left, float right);
    
    std::list<segment> get_segments(std::uint32_t net_id);
    std::list<pad> get_pads(std::uint32_t net_id);
    std::list<via> get_vias(std::uint32_t net_id);
    std::list<zone> get_zones(std::uint32_t net_id);
    std::vector<std::list<z_extractor::segment> > get_segments_sort(std::uint32_t net_id);
    std::string get_net_name(std::uint32_t net_id);
    std::uint32_t get_net_id(std::string name);
    
    
    bool gen_subckt(std::uint32_t net_id, std::string& ckt, std::set<std::string>& reference_value, std::string& call);
    
    bool gen_subckt(std::vector<std::uint32_t> net_ids, std::vector<std::set<std::string> > mutual_ind_tstamp,
            std::string& ckt, std::set<std::string>& reference_value, std::string& call);
    
    bool gen_subckt_zo(std::uint32_t net_id, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value, std::string& call, float& Z0_avg, float& td_sum, float& velocity_avg);
                        
    bool gen_subckt_coupled_tl(std::uint32_t net_id0, std::uint32_t net_id1, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value, std::string& call,
                        float Z0_avg[2], float td_sum[2], float velocity_avg[2], float& Zodd_avg, float& Zeven_avg);
    
    
    std::string gen_zone_fasthenry(std::uint32_t net_id, std::set<z_extractor::pcb_point>& points);
    
    
    void set_calc(std::uint32_t type = Z0_calc::Z0_CALC_MMTL);
    void set_step(float step) { _Z0_step = step; }
    void set_coupled_max_d(float dist) { _coupled_max_d = dist; }
    void set_coupled_min_len(float len) { _coupled_min_len = len; }
    void set_conductivity(float conductivity) { _conductivity = conductivity; }
    void enable_lossless_tl(bool b) { _lossless_tl = b; }
    void enable_ltra_model(bool b) { _ltra_model = b; }
    void enable_via_tl_mode(bool b) { _via_tl_mode = b; }
    void enable_openmp(bool b) { _enable_openmp = b; }
    void dump();
    
    static std::string format_net_name(const std::string& net_name) { return _format_net_name(net_name); }
    static std::string gen_pad_net_name(const std::string& reference_value, const std::string& net_name)
    {
        return _gen_pad_net_name(reference_value, net_name);
    }
    
private:
    void _get_pad_pos(const pad& p, float& x, float& y);
    std::string _get_tstamp_short(const std::string& tstamp);
    static std::string _format_net(const std::string& name);
    std::string _pos2net(float x, float y, const std::string& layer);
    static std::string _format_net_name(const std::string& net_name);
    std::string _format_layer_name(std::string layer_name);
    static std::string _gen_pad_net_name(const std::string& reference_value, const std::string& net_name);
    
    std::vector<std::string> _get_all_cu_layer();
    std::vector<std::string> _get_all_dielectric_layer();
    std::vector<std::string> _get_all_mask_layer();
    std::vector<std::string> _get_via_layers(const via& v);
    std::vector<std::string> _get_via_conn_layers(const via& v);
    float _get_via_conn_len(const z_extractor::via& v);
    
    std::vector<std::string> _get_pad_conn_layers(const pad& p);
    
    float _get_layer_distance(const std::string& layer_name1, const std::string& layer_name2);
    float _get_layer_thickness(const std::string& layer_name);
    float _get_layer_z_axis(const std::string& layer_name);
    float _get_layer_epsilon_r(const std::string& layer_name);
    float _get_cu_layer_epsilon_r(const std::string& layer_name);
    float _get_layer_epsilon_r(const std::string& layer_start, const std::string& layer_end);
    float _get_board_thickness();
    float _get_cu_min_thickness();
    
    
    bool _float_equal(float a, float b);
    bool _point_equal(float x1, float y1, float x2, float y2);
    
    /* 找到下一个连接到(x, y)的走线 */
    bool _segments_get_next(std::list<segment>& segments, z_extractor::segment& s, float x, float y, const std::string& layer_name);
    /* 检测是否有未连接的走线 */
    bool _check_segments(std::uint32_t net_id);
    
    /* 单位 欧 */
    float _calc_segment_r(const segment& s);
    /* 单位 nH */
    float _calc_segment_l(const segment& s);
    /* 单位 nH */
    float _calc_via_l(const via& s, const std::string& layer_name1, const std::string& layer_name2);
    
    void _get_zone_cond(const z_extractor::zone& z, std::list<cond>& conds, std::set<z_extractor::pcb_point>& points);
    
    void _create_refs_mat(std::vector<std::uint32_t> refs_id, std::map<std::string, cv::Mat>& refs_mat);
    void _draw_segment(cv::Mat& img, z_extractor::segment& s, std::uint8_t b, std::uint8_t g, std::uint8_t r);
    
    /* 提取走线附近的参考平面横界面参数 */
    std::list<std::pair<float, float> > _get_mat_line(const cv::Mat& img, float x1, float y1, float x2, float y2);
    std::string _gen_segment_Z0_ckt_openmp(const std::string& cir_name, z_extractor::segment& s, const std::map<std::string, cv::Mat>& refs_mat,
                                            std::vector<std::pair<float, float> >& v_Z0_td);
    std::string _gen_segment_coupled_Z0_ckt_openmp(const std::string& cir_name, z_extractor::segment& s0, z_extractor::segment& s1, const std::map<std::string, cv::Mat>& refs_mat,
                                                    std::vector<std::pair<float, float> > v_Z0_td[2],
                                                    std::vector<std::pair<float, float> >& v_Zodd_td,
                                                    std::vector<std::pair<float, float> >& v_Zeven_td);
    
    std::string _gen_via_Z0_ckt(z_extractor::via& v, std::map<std::string, cv::Mat>& refs_mat, std::string& call, float& td);
    std::string _gen_via_model_ckt(z_extractor::via& v, std::map<std::string, cv::Mat>& refs_mat, std::string& call, float& td);
    
    
    /* 获取走线长度 */
    float _get_segment_len(const z_extractor::segment& s);
    /* 获取从起点向终点前进指定offset后的坐标 */
    void _get_segment_pos(const z_extractor::segment& s, float offset, float& x, float& y);
    
    /* 获取过从起点向终点前进指定offset后的点且垂直与走线的一条线段 长度为 w 线段中点与走线相交 */
    void _get_segment_perpendicular(const z_extractor::segment& s, float offset, float w, float& x_left, float& y_left, float& x_right, float& y_right);
    
    /* 获取走线在offset位置处的参考平面的截面 */
    std::list<std::pair<float/*中心点*/, float/*宽度*/> > _get_segment_ref_plane(const z_extractor::segment& s, const cv::Mat& ref, float offset, float w);
    
    /* 获取过孔反焊盘直径 */
    float _get_via_anti_pad_diameter(const z_extractor::via& v, const std::map<std::string, cv::Mat>& refs_mat, std::string layer);
    
    bool _is_coupled(const z_extractor::segment& s1, const z_extractor::segment& s2, float coupled_max_d, float coupled_min_len);
    void _split_segment(const z_extractor::segment& s, std::list<z_extractor::segment>& ss, float x1, float y1, float x2, float y2);
    
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
    
    float _Z0_step;
    float _Z0_w_ratio;
    float _Z0_h_ratio;
    float _coupled_max_d;
    float _coupled_min_len;
    
    /* 无损传输线 */
    bool _lossless_tl;
    bool _ltra_model;
    bool _via_tl_mode;
    bool _enable_openmp;
    
    float _img_ratio;
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
    
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
};

#endif