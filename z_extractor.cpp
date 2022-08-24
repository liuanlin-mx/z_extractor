#include <float.h>
#include <math.h>
#include <omp.h>
#include "z_extractor.h"
#include <opencv2/opencv.hpp>
#include "fasthenry.h"
#include "atlc.h"
#include "Z0_calc.h"
#include "calc.h"

#if 0
#define log_debug(fmt, args...) printf(fmt, ##args)
#else
#define log_debug(fmt, args...)
#endif

#define log_info(fmt, args...) printf(fmt, ##args)

#define DBG_IMG 0
z_extractor::z_extractor()
{
    _Z0_step = 0.5;
    _Z0_w_ratio = 10;
    _Z0_h_ratio = 100;
    
    _coupled_max_gap = 2;
    _coupled_min_len = 0.2;
    _lossless_tl = true;
    _ltra_model = true;
    _via_tl_mode = false;
    _enable_openmp = true;
    
    _img_ratio = 1 / (0.0254 * 0.5);
    _pcb_top = 10000.;
    _pcb_bottom = 0;
    _pcb_left = 10000.;
    _pcb_right = 0;
    
    _conductivity = 5.0e7;
    
    
    std::int32_t thread_nums = omp_get_max_threads();
    for (std::int32_t i = 0; i < std::max(1, thread_nums); i++)
    {
        std::shared_ptr<Z0_calc> calc = Z0_calc::create(Z0_calc::Z0_CALC_ATLC);
        char name[32];
        sprintf(name, "mmtl_tmp%d", i);
        calc->set_tmp_name(name);
        _Z0_calc.push_back(calc);
    }
}

z_extractor::~z_extractor()
{
}


bool z_extractor::add_net(std::uint32_t id, std::string name)
{
    _nets.emplace(id, name);
    return 0;
}


bool z_extractor::add_segment(const segment& s)
{
    _segments.emplace(s.net, s);
    return 0;
}


bool z_extractor::add_via(const via& v)
{
    _vias.emplace(v.net, v);
    return 0;
}


bool z_extractor::add_zone(const zone& z)
{
    _zones.emplace(z.net, z);
    return 0;
}


bool z_extractor::add_pad(const pad& p)
{
    _pads.emplace(p.net, p);
    return 0;
}


bool z_extractor::add_layers(const layer& l)
{
    _layers.push_back(l);
    return 0;
}


void z_extractor::set_edge(float top, float bottom, float left, float right)
{
    _pcb_top = top;
    _pcb_bottom = bottom;
    _pcb_left = left;
    _pcb_right = right;
}
    

std::list<z_extractor::segment> z_extractor::get_segments(std::uint32_t net_id)
{
    std::list<segment> segments;
    auto v = _segments.equal_range(net_id);
    if(v.first != std::end(_segments))
    {
        for (auto it = v.first; it != v.second; ++it)
        {
            segments.push_back(it->second);
        }
    }
    return segments;
}


std::list<z_extractor::pad> z_extractor::get_pads(std::uint32_t net_id)
{
    std::list<pad> pads;
    auto p = _pads.equal_range(net_id);
    if(p.first != std::end(_pads))
    {
        for (auto it = p.first; it != p.second; ++it)
        {
            pads.push_back(it->second);
        }
    }
    return pads;
}


bool z_extractor::get_pad(const std::string& footprint, const std::string& pad_number, z_extractor::pad& pad)
{
    for (const auto& pad_: _pads)
    {
        if (pad_.second.footprint == footprint && pad_.second.pad_number == pad_number)
        {
            pad = pad_.second;
            return true;
        }
    }
    return false;
}


std::list<z_extractor::via> z_extractor::get_vias(std::uint32_t net_id)
{
    std::list<via> vias;
    auto v = _vias.equal_range(net_id);
    if(v.first != std::end(_vias))
    {
        for (auto it = v.first; it != v.second; ++it)
        {
            vias.push_back(it->second);
        }
    }
    return vias;
}


std::list<z_extractor::zone> z_extractor::get_zones(std::uint32_t net_id)
{
    std::list<zone> zones;
    auto z = _zones.equal_range(net_id);
    if(z.first != std::end(_zones))
    {
        for (auto it = z.first; it != z.second; ++it)
        {
            zones.push_back(it->second);
        }
    }
    return zones;
}


std::vector<std::list<z_extractor::segment> > z_extractor::get_segments_sort(std::uint32_t net_id)
{
    std::vector<std::list<z_extractor::segment> > v_segments;
    std::list<z_extractor::segment> segments = get_segments(net_id);
    while (segments.size())
    {
        std::list<z_extractor::segment> s_list;
        z_extractor::segment first = segments.front();
        segments.pop_front();
        s_list.push_back(first);
        z_extractor::segment tmp = first;
        z_extractor::segment next;
        
        while (_segments_get_next(segments, next, tmp.start.x, tmp.start.y, tmp.layer_name))
        {
            if (_point_equal(next.start.x, next.start.y, tmp.start.x, tmp.start.y))
            {
                std::swap(next.start, next.end);
            }
            tmp = next;
            s_list.push_front(next);
        }
        
        tmp = first;
        
        while (_segments_get_next(segments, next, tmp.end.x, tmp.end.y, tmp.layer_name))
        {
            if (_point_equal(next.end.x, next.end.y, tmp.end.x, tmp.end.y))
            {
                std::swap(next.start, next.end);
            }
            tmp = next;
            s_list.push_back(next);
        }
        
        v_segments.push_back(s_list);
    }
    
    for (auto& s_list : v_segments)
    {
        for (auto i : s_list)
        {
            log_debug("--- start: x:%f y:%f end: x:%f y:%f\n", i.start.x, i.start.y, i.end.x, i.end.y);
        }
        log_debug("--------------------------------\n");
    }
    return v_segments;
}

std::string z_extractor::get_net_name(std::uint32_t net_id)
{
    if (_nets.count(net_id))
    {
        return _nets[net_id];
    }
    return "";
}

std::uint32_t z_extractor::get_net_id(std::string name)
{
    for (const auto& net: _nets)
    {
        if (net.second == name)
        {
            return net.first;
        }
    }
    return 0;
}


bool z_extractor::gen_subckt_rl(const std::string& footprint1, const std::string& footprint1_pad_number,
                        const std::string& footprint2, const std::string& footprint2_pad_number,
                        std::string& ckt, std::string& call, float& r, float& l)
{
    pad pad1;
    pad pad2;
    if (!get_pad(footprint1, footprint1_pad_number, pad1))
    {
        printf("not found %s.%s\n", footprint1.c_str(), footprint1_pad_number.c_str());
        return false;
    }
    
    if (!get_pad(footprint2, footprint2_pad_number, pad2))
    {
        printf("not found %s.%s\n", footprint2.c_str(), footprint2_pad_number.c_str());
        return false;
    }
    
    if (pad1.net != pad2.net)
    {
        printf("err: %s.%s %s.%s not on the same network\n", footprint1.c_str(), footprint1_pad_number.c_str(),
                                    footprint2.c_str(), footprint2_pad_number.c_str());
        return false;
    }
    std::uint32_t net_id = pad1.net;
    
    
    std::list<pad> pads = get_pads(net_id);
    std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
    std::list<via> vias = get_vias(net_id);
    
    /* 构建fasthenry */
    fasthenry henry;
    henry.set_conductivity(_conductivity);
    std::map<std::string, cv::Mat> zone_mat;
    std::map<std::string, std::list<cond> > conds;
    bool have_zones = !get_zones(net_id).empty();
    float grid_size = 1;
    if (have_zones)
    {
        _create_refs_mat({net_id}, zone_mat, false);
        _add_zone(henry, net_id, zone_mat, conds, grid_size);
    }
    
    for (auto& s_list: v_segments)
    {
        float z_val = _get_layer_z_axis(s_list.front().layer_name);
        float h_val = _get_layer_thickness(s_list.front().layer_name);
        
        for (auto& s: s_list)
        { 
            henry.add_wire(_pos2net(s.start.x, s.start.y, s.layer_name), _pos2net(s.end.x, s.end.y, s.layer_name),
                                _get_tstamp_short(s.tstamp),
                                fasthenry::point(s.start.x, s.start.y, z_val),
                                fasthenry::point(s.end.x, s.end.y, z_val), s.width, h_val);
            if (have_zones)
            {
                _conn_to_zone(henry, s.start.x, s.start.y, zone_mat, s.layer_name, conds, grid_size);
                _conn_to_zone(henry, s.end.x, s.end.y, zone_mat, s.layer_name, conds, grid_size);
            }
        }
    }
    
    for (auto& v: vias)
    {
        std::vector<std::string> layers = _get_via_layers(v);
        for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
        {
            const std::string& start = layers[i];
            const std::string& end = layers[i + 1];
            
            float z1 = _get_layer_z_axis(start);
            float z2 = _get_layer_z_axis(end);
            
            henry.add_via(_pos2net(v.at.x, v.at.y, start), _pos2net(v.at.x, v.at.y, end),
                                _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                fasthenry::point(v.at.x, v.at.y, z1),
                                fasthenry::point(v.at.x, v.at.y, z2), v.drill, v.size);
            if (have_zones)
            {
                _conn_to_zone(henry, v.at.x, v.at.y, zone_mat, start, conds, grid_size);
                _conn_to_zone(henry, v.at.x, v.at.y, zone_mat, end, conds, grid_size);
            }
        }
    }

    for (const auto& pad: pads)
    {
        float x;
        float y;
        _get_pad_pos(pad, x, y);
        
        std::vector<std::string> layers = _get_pad_layers(pad);
        for (std::uint32_t i = 1; i < layers.size(); i++)
        {
            const std::string& start = layers[i - 1];
            const std::string& end = layers[i];
            float z1 = _get_layer_z_axis(start);
            float z2 = _get_layer_z_axis(end);
            
            henry.add_via(_pos2net(x, y, start), _pos2net(x, y, end),
                                _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                fasthenry::point(x, y, z1),
                                fasthenry::point(x, y, z2), 1, 1);
            
            if (have_zones)
            {
                _conn_to_zone(henry, x, y, zone_mat, start, conds, grid_size);
                _conn_to_zone(henry, x, y, zone_mat, end, conds, grid_size);
            }
        }
    }
    
    //henry.dump();
    
    float x1;
    float y1;
    float x2;
    float y2;
    double r_ = 0;
    double l_ = 0;
        
    std::vector<std::string> layers1 = _get_pad_layers(pad1);
    std::vector<std::string> layers2 = _get_pad_layers(pad2);
    _get_pad_pos(pad1, x1, y1);
    _get_pad_pos(pad2, x2, y2);
    henry.calc_impedance(_pos2net(x1, y1, layers1.front()), _pos2net(x2, y2, layers2.front()), r_, l_);
    
    
    char buf[512];
    std::string comment;
    std::string subckt_name = _format_net_name(_nets[net_id] + "_" +
                            footprint1 + "_" + footprint1_pad_number + "-" + 
                            footprint2 + "_" + footprint2_pad_number) + " ";
    
    std::string ckt_pin1 = footprint1 + "_" + footprint1_pad_number;
    std::string ckt_pin2 = footprint2 + "_" + footprint2_pad_number;
    ckt = ".subckt " + subckt_name + " " + ckt_pin1 + " " + ckt_pin2 + "\n";
    call = "X" + subckt_name + " " + ckt_pin1 + " " + ckt_pin2 + " " + subckt_name + "\n";
    
    comment = "****" + footprint1 + "." + footprint1_pad_number + "    " + footprint2 + "." + footprint2_pad_number +  "*****\n";
    ckt = comment + ckt;
    sprintf(buf, "R1 %s mid %lg\n", ckt_pin1.c_str(), r_);
    ckt += buf;
    sprintf(buf, "L1 mid %s %lg\n", ckt_pin2.c_str(), l_);
    ckt += buf;
    ckt += ".ends\n";
    
    r = r_;
    l = l_;
    return true;
}


bool z_extractor::gen_subckt(std::uint32_t net_id, std::string& ckt, std::set<std::string>& footprint, std::string& call)
{
    std::string comment;
    std::string sub;
    std::string pad_ckt;
    char buf[512] = {0};
    
    if (_check_segments(net_id) == false)
    {
        return false;
    }
    
    std::list<pad> pads = get_pads(net_id);
    std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
    std::list<via> vias = get_vias(net_id);
    
    /* 生成子电路参数和调用 */
    ckt = ".subckt " + _format_net_name(_nets[net_id]) + " ";
    call = "X" + _format_net_name(_nets[net_id]) + " ";
    comment = std::string(ckt.length(), '*');
    for (auto& p: pads)
    {
        float x;
        float y;
        _get_pad_pos(p, x, y);
        std::vector<std::string> layers = _get_pad_conn_layers(p);
        if (layers.size() == 0)
        {
            printf("err: %s.%s no connection.\n", p.footprint.c_str(), p.pad_number.c_str());
            return false;
        }
        sprintf(buf, "%s ", _pos2net(x, y, layers.front()).c_str());

        ckt += buf;
        call += _gen_pad_net_name(p.footprint, _format_net_name(_nets[net_id]));
        call += " ";
        
        comment += p.footprint + ":" + _nets[net_id] + " ";
        footprint.insert(p.footprint);
        
        for (std::uint32_t i = 1; i < layers.size(); i++)
        {
            sprintf(buf, "R%s%d %s %s 0\n", _get_tstamp_short(p.tstamp).c_str(), i,
                    _pos2net(x, y, layers.front()).c_str(), _pos2net(x, y, layers[i]).c_str());
            pad_ckt += buf;
        }
    }
    ckt += "\n";
    call += _format_net_name(_nets[net_id]);
    call += "\n";

    comment += "\n";
    ckt = comment + ckt + pad_ckt;
    
    
    /* 构建fasthenry */
    fasthenry henry;
    henry.set_conductivity(_conductivity);
    
    for (auto& s_list: v_segments)
    {
        float z_val = _get_layer_z_axis(s_list.front().layer_name);
        float h_val = _get_layer_thickness(s_list.front().layer_name);
        
        for (auto& s: s_list)
        { 
            henry.add_wire(_pos2net(s.start.x, s.start.y, s.layer_name), _pos2net(s.end.x, s.end.y, s.layer_name),
                                _get_tstamp_short(s.tstamp),
                                fasthenry::point(s.start.x, s.start.y, z_val),
                                fasthenry::point(s.end.x, s.end.y, z_val), s.width, h_val);
        }
    }
    
    for (auto& v: vias)
    {
        std::vector<std::string> layers = _get_via_layers(v);
        for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
        {
            const std::string& start = layers[i];
            const std::string& end = layers[i + 1];
            
            float z1 = _get_layer_z_axis(start);
            float z2 = _get_layer_z_axis(end);
            
            henry.add_via(_pos2net(v.at.x, v.at.y, start), _pos2net(v.at.x, v.at.y, end),
                                _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                fasthenry::point(v.at.x, v.at.y, z1),
                                fasthenry::point(v.at.x, v.at.y, z2), v.drill, v.drill);
        }
    }

    for (const auto& pad: pads)
    {
        float x;
        float y;
        _get_pad_pos(pad, x, y);
        
        std::vector<std::string> layers = _get_pad_layers(pad);
        for (std::uint32_t i = 1; i < layers.size(); i++)
        {
            const std::string& start = layers[i - 1];
            const std::string& end = layers[i];
            float z1 = _get_layer_z_axis(start);
            float z2 = _get_layer_z_axis(end);
            
            henry.add_via(_pos2net(x, y, start), _pos2net(x, y, end),
                                _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                fasthenry::point(x, y, z1),
                                fasthenry::point(x, y, z2), 1, 1);
        }
    }
    //henry.dump();
    
    /* 生成走线参数 */
    for (auto& s_list: v_segments)
    {
        std::string ckt_net_name;
            
        for (auto& s: s_list)
        {
            double r = 0;
            double l = 0;
            if (henry.calc_impedance(_pos2net(s.start.x, s.start.y, s.layer_name), _pos2net(s.end.x, s.end.y, s.layer_name), r, l))
            {
                sprintf(buf, "R%s %s %s_mid %lg\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str(),
                                    r);
                ckt += buf;
                sprintf(buf, "L%s  %s_mid %s %lg\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    l);
                ckt += buf;
            }
        }
    }
    
    /* 生成过孔参数 */
    for (auto& v: vias)
    {
        std::vector<std::string> layers = _get_via_layers(v);
        for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
        {
            const std::string& start = layers[i];
            const std::string& end = layers[i + 1];
            
            double r = 0;
            double l = 0;
            if (henry.calc_impedance(_pos2net(v.at.x, v.at.y, start), _pos2net(v.at.x, v.at.y, end), r, l))
            {
                sprintf(buf, "Rv%s %s %s_mid %lg\n", _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                    _pos2net(v.at.x, v.at.y, start).c_str(),
                                    _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                    r);
                ckt += buf;
                sprintf(buf, "Lv%s %s_mid %s %lg\n", _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                    _format_net(_get_tstamp_short(v.tstamp) + start + end).c_str(),
                                    _pos2net(v.at.x, v.at.y, end).c_str(),
                                    l);
                ckt += buf;
            }
        }
    }
    
    for (const auto& pad: pads)
    {
        float x;
        float y;
        _get_pad_pos(pad, x, y);
        
        std::vector<std::string> layers = _get_pad_layers(pad);
        for (std::uint32_t i = 1; i < layers.size(); i++)
        {
            const std::string& start = layers[i - 1];
            const std::string& end = layers[i];
            
            double r = 0;
            double l = 0;
            if (henry.calc_impedance(_pos2net(x, y, start), _pos2net(x, y, end), r, l))
            {
                sprintf(buf, "Rp%s %s %s_mid %lg\n", _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                    _pos2net(x, y, start).c_str(),
                                    _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                    r);
                ckt += buf;
                sprintf(buf, "Lp%s %s_mid %s %lg\n", _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                    _format_net(_get_tstamp_short(pad.tstamp) + start + end).c_str(),
                                    _pos2net(x, y, end).c_str(),
                                    l);
                ckt += buf;
            }
        }
    }
        
    ckt += ".ends\n";
    ckt += sub;
    return true;
}
#if 0
bool z_extractor::gen_subckt(std::vector<std::uint32_t> net_ids, std::vector<std::set<std::string> > mutual_ind_tstamp,
            std::string& ckt, std::set<std::string>& footprint, std::string& call)
{
    std::string sub;
    std::string tmp;
    fasthenry henry;
    henry.set_conductivity(_conductivity);
    char buf[512] = {0};
    
    //生成子电路参数和调用代码
    ckt = ".subckt ";
    call = "X";
    for (auto& net_id: net_ids)
    {
        ckt += _format_net_name(_nets[net_id]);
        call += _format_net_name(_nets[net_id]);
        tmp += _format_net_name(_nets[net_id]);
    }
    ckt += " ";
    call += " ";
    
    for (auto& net_id: net_ids)
    {
        std::list<pad> pads = get_pads(net_id);
        
        for (auto& p: pads)
        {
            float x;
            float y;
            _get_pad_pos(p, x, y);
            sprintf(buf, "%s ", _pos2net(x, y, p.layers.front()).c_str());

            ckt += buf;
            call += _gen_pad_net_name(p.footprint, _format_net_name(_nets[net_id]));
            call += " ";
            footprint.insert(p.footprint);
        }
    }
    
    ckt += "\n";
    call += tmp;
    call += "\n";
    
    ckt = call + ckt;
    
    
    //构建fasthenry
    for (auto& net_id: net_ids)
    {
        std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
        
        for (auto& s_list: v_segments)
        {
            float z_val = _get_layer_distance(_layers.front().name, s_list.front().layer_name);
            float h_val = _get_layer_thickness(s_list.front().layer_name);
                
            for (auto& s: s_list)
            {
                henry.add_wire(_get_tstamp_short(s.tstamp).c_str(), fasthenry::point(s.start.x, s.start.y, z_val),
                                    fasthenry::point(s.end.x, s.end.y, z_val), s.width, h_val);
            }
        }
        
        std::list<via> vias = get_vias(net_id);
        for (auto& v: vias)
        {
            std::vector<std::string> layers = _get_via_layers(v);
            for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
            {
                const std::string& start = layers[i];
                const std::string& end = layers[i + 1];
                henry.add_via((_get_tstamp_short(v.tstamp) + _format_layer_name(start) + _format_layer_name(end)).c_str(),
                                fasthenry::point(v.at.x, v.at.y, _get_layer_z_axis(start)),
                                fasthenry::point(v.at.x, v.at.y, _get_layer_z_axis(end)),
                                v.drill, v.size);
            }
        }
    }
    //henry.dump();
    std::set<std::string> tstamp_tmp;
    for (auto& mutual: mutual_ind_tstamp)
    {
        for (auto& tstamp: mutual)
        {
            tstamp_tmp.insert(tstamp);
        }
    }
    
    
    /* 计算互感参数 */
    for (auto mutual: mutual_ind_tstamp)
    {
        std::string ckt_name = "RL";
        std::uint32_t pins = 0;
        std::list<std::string> wire_names;
        std::string call_param;
        for (auto& net_id: net_ids)
        {
            std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
            for (auto& s_list: v_segments)
            {
                for (auto& s: s_list)
                {
                    std::string tstamp = _get_tstamp_short(s.tstamp);
                    if (mutual.count(tstamp))
                    {
                        ckt_name += tstamp;
                        pins += 2;
                        wire_names.push_back(tstamp);
                        
                        sprintf(buf, " %s %s ", _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                            _pos2net(s.end.x, s.end.y, s.layer_name).c_str());
                        call_param += buf;
                    }
                }
            }
        }
        
        sub += henry.gen_ckt2(wire_names, ckt_name);
        ckt += "X" + ckt_name + call_param + ckt_name + "\n";
        
    }
    
    /* 计算走线参数 并生成电路*/
    for (auto& net_id: net_ids)
    {
        std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
        for (auto& s_list: v_segments)
        {
            std::string ckt_net_name;
            for (auto& s: s_list)
            {
                std::string tstamp = _get_tstamp_short(s.tstamp);
                if (tstamp_tmp.count(tstamp))
                {
                    continue;
                }
                
                sub += henry.gen_ckt(_get_tstamp_short(s.tstamp).c_str(), ("RL" + _get_tstamp_short(s.tstamp)).c_str());
                
                sprintf(buf, "X%s %s %s RL%s\n", tstamp.c_str(),
                                        _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                        _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                        tstamp.c_str());
                ckt += buf;
            }
        }
    }
    
    /* 生成过孔参数 */
    for (auto& net_id: net_ids)
    {
        std::list<via> vias = get_vias(net_id);
    
        for (auto& v: vias)
        {
            
            std::string tstamp = _get_tstamp_short(v.tstamp);
            if (tstamp_tmp.count(tstamp))
            {
                continue;
            }
                
            std::vector<std::string> layers = _get_via_layers(v);
            for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
            {
                const std::string& start = layers[i];
                const std::string& end = layers[i + 1];
                std::string name = tstamp + _format_layer_name(start) + _format_layer_name(end);
                sub += henry.gen_ckt(name.c_str(), ("RL" + name).c_str());
                sprintf(buf, "X%s %s %s RL%s\n", name.c_str(),
                                        _pos2net(v.at.x, v.at.y, start).c_str(),
                                        _pos2net(v.at.x, v.at.y, end).c_str(),
                                        name.c_str());
                ckt += buf;
            }
        }
    }
    
    ckt += ".ends\n";
    ckt += sub;
    return true;
}
#endif


bool z_extractor::gen_subckt_zo(std::uint32_t net_id, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& footprint, std::string& call, float& Z0_avg, float& td_sum, float& velocity_avg)
{
    std::string comment;
    std::string sub;
    std::string pad_ckt;
    char buf[512] = {0};
    
    if (_check_segments(net_id) == false)
    {
        return false;
    }
    
    std::list<pad> pads = get_pads(net_id);
    std::vector<std::list<z_extractor::segment> > v_segments = get_segments_sort(net_id);
    std::list<via> vias = get_vias(net_id);
    
    /* 生成子电路参数和调用 */
    ckt = ".subckt " + _format_net_name(_nets[net_id]) + " ";
    call = "X" + _format_net_name(_nets[net_id]) + " ";
    comment = std::string(ckt.length(), '*');
    for (auto& p: pads)
    {
        float x;
        float y;
        _get_pad_pos(p, x, y);
        std::vector<std::string> layers = _get_pad_conn_layers(p);
        if (layers.size() == 0)
        {
            printf("err: %s.%s no connection.\n", p.footprint.c_str(), p.pad_number.c_str());
            return false;
        }
        sprintf(buf, "%s ", _pos2net(x, y, layers.front()).c_str());

        ckt += buf;
        call += _gen_pad_net_name(p.footprint, _format_net_name(_nets[net_id]));
        call += " ";
        
        comment += p.footprint + ":" + _nets[net_id] + " ";
        footprint.insert(p.footprint);
        
        for (std::uint32_t i = 1; i < layers.size(); i++)
        {
            sprintf(buf, "R%s%d %s %s 0\n", _get_tstamp_short(p.tstamp).c_str(), i,
                    _pos2net(x, y, layers.front()).c_str(), _pos2net(x, y, layers[i]).c_str());
            pad_ckt += buf;
        }
    }
    ckt += "\n";
    call += _format_net_name(_nets[net_id]);
    call += "\n";

    comment += "\n";
    ckt = comment + ckt + pad_ckt;
    
    
    
    float len = 0;
    std::vector<std::pair<float, float> > v_Z0_td;
    /* 生成走线参数 */
    std::map<std::string, cv::Mat> refs_mat;
    _create_refs_mat(refs_id, refs_mat);
    for (auto& s_list: v_segments)
    {
        std::string ckt_net_name;
        std::vector<z_extractor::segment> v_list(s_list.begin(), s_list.end());
        
        #pragma omp parallel for
        for (std::uint32_t i = 0; i < v_list.size(); i++)
        {
            char buf[512];
            z_extractor::segment& s = v_list[i];
            std::string tstamp = _get_tstamp_short(s.tstamp);
            std::string subckt;
            std::vector<std::pair<float, float> > v_Z0_td_;
            subckt = _gen_segment_Z0_ckt_openmp(("ZO" + _get_tstamp_short(s.tstamp)).c_str(), s, refs_mat, v_Z0_td_);
            
            sprintf(buf, "X%s %s %s ZO%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
            #pragma omp critical
            {
                sub += subckt;
                ckt += buf;
                len += _get_segment_len(s);
                v_Z0_td.insert(v_Z0_td.end(), v_Z0_td_.begin(), v_Z0_td_.end());
                for (const auto& Z0_td: v_Z0_td_)
                {
                    td_sum += Z0_td.second;
                }
            }
        }
    }
    
    /* 生成过孔参数 */
    for (auto& v: vias)
    {
        std::string via_call;
        float td = 0;
        if (_via_tl_mode)
        {
            sub += _gen_via_Z0_ckt(v, refs_mat, via_call, td);
        }
        else
        {
            sub += _gen_via_model_ckt(v, refs_mat, via_call, td);
        }
        td_sum += td;
        len += _get_via_conn_len(v);
        ckt += via_call;
    }

    ckt += ".ends\n";
    ckt += sub;
    
    velocity_avg = len / td_sum;
    float product_sum = 0.;
    float sum = 0.;
    for (auto& Z0: v_Z0_td)
    {
        product_sum += Z0.first * Z0.second * Z0.second;
        sum += Z0.second * Z0.second;
    }
    Z0_avg = product_sum / sum;
    return true;
}


bool z_extractor::gen_subckt_coupled_tl(std::uint32_t net_id0, std::uint32_t net_id1, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& footprint, std::string& call,
                        float Z0_avg[2], float td_sum[2], float velocity_avg[2], float& Zodd_avg, float& Zeven_avg)
{
    
    if (_check_segments(net_id0) == false || _check_segments(net_id1) == false)
    {
        return false;
    }
    
    std::list<pad> pads0 = get_pads(net_id0);
    std::vector<std::list<z_extractor::segment> > v_segments0 = get_segments_sort(net_id0);
    std::list<via> vias0 = get_vias(net_id0);
    
    std::list<pad> pads1 = get_pads(net_id1);
    std::vector<std::list<z_extractor::segment> > v_segments1 = get_segments_sort(net_id1);
    std::list<via> vias1 = get_vias(net_id1);
    
#if DBG_IMG
    cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC3, cv::Scalar(0, 0, 0));
#endif

    std::multimap<float, std::pair<z_extractor::segment, z_extractor::segment> > coupler_segment;
    /* 找到符合耦合条件的走线 */
    for (auto& s_list0: v_segments0)
    {
        for (auto it0 = s_list0.begin(); it0 != s_list0.end();)
        {
            auto& s0 = *it0;
            bool _brk = false;
            for (auto& s_list1: v_segments1)
            {
                for (auto it1 = s_list1.begin(); it1 != s_list1.end(); it1++)
                {
                    auto& s1 = *it1;
                    if (_is_coupled(s0, s1, _coupled_max_gap, _coupled_min_len))
                    {
                        double aox1;
                        double aoy1;
                        double aox2;
                        double aoy2;
                        double box1;
                        double boy1;
                        double box2;
                        double boy2;
                        
                        if (calc_parallel_lines_overlap(s0.start.x, s0.start.y, s0.end.x, s0.end.y,
                                                        s1.start.x, s1.start.y, s1.end.x, s1.end.y,
                                                        aox1, aoy1, aox2, aoy2,
                                                        box1, boy1, box2, boy2))
                        {
                            std::list<z_extractor::segment> ss0;
                            std::list<z_extractor::segment> ss1;
                            _split_segment(s0, ss0, aox1, aoy1, aox2, aoy2);
                            _split_segment(s1, ss1, box1, boy1, box2, boy2);
                            double couple_len = calc_dist(aox1, aoy1, aox2, aoy2);
                            coupler_segment.emplace(1.0 / couple_len, std::pair<z_extractor::segment, z_extractor::segment>(ss0.front(), ss1.front()));
                            
                            ss0.pop_front();
                            ss1.pop_front();
                            
                            s_list0.erase(it0);
                            
                            s_list0.splice(s_list0.end(), ss0);
                            s_list1.splice(s_list1.end(), ss1);
                            s_list1.erase(it1);
                            
                            it0 = s_list0.begin();
                            _brk = true;
                            break;
                        }
                    }
                }
                if (_brk)
                {
                    break;
                }
            }
            if (!_brk)
            {
                it0++;
            }
            
        }
    }

    /* 显示 */
#if DBG_IMG
    for (auto& s_list0: v_segments0)
    {
        for (auto& s0: s_list0)
        {
            _draw_segment(img, s0, 255, 0, 0);
            
            for (auto& s_list1: v_segments1)
            {
                for (auto& s1: s_list1)
                {
                    _draw_segment(img, s1, 0, 255, 0);
                }
            }
        }
    }
    for (auto& ss_item: coupler_segment)
    {
        auto& ss = ss_item.second;
        
        _draw_segment(img, ss.first, 0, 0, 255);
        _draw_segment(img, ss.second, 0, 0, 255);
    }
    
    cv::imshow("img", img);
    cv::waitKey(0);
#endif
    
    std::string sub;
    std::string tmp;
    std::string comment;
    std::string pad_ckt;
    
    char buf[512] = {0};
    std::uint32_t net_ids[2] = {net_id0, net_id1};
    
    //生成子电路参数和调用代码
    ckt = ".subckt ";
    call = "X";
    
    for (auto& net_id: net_ids)
    {
        ckt += _format_net_name(_nets[net_id]);
        call += _format_net_name(_nets[net_id]);
        tmp += _format_net_name(_nets[net_id]);
    }
    
    ckt += " ";
    call += " ";
    
    comment = std::string(ckt.length(), '*');
    
    for (auto& net_id: net_ids)
    {
        std::list<pad> pads = get_pads(net_id);
        
        for (auto& p: pads)
        {
            float x;
            float y;
            _get_pad_pos(p, x, y);
            std::vector<std::string> layers = _get_pad_conn_layers(p);
            if (layers.size() == 0)
            {
                printf("err: %s.%s no connection.\n", p.footprint.c_str(), p.pad_number.c_str());
                return false;
            }
            sprintf(buf, "%s ", _pos2net(x, y, layers.front()).c_str());

            ckt += buf;
            call += _gen_pad_net_name(p.footprint, _format_net_name(_nets[net_id]));
            call += " ";
            
            comment += p.footprint + ":" + _nets[net_id];
            comment += " ";
            footprint.insert(p.footprint);
            
            for (std::uint32_t i = 1; i < layers.size(); i++)
            {
                sprintf(buf, "R%s%d %s %s 0\n", _get_tstamp_short(p.tstamp).c_str(), i,
                        _pos2net(x, y, layers.front()).c_str(), _pos2net(x, y, layers[i]).c_str());
                pad_ckt += buf;
            }
        }
    }
    
    ckt += "\n";
    call += tmp;
    call += "\n";
    
    ckt = comment + "\n" + ckt + pad_ckt;
    
    
    float len[2] = {0, 0};
    std::vector<std::pair<float, float> > v_Z0_td[2];
    std::vector<std::pair<float, float> > v_Zodd_td;
    std::vector<std::pair<float, float> > v_Zeven_td;
    
    /* 生成走线参数 */
    std::map<std::string, cv::Mat> refs_mat;
    _create_refs_mat(refs_id, refs_mat);
    
    std::vector<std::pair<z_extractor::segment, z_extractor::segment> > v_coupler_segment;
    for (auto& ss_item: coupler_segment)
    {
        v_coupler_segment.push_back(ss_item.second);
    }
    
    #pragma omp parallel for
    for (std::uint32_t i = 0; i < v_coupler_segment.size(); i++)
    {
        z_extractor::segment& s0 = v_coupler_segment[i].first;
        z_extractor::segment& s1 = v_coupler_segment[i].second;
        
        std::vector<std::pair<float, float> > v_Z0_td_[2];
        std::vector<std::pair<float, float> > v_Zodd_td_;
        std::vector<std::pair<float, float> > v_Zeven_td_;
        std::string subckt = _gen_segment_coupled_Z0_ckt_openmp(("CPL" + _get_tstamp_short(s0.tstamp)).c_str(), s0, s1, refs_mat, v_Z0_td_, v_Zodd_td_, v_Zeven_td_);
        
        
        char buf[512] = {0};
        sprintf(buf, "X%s %s %s %s %s CPL%s\n", _get_tstamp_short(s0.tstamp).c_str(),
                        _pos2net(s0.start.x, s0.start.y, s0.layer_name).c_str(),
                        _pos2net(s0.end.x, s0.end.y, s0.layer_name).c_str(),
                        _pos2net(s1.start.x, s1.start.y, s1.layer_name).c_str(),
                        _pos2net(s1.end.x, s1.end.y, s1.layer_name).c_str(),
                        _get_tstamp_short(s0.tstamp).c_str());
        #pragma omp critical
        {
            sub += subckt;
            ckt += buf;
            len[0] += _get_segment_len(s0);
            len[1] += _get_segment_len(s1);
            
            v_Z0_td[0].insert(v_Z0_td[0].end(), v_Z0_td_[0].begin(), v_Z0_td_[0].end());
            v_Z0_td[1].insert(v_Z0_td[1].end(), v_Z0_td_[1].begin(), v_Z0_td_[1].end());
            v_Zodd_td.insert(v_Zodd_td.end(), v_Zodd_td_.begin(), v_Zodd_td_.end());
            v_Zeven_td.insert(v_Zeven_td.end(), v_Zeven_td_.begin(), v_Zeven_td_.end());
            
            for (std::uint32_t i = 0; i < sizeof(net_ids) / sizeof(net_ids[0]); i++)
            {
                for (const auto& Z0_td: v_Z0_td_[i])
                {
                    td_sum[i] += Z0_td.second;
                }
            }
        }
    }
    
    v_segments0.insert(v_segments0.end(), v_segments1.begin(), v_segments1.end());
    
    for (auto& s_list: v_segments0)
    {
        std::string ckt_net_name;
        std::vector<z_extractor::segment> v_list(s_list.begin(), s_list.end());
        
        #pragma omp parallel for
        for (std::uint32_t i = 0; i < v_list.size(); i++)
        {
            char buf[512];
            z_extractor::segment& s = v_list[i];
            std::string tstamp = _get_tstamp_short(s.tstamp);
            std::string subckt;
            std::vector<std::pair<float, float> > v_Z0_td_;
            
            std::uint32_t idx = (s.net == net_id0)? 0: 1;
            subckt = _gen_segment_Z0_ckt_openmp(("ZO" + _get_tstamp_short(s.tstamp)).c_str(), s, refs_mat, v_Z0_td_);
            
            sprintf(buf, "X%s %s %s ZO%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
                                    
            #pragma omp critical
            {
                sub += subckt;
                ckt += buf;
                len[idx] += _get_segment_len(s);
                v_Z0_td[idx].insert(v_Z0_td[idx].end(), v_Z0_td_.begin(), v_Z0_td_.end());
                for (const auto& Z0_td: v_Z0_td_)
                {
                    td_sum[idx] += Z0_td.second;
                }
            }
        }
    }
    
    
    /* 生成过孔参数 */
    for (std::uint32_t i = 0; i < sizeof(net_ids) / sizeof(net_ids[0]); i++)
    {
        std::list<via> vias = get_vias(net_ids[i]);
    
        for (auto& v: vias)
        {
            std::string via_call;
            float td = 0;
            if (_via_tl_mode)
            {
                sub += _gen_via_Z0_ckt(v, refs_mat, via_call, td);
            }
            else
            {
                sub += _gen_via_model_ckt(v, refs_mat, via_call, td);
            }
            
            td_sum[i] += td;
            len[i] += _get_via_conn_len(v);
            ckt += via_call;
        }
    }
    
    ckt += ".ends\n";
    ckt += sub;
    
    velocity_avg[0] = len[0] / td_sum[0];
    velocity_avg[1] = len[1] / td_sum[1];
    
    for (std::uint32_t i = 0; i < sizeof(net_ids) / sizeof(net_ids[0]); i++)
    {
        float product_sum = 0.;
        float sum = 0.;
        for (auto& Z0: v_Z0_td[i])
        {
            product_sum += Z0.first * Z0.second * Z0.second;
            sum += Z0.second * Z0.second;
        }
        Z0_avg[i] = product_sum / sum;
    }
    
    {
        float product_sum = 0.;
        float sum = 0.;
        for (auto& Zodd: v_Zodd_td)
        {
            product_sum += Zodd.first * Zodd.second * Zodd.second;
            sum += Zodd.second * Zodd.second;
        }
        Zodd_avg = product_sum / sum;
    }
    {
        float product_sum = 0.;
        float sum = 0.;
        for (auto& Zeven: v_Zeven_td)
        {
            product_sum += Zeven.first * Zeven.second * Zeven.second;
            sum += Zeven.second * Zeven.second;
        }
        Zeven_avg = product_sum / sum;
    }
    return true;
}

#if 0
std::string z_extractor::gen_zone_fasthenry(std::uint32_t net_id, std::set<z_extractor::pcb_point>& points)
{
    std::string text;
    char buf[512];
    std::list<z_extractor::zone> zones = get_zones(net_id);
    std::uint32_t i = 0;
    points.clear();
    for (auto& z: zones)
    {
        std::list<z_extractor::cond> conds;
        
        _get_zone_cond(z, conds, points);
        std::string name = _format_layer_name(z.layer_name);
        
        float z_val = _get_layer_distance(_layers.front().name, z.layer_name);
        for (auto& p: points)
        {
            sprintf(buf, "N%d%sx%dy%dz%d x=%.2f y=%.2f z=%.2f\n",
                net_id, name.c_str(), (std::int32_t)(p.x * 10), (std::int32_t)(p.y * 10), (std::int32_t)(z_val * 10), p.x, p.y, z_val);
            text += std::string(buf);
        }
        
        for (auto& c: conds)
        {
            sprintf(buf, "E%d%s%u N%d%sx%dy%dz%d N%d%sx%dy%dz%d w=%f h=%f nhinc=1 nwinc=1\n",
                        net_id,
                        name.c_str(),
                        i++,
                        net_id, name.c_str(),
                        (std::int32_t)(c.start.x * 10), (std::int32_t)(c.start.y * 10), (std::int32_t)(z_val * 10),
                        net_id, name.c_str(),
                        (std::int32_t)(c.end.x * 10), (std::int32_t)(c.end.y * 10), (std::int32_t)(z_val * 10),
                        c.w, c.h);
            text += std::string(buf);
        }
    }
    return text;
}
#endif

void z_extractor::set_calc(std::uint32_t type)
{
    if (type == Z0_calc::Z0_CALC_MMTL)
    {
        std::int32_t thread_nums = omp_get_max_threads();
        _Z0_calc.clear();
        for (std::int32_t i = 0; i < thread_nums; i++)
        {
            std::shared_ptr<Z0_calc> calc = Z0_calc::create(Z0_calc::Z0_CALC_MMTL);
            char name[32];
            sprintf(name, "mmtl_tmp%d", i);
            calc->set_tmp_name(name);
            _Z0_calc.push_back(calc);
        }
    }
    else if (type == Z0_calc::Z0_CALC_ATLC)
    {
        std::int32_t thread_nums = omp_get_max_threads();
        _Z0_calc.clear();
        for (std::int32_t i = 0; i < thread_nums; i++)
        {
            std::shared_ptr<Z0_calc> calc = Z0_calc::create(Z0_calc::Z0_CALC_ATLC);
            char name[32];
            sprintf(name, "mmtl_tmp%d", i);
            calc->set_tmp_name(name);
            _Z0_calc.push_back(calc);
        }
    }
}


void z_extractor::dump()
{
    for (auto& i : _nets)
    {
        printf("net: id:%d name:%s\n", i.first, i.second.c_str());
        auto s = _segments.equal_range(i.first);
        if(s.first != std::end(_segments))
        {
            for (auto it = s.first; it != s.second; ++it)
            {
                printf("segment: start:(%f %f) end:(%f %f) width:%f layer:%s tstamp:%s\n",
                    it->second.start.x, it->second.start.y,
                    it->second.end.x, it->second.end.y,
                    it->second.width,
                    it->second.layer_name.c_str(), it->second.tstamp.c_str());
            }
        }
        
        auto v = _vias.equal_range(i.first);
        if(v.first != std::end(_vias))
        {
            for (auto it = v.first; it != v.second; ++it)
            {
                printf("via: at:(%f %f) size:%f drill:%f ",
                    it->second.at.x, it->second.at.y,
                    it->second.size, it->second.drill);
                    
                printf("layers:");
                for (auto layer : it->second.layers)
                {
                    printf(" %s", layer.c_str());
                }
                printf(" tstamp:%s\n", it->second.tstamp.c_str());
            }
        }
        
        auto p = _pads.equal_range(i.first);
        if(p.first != std::end(_pads))
        {
            for (auto it = p.first; it != p.second; ++it)
            {
                float angle = it->second.at_angle * M_PI / 180;
                float x = it->second.at.x;
                float y = it->second.at.y;
                
                float x1 = cosf(angle) * x - sinf(angle) * y;
                float y1 = cosf(angle) * y + sinf(angle) * x;
                printf("pad: ref at:(%f %f %f) ",
                    it->second.ref_at.x, it->second.ref_at.y, it->second.ref_at_angle);
                
                printf("at:(%f %f %f) ",
                    it->second.at.x, it->second.at.y, it->second.at_angle);
                    
                printf("pad:(%f %f) ",
                    it->second.ref_at.x + x1, it->second.ref_at.y + y1);
                
                printf("layers:");
                for (auto layer : it->second.layers)
                {
                    printf(" %s", layer.c_str());
                }
                
                printf(" net:%d net_name:%s", it->second.net, it->second.net_name.c_str());
                printf(" tstamp:%s\n", it->second.tstamp.c_str());
                
            }
        }
    }
}

bool z_extractor::_float_equal(float a, float b)
{
    return fabs(a - b) < _float_epsilon;
}


bool z_extractor::_point_equal(float x1, float y1, float x2, float y2)
{
    return _float_equal(x1, x2) && _float_equal(y1, y2);
}


void z_extractor::_get_pad_pos(const pad& p, float& x, float& y)
{
    float angle = p.ref_at_angle * M_PI / 180;
    float at_x = p.at.x;
    float at_y = -p.at.y;
                
    float x1 = cosf(angle) * at_x - sinf(angle) * at_y;
    float y1 = sinf(angle) * at_x + cosf(angle) * at_y;
    
    x = p.ref_at.x + x1;
    y = p.ref_at.y - y1;
}

std::string z_extractor::_get_tstamp_short(const std::string& tstamp)
{
    size_t pos = tstamp.find('-');
    if (pos != tstamp.npos)
    {
        return tstamp.substr(0, pos);
    }
    return tstamp;
}


std::string z_extractor::_format_net(const std::string& name)
{
    std::string tmp = name;
    for (auto& c: tmp)
    {
        if ((c < 'a' || c > 'z')
            && (c < 'A' || c > 'Z')
            && (c < '0' || c > '9')
            && c != '_' && c != '-')
        {
            c = '_';
        }
    }
    return tmp;
}

std::string z_extractor::_pos2net(float x, float y, const std::string& layer)
{
    char buf[128] = {0};
    sprintf(buf, "%d_%d", std::int32_t(x * 1000), std::int32_t(y * 1000));
    
    return std::string(buf) + _format_net(layer);
}


std::string z_extractor::_format_net_name(const std::string& net_name)
{
    return "NET_" + _format_net(net_name);
}


std::string z_extractor::_format_layer_name(std::string layer_name)
{
    return _format_net(layer_name);
}


std::string z_extractor::_gen_pad_net_name(const std::string& footprint, const std::string& net_name)
{
    char buf[256] = {0};
    sprintf(buf, "%s_%s", footprint.c_str(), net_name.c_str());
    return std::string(buf);
}


std::vector<std::string> z_extractor::_get_all_cu_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != "copper")
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}

std::vector<std::string> z_extractor::_get_all_dielectric_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != "core"
            && l.type != "prepreg"
            && l.type != "Top Solder Mask"
            && l.type != "Bottom Solder Mask")
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}

std::vector<std::string> z_extractor::_get_all_mask_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != "Top Solder Mask" && l.type != "Bottom Solder Mask")
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}


std::vector<std::string> z_extractor::_get_via_layers(const via& v)
{
    std::vector<std::string> layers;
    bool flag = false;
    for (auto& l: _layers)
    {
        if (l.type != "copper")
        {
            continue;
        }
        
        if (l.name == v.layers.front() || l.name == v.layers.back())
        {
            if (!flag)
            {
                flag = true;
                layers.push_back(l.name);
                continue;
            }
            else
            {
                layers.push_back(l.name);
                break;
            }
        }
        
        if (flag)
        {
            layers.push_back(l.name);
        }
    }
    return layers;
}


std::vector<std::string> z_extractor::_get_via_conn_layers(const z_extractor::via& v)
{
    std::vector<std::string> layers;
    std::set<std::string> layer_set;
    std::list<z_extractor::segment> segments = get_segments(v.net);
    for (const auto& s: segments)
    {
        if (_point_equal(s.start.x, s.start.y, v.at.x, v.at.y) || _point_equal(s.end.x, s.end.y, v.at.x, v.at.y))
        {
            layer_set.insert(s.layer_name);
        }
    }
    
    for (auto& l: _layers)
    {
        if (layer_set.count(l.name))
        {
            layers.push_back(l.name);
        }
    }
    return layers;
}


float z_extractor::_get_via_conn_len(const z_extractor::via& v)
{
    float min_z = 1;
    float max_z = -1;
    float len = 0;
    std::vector<std::string> conn_layers = _get_via_conn_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)conn_layers.size(); i++)
    {
        const std::string& layer_name = conn_layers[i];
        float z = _get_layer_z_axis(layer_name);
        if (z < min_z)
        {
            min_z = z;
        }
        if (z > max_z)
        {
            max_z = z;
        }
    }
    if (max_z > min_z)
    {
        len = max_z - min_z;
    }
    return len;
}


std::vector<std::string> z_extractor::_get_pad_conn_layers(const z_extractor::pad& p)
{
    float x;
    float y;
    _get_pad_pos(p, x, y);
    
    std::set<std::string> layer_set;
    std::list<z_extractor::segment> segments = get_segments(p.net);
    for (const auto& s: segments)
    {
        if (_point_equal(s.start.x, s.start.y, x, y) || _point_equal(s.end.x, s.end.y, x, y))
        {
            layer_set.insert(s.layer_name);
        }
    }
    
    std::vector<std::string> layers;
    std::vector<std::string> layers_tmp;
    for (const auto& layer: p.layers)
    {
        if (layer.find("*") != layer.npos)
        {
            layers_tmp = _get_all_cu_layer();
        }
        else
        {
            layers_tmp.push_back(layer);
        }
    }
    
    for (const auto& layer_name: layers_tmp)
    {
        if (layer_set.count(layer_name))
        {
            layers.push_back(layer_name);
        }
    }
    return layers;
}

std::vector<std::string> z_extractor::_get_pad_layers(const pad& p)
{
    std::vector<std::string> layers;
    for (const auto& layer: p.layers)
    {
        if (layer.find("*") != layer.npos)
        {
            layers = _get_all_cu_layer();
        }
        else
        {
            layers.push_back(layer);
        }
    }
    return layers;
}


float z_extractor::_get_layer_distance(const std::string& layer_name1, const std::string& layer_name2)
{
    bool flag = false;
    float dist = 0;
    for (auto& l: _layers)
    {
        if (l.name == layer_name1 || l.name == layer_name2)
        {
            if (!flag)
            {
                flag = true;
                continue;
            }
            else
            {
                return dist;
            }
        }
        if (flag)
        {
            dist += l.thickness;
        }
    }
    return 0;
}


float z_extractor::_get_layer_thickness(const std::string& layer_name)
{
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            return l.thickness;
        }
    }
    return 0;
}


float z_extractor::_get_layer_z_axis(const std::string& layer_name)
{
    float dist = 0;
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            break;
        }
        dist += l.thickness;
    }
    return dist;
}

float z_extractor::_get_layer_epsilon_r(const std::string& layer_name)
{
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            return l.epsilon_r;
        }
    }
    return 1;
}

/* 取上下两层介电常数的均值 */
float z_extractor::_get_cu_layer_epsilon_r(const std::string& layer_name)
{
    layer up;
    layer down;
    std::int32_t state = 0;
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            state = 1;
            continue;
        }
        if (state == 0)
        {
            up = l;
        }
        else if (state == 1)
        {
            down = l;
            break;
        }
    }
    
    
    if (up.type == "Top Solder Mask" || up.type == "Bottom Solder Mask")
    {
        return up.epsilon_r;
    }
    
    if (down.type == "Top Solder Mask" || down.type == "Bottom Solder Mask")
    {
        return down.epsilon_r;
    }
    
    return (up.epsilon_r + down.epsilon_r) * 0.5;
}

float z_extractor::_get_layer_epsilon_r(const std::string& layer_start, const std::string& layer_end)
{
    float epsilon_r = 0;
    bool flag = false;
    
    for (auto& l: _layers)
    {
        if (l.name == layer_start || l.name == layer_end)
        {
            if (!flag)
            {
                flag = true;
                continue;
            }
            else
            {
                return epsilon_r;
            }
        }
        if (flag)
        {
            epsilon_r = l.epsilon_r;
        }
    }
    return 1;
}

float z_extractor::_get_board_thickness()
{
    float dist = 0;
    for (auto& l: _layers)
    {
        dist += l.thickness;
    }
    return dist;
}


float z_extractor::_get_cu_min_thickness()
{
    float thickness = 1;
    for (const auto& l: _layers)
    {
        if (l.type == "copper")
        {
            if (l.thickness < thickness)
            {
                thickness = l.thickness;
            }
        }
    }
    return thickness;
}


bool z_extractor::_cu_layer_is_outer_layer(const std::string& layer_name)
{
    layer up;
    layer down;
    std::int32_t state = 0;
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            state = 1;
            continue;
        }
        if (state == 0)
        {
            up = l;
        }
        else if (state == 1)
        {
            down = l;
            break;
        }
    }
    
    if (up.type == "Top Solder Mask"
        || up.type == "Bottom Solder Mask"
        || down.type == "Top Solder Mask"
        || down.type == "Bottom Solder Mask")
    {
        return true;
    }
    return false;
}

float z_extractor::_get_segment_len(const z_extractor::segment& s)
{
    if (s.is_arc())
    {
        double cx;
        double cy;
        double radius;
        double angle;
        calc_arc_center_radius(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, radius);
        calc_arc_angle(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, radius, angle);
        return calc_arc_len(radius, angle);
    }
    else
    {
        return hypot(s.end.x - s.start.x, s.end.y - s.start.y);
    }
}

void z_extractor::_get_segment_pos(const z_extractor::segment& s, float offset, float& x, float& y)
{
    if (s.is_arc())
    {
        double cx;
        double cy;
        double arc_radius;
        double arc_angle;
        calc_arc_center_radius(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, arc_radius);
        calc_arc_angle(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, arc_radius, arc_angle);
        double arc_len = calc_arc_len(arc_radius, arc_angle);
        
        double angle = arc_angle * offset / arc_len;
        
        double x1 = cosf(angle) * (s.start.x - cx) - sinf(angle) * -(s.start.y - cy);
        double y1 = sinf(angle) * (s.start.x - cx) + cosf(angle) * -(s.start.y - cy);
    
        x = cx + x1;
        y = cy - y1;
    }
    else
    {
        float angle = calc_angle(s.start.x, s.start.y, s.end.x, s.end.y);
        x = s.start.x + offset * cos(angle);
        y = -(-s.start.y + offset * sin(angle));
    }
}


void z_extractor::_get_segment_perpendicular(const z_extractor::segment& s, float offset, float w, float& x_left, float& y_left, float& x_right, float& y_right)
{
    if (s.is_arc())
    {
        float x = 0;
        float y = 0;
        double cx;
        double cy;
        double arc_radius;
        calc_arc_center_radius(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, arc_radius);
        
        _get_segment_pos(s, offset, x, y);
        
        double rad_left = calc_angle(cx, cy, x, y);
        double rad_right = rad_left - (double)M_PI;
        
        /* >0 弧线为逆时针方向 */
        if ((s.mid.x - s.start.x) * (-s.end.y - -s.mid.y) - (-s.mid.y - -s.start.y) * (s.end.x - s.mid.x) > 0)
        {
            std::swap(rad_left, rad_right);
        }
        
        x_left = x + w * 0.5 * cos(rad_left);
        y_left = -(-y + w * 0.5 * sin(rad_left));
        x_right = x + w * 0.5 * cos(rad_right);
        y_right = -(-y + w * 0.5 * sin(rad_right));
    }
    else
    {
        float x = 0;
        float y = 0;
        float angle = calc_angle(s.start.x, s.start.y, s.end.x, s.end.y);
        float rad_left = angle + (float)M_PI_2;
        float rad_right = angle - (float)M_PI_2;
        
        _get_segment_pos(s, offset, x, y);
        
        x_left = x + w * 0.5 * cos(rad_left);
        y_left = -(-y + w * 0.5 * sin(rad_left));
        x_right = x + w * 0.5 * cos(rad_right);
        y_right = -(-y + w * 0.5 * sin(rad_right));
    }
}

bool z_extractor::_segments_get_next(std::list<z_extractor::segment>& segments, z_extractor::segment& s, float x, float y, const std::string& layer_name)
{
    for (auto it = segments.begin(); it != segments.end(); it++)
    {
        if (it->layer_name != layer_name)
        {
            continue;
        }
        
        if (_point_equal(x, y, it->start.x, it->start.y)
            || _point_equal(x, y, it->end.x, it->end.y))
        {
            s = *it;
            segments.erase(it);
            return true;
            break;
        }
    }
    return false;
}


bool z_extractor::_check_segments(std::uint32_t net_id)
{
    std::list<z_extractor::segment> segments = get_segments(net_id);
    std::list<z_extractor::pad> pads = get_pads(net_id);
    std::list<z_extractor::via> vias = get_vias(net_id);
    
    std::list<z_extractor::segment> no_conn;
    std::list<z_extractor::segment> conn;
    
    while (!segments.empty())
    {
        z_extractor::segment s = segments.front();
        segments.pop_front();
        bool start = false;
        bool end = false;
        for (const auto& it: segments)
        {
            if (it.layer_name != s.layer_name)
            {
                continue;
            }
            if (_point_equal(s.start.x, s.start.y, it.start.x, it.start.y)
                || _point_equal(s.start.x, s.start.y, it.end.x, it.end.y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, it.start.x, it.start.y)
                || _point_equal(s.end.x, s.end.y, it.end.x, it.end.y))
            {
                end = true;
            }
        }
        
        for (const auto& it: conn)
        {
            if (it.layer_name != s.layer_name)
            {
                continue;
            }
            if (_point_equal(s.start.x, s.start.y, it.start.x, it.start.y)
                || _point_equal(s.start.x, s.start.y, it.end.x, it.end.y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, it.start.x, it.start.y)
                || _point_equal(s.end.x, s.end.y, it.end.x, it.end.y))
            {
                end = true;
            }
        }
        
        for (const auto& it: no_conn)
        {
            if (it.layer_name != s.layer_name)
            {
                continue;
            }
            if (_point_equal(s.start.x, s.start.y, it.start.x, it.start.y)
                || _point_equal(s.start.x, s.start.y, it.end.x, it.end.y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, it.start.x, it.start.y)
                || _point_equal(s.end.x, s.end.y, it.end.x, it.end.y))
            {
                end = true;
            }
        }
        
        for (const auto&p: pads)
        {
            std::vector<std::string> layers = _get_pad_conn_layers(p);
            bool brk = true;
            for (const auto& l: layers)
            {
                if (l == s.layer_name)
                {
                    brk = false;
                    break;
                }
            }
            
            if (brk)
            {
                continue;
            }
            
            float x;
            float y;
            _get_pad_pos(p, x, y);
            
            if (_point_equal(s.start.x, s.start.y, x, y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, x, y))
            {
                end = true;
            }
        }
        
        
        for (const auto&v: vias)
        {
            std::vector<std::string> layers = _get_via_conn_layers(v);
            bool brk = true;
            for (const auto& l: layers)
            {
                if (l == s.layer_name)
                {
                    brk = false;
                    break;
                }
            }
            if (brk)
            {
                continue;
            }
            
            if (_point_equal(s.start.x, s.start.y, v.at.x, v.at.y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, v.at.x, v.at.y))
            {
                end = true;
            }
        }
        
        if (start && end)
        {
            conn.push_back(s);
        }
        else
        {
            if (!start)
            {
                printf("err: no connection (net:%s x:%.4f y:%.4f).\n", get_net_name(s.net).c_str(), s.start.x, s.start.y);
            }
            if (!end)
            {
                printf("err: no connection (net:%s x:%.4f y:%.4f).\n", get_net_name(s.net).c_str(), s.end.x, s.end.y);
            }
            
            no_conn.push_back(s);
        }
    }
    
    return no_conn.empty();
}


void z_extractor::_get_zone_cond(std::uint32_t net_id, const std::map<std::string, cv::Mat>& zone_mat, std::map<std::string, std::list<cond> >& conds, float& grid_size)
{
    float img_grid_size = _get_pcb_img_cols() / 50;
    if (img_grid_size < _get_pcb_img_rows() / 50)
    {
        img_grid_size = _get_pcb_img_rows() / 50;
    }
    if (_cvt_pcb_len(img_grid_size) < 0.5)
    {
        img_grid_size = _cvt_img_len(0.5);
    }
    grid_size = _cvt_pcb_len(img_grid_size);
    
    //float img_grid_size = _cvt_img_len(grid_size);
    std::uint32_t w_n = _get_pcb_img_cols() / img_grid_size;
    std::uint32_t h_n = _get_pcb_img_rows() / img_grid_size;
    
    for (const auto& mat: zone_mat)
    {
        const cv::Mat& img = mat.second;
        
        if (conds.count(mat.first) == 0)
        {
            conds.emplace(mat.first, std::list<cond>());
        }
        
        std::list<cond>& cond_list = conds[mat.first];
        
        for (std::uint32_t y = 0; y < h_n; y++)
        {
            for (std::uint32_t x = 0; x < w_n; x++)
            {
                std::uint32_t x1 = x * _get_pcb_img_cols() / w_n;
                std::uint32_t y1 = y * _get_pcb_img_rows() / h_n;
                std::uint32_t x2 = (x + 1) * _get_pcb_img_cols() / w_n;
                std::uint32_t y2 = (y + 1) * _get_pcb_img_rows() / h_n;
                
                cv::Rect r(x1, y1, x2 - x1, y2 - y1);
                cv::Mat roi = img(r);
                
                std::uint32_t count = 0;
                for(std::int32_t i = 0;i < roi.rows; i++)
                {
                    for(std::int32_t j = 0; j < roi.cols; j++)
                    {
                        if (roi.at<std::uint8_t>(i, j) > 0)
                        {
                            count++;
                        }
                    }
                }
                
                if (count >= (std::uint32_t)roi.rows * roi.cols * 0.8)
                {
                    //cv::line(img2, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 255));
                    float w_ratio = 1;
                    pcb_point p;
                    cond c;
                    c.start.x = _cvt_pcb_x(x1);
                    c.start.y = _cvt_pcb_y(y1);
                    c.end.x = _cvt_pcb_x(x2);
                    c.end.y = _cvt_pcb_y(y1);
                    c.w = _cvt_pcb_len(y2 - y1) * w_ratio;
                    
                    cond_list.push_back(c);
                    
                    c.start.x = _cvt_pcb_x(x1);
                    c.start.y = _cvt_pcb_y(y1);
                    c.end.x = _cvt_pcb_x(x1);
                    c.end.y = _cvt_pcb_y(y2);
                    c.w = _cvt_pcb_len(x2 - x1) * w_ratio;
                    cond_list.push_back(c);
                    
                    
                    c.start.x = _cvt_pcb_x(x2);
                    c.start.y = _cvt_pcb_y(y1);
                    c.end.x = _cvt_pcb_x(x2);
                    c.end.y = _cvt_pcb_y(y2);
                    c.w = _cvt_pcb_len(x2 - x1) * w_ratio;
                    cond_list.push_back(c);
                    
                    c.start.x = _cvt_pcb_x(x1);
                    c.start.y = _cvt_pcb_y(y2);
                    c.end.x = _cvt_pcb_x(x2);
                    c.end.y = _cvt_pcb_y(y2);
                    c.w = _cvt_pcb_len(y2 - y1) * w_ratio;
                    cond_list.push_back(c);
                }
            }
        }
    }
}



void z_extractor::_add_zone(fasthenry& henry, std::uint32_t net_id, const std::map<std::string, cv::Mat>& zone_mat, std::map<std::string, std::list<cond> >& conds, float& grid_size)
{
    _get_zone_cond(net_id, zone_mat, conds, grid_size);
    
    for (const auto& cond: conds)
    {
        const std::string& layer_name = cond.first;
        const std::list<z_extractor::cond>& cond_list = cond.second;
        
        float z_val = _get_layer_z_axis(layer_name);
        float h_val = _get_layer_thickness(layer_name);
        for (auto& c: cond_list)
        {
            std::string node1 = _pos2net(c.start.x, c.start.y, layer_name);
            std::string node2 = _pos2net(c.end.x, c.end.y, layer_name);
            henry.add_wire(node1, node2,
                                node1 + node2,
                                fasthenry::point(c.start.x, c.start.y, z_val),
                                fasthenry::point(c.end.x, c.end.y, z_val), c.w, h_val, 1, 1);
        }
        
    }
}


void z_extractor::_conn_to_zone(fasthenry& henry, float x, float y, std::map<std::string, cv::Mat>& zone_mat, const std::string& layer_name, std::map<std::string, std::list<z_extractor::cond> >& conds, float grid_size)
{
    if (conds.count(layer_name) == 0 || zone_mat.count(layer_name) == 0)
    {
        return;
    }
    
    std::list<cond>& cond_list = conds[layer_name];
    const cv::Mat& img = zone_mat[layer_name];
    
    if (img.at<std::uint8_t>(_cvt_img_y(y), _cvt_img_x(x)) == 0)
    {
        return;
    }
    std::string node1;
    std::string node2;
    float min_dist = 10000;
    for (const auto& c: cond_list)
    {
        float dist = calc_dist(c.start.x, c.start.y, x, y);
        if (dist < min_dist)
        {
            min_dist = dist;
            node1 = _pos2net(c.start.x, c.start.y, layer_name);
            node2 = _pos2net(x, y, layer_name);
        }
        
        dist = calc_dist(c.end.x, c.end.y, x, y);
        if (dist < min_dist)
        {
            min_dist = dist;
            node1 = _pos2net(c.end.x, c.end.y, layer_name);
            node2 = _pos2net(x, y, layer_name);
        }
    }
    //printf("grid_size:%f min_dist:%f\n", grid_size, min_dist);
    //if (min_dist < grid_size)
    {
        henry.add_equiv(node1, node2);
    }
}



void z_extractor::_draw_segment(cv::Mat& img, z_extractor::segment& s, std::uint8_t b, std::uint8_t g, std::uint8_t r)
{
    if (s.is_arc())
    {
        float s_len = _get_segment_len(s);
        for (float i = 0; i <= s_len - 0.01; i += 0.01)
        {
            float x1 = 0;
            float y1 = 0;
            float x2 = 0;
            float y2 = 0;
            _get_segment_pos(s, i, x1, y1);
            _get_segment_pos(s, i + 0.01, x2, y2);
            
            cv::line(img,
                cv::Point(_cvt_img_x(x1), _cvt_img_y(y1)),
                cv::Point(_cvt_img_x(x2), _cvt_img_y(y2)),
                cv::Scalar(b, g, r), _cvt_img_len(s.width), cv::LINE_4);
        }
    }
    else
    {
        cv::line(img,
            cv::Point(_cvt_img_x(s.start.x), _cvt_img_y(s.start.y)),
            cv::Point(_cvt_img_x(s.end.x), _cvt_img_y(s.end.y)),
            cv::Scalar(b, g, r), _cvt_img_len(s.width), cv::LINE_4);
    }
}


void z_extractor::_create_refs_mat(std::vector<std::uint32_t> refs_id, std::map<std::string, cv::Mat>& refs_mat, bool use_segment)
{
    for (auto ref_id: refs_id)
    {
        std::list<z_extractor::zone> zones = get_zones(ref_id);
        for (auto& zone: zones)
        {
            if (zone.pts.size() == 0)
            {
                continue;
            }
            if (refs_mat.count(zone.layer_name) == 0)
            {
                cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC1, cv::Scalar(0, 0, 0));
                refs_mat.emplace(zone.layer_name, img);
            }
            
            std::vector<cv::Point> pts;
            for (auto& i : zone.pts)
            {
                cv::Point p(_cvt_img_x(i.x), _cvt_img_y(i.y));
                pts.push_back(p);
            }
            cv::fillPoly(refs_mat[zone.layer_name], std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(255, 255, 255));
            
            //cv::Mat tmp;
            //cv::resize(refs_mat[zone.layer_name], tmp, cv::Size(1280, 960));
            //cv::namedWindow("mat", cv::WINDOW_NORMAL);
            //cv::imshow("mat", refs_mat[zone.layer_name]);
            //cv::waitKey();
        }
        
        if (use_segment)
        {
            std::vector<std::list<z_extractor::segment> > segments = get_segments_sort(ref_id);
            for (auto& segment: segments)
            {
                for (auto& s: segment)
                {
                    if (refs_mat.count(s.layer_name) == 0)
                    {
                        cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC1, cv::Scalar(0, 0, 0));
                        refs_mat.emplace(s.layer_name, img);
                    }
                    _draw_segment(refs_mat[s.layer_name], s, 255, 255, 255);
                        
                }
                
                /*for (auto& mat: refs_mat)
                {
                    cv::Mat tmp;
                    cv::resize(mat.second, tmp, cv::Size(1280, 960));
                    cv::namedWindow("mat", cv::WINDOW_NORMAL);
                    cv::imshow("mat", tmp);
                    cv::waitKey();
                }*/
            }
        }
    }
    
}

std::list<std::pair<float, float> > z_extractor::_get_mat_line(const cv::Mat& img, float x1, float y1, float x2, float y2)
{
    std::list<std::pair<float, float> > tmp;
    
#if DBG_IMG
    cv::Mat rgb;
    cv::cvtColor(img, rgb, cv::COLOR_GRAY2BGR);
    cv::line(rgb,
                    cv::Point(_cvt_img_x(x1), _cvt_img_y(y1)),
                    cv::Point(_cvt_img_x(x2), _cvt_img_y(y2)),
                    cv::Scalar(0, 0, 255), 1, cv::LINE_4);
    cv::imshow("rgb", rgb);
    cv::waitKey();
#endif
    float len = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    float angle = calc_angle(x1, y1, x2, y2);
    
    bool flag = false;
    float start = 0;
    float end = -1;
    
    for (float l = 0; l < len; l += 0.01)
    {
        float x = x1 + l * cos(angle);
        float y = -(-y1 + l * sin(angle));
        
        std::int32_t img_y = _cvt_img_y(y);
        std::int32_t img_x = _cvt_img_x(x);
        if (img_y >= 0 && img_y < img.rows
            && img_x >= 0 && img_x <= img.cols
            && img.at<std::uint8_t>(_cvt_img_y(y), _cvt_img_x(x)) > 0)
        {
            if (!flag)
            {
                flag = true;
                start = l;
                end = -1;
            }
            else
            {
                end = l;
            }
        }
        else
        {
            if (flag)
            {
                flag = false;
                if (end != -1)
                {
                    float w = (end - start);
                    float m = (end + start - len) * 0.5;
                    tmp.push_back(std::pair<float, float>(m, w));
                }
            }
        }
    }
    
    if (flag && end > -1)
    {
        flag = false;
        float w = (end - start);
        float m = (end  + start - len) * 0.5;
        tmp.push_back(std::pair<float, float>(m, w));
    }
    
    return tmp;
}


std::list<std::pair<float, float> > z_extractor::_get_segment_ref_plane(const z_extractor::segment& s, const cv::Mat& ref, float offset, float w)
{
    float x_left = 0;
    float y_left = 0;
    float x_right = 0;
    float y_right = 0;
    
    _get_segment_perpendicular(s, offset, w, x_left, y_left, x_right, y_right);
    return _get_mat_line(ref, x_left, y_left, x_right, y_right);
}


float z_extractor::_get_via_anti_pad_diameter(const z_extractor::via& v,  const std::map<std::string, cv::Mat>& refs_mat, std::string layer)
{
    float diameter = v.size * 5;
    if (refs_mat.count(layer) == 0)
    {
        return diameter;
    }
    const cv::Mat& img = refs_mat.find(layer)->second;
    
    float y = v.at.y - diameter * 0.5;
    float y_end = v.at.y + diameter * 0.5;
    while (y < y_end)
    {
        
        float x = v.at.x - diameter * 0.5;
        float x_end = v.at.x + diameter * 0.5;
        
        while (x < x_end)
        {
            std::int32_t img_y = _cvt_img_y(y);
            std::int32_t img_x = _cvt_img_x(x);
            if (!(img_y >= 0 && img_y < img.rows
                    && img_x >= 0 && img_x < img.cols)
                || img.at<std::uint8_t>(_cvt_img_y(y), _cvt_img_x(x)) > 0)
            {
                float dist = calc_dist(x, y, v.at.x, v.at.y);
                if (dist * 2 < diameter)
                {
                    diameter = dist * 2;
                }
            }
            x += 0.01;
        }
        y += 0.01;
    }
    if (diameter <= v.size)
    {
        diameter = v.size * 2;
    }
    return diameter;
}



float z_extractor::_calc_segment_r(const segment& s)
{
    //R = ρ * l / (w * h)  ρ:电导率(cu:0.0172) l:长度(m) w:线宽(mm) h:线厚度(mm) R:电阻(欧姆)
    float l = sqrt((s.start.x - s.end.x) * (s.start.x - s.end.x) + (s.start.y - s.end.y) * (s.start.y - s.end.y));
    
    return _resistivity * l * 0.001 / (s.width * _get_layer_thickness(s.layer_name));
}


float z_extractor::_calc_segment_l(const segment& s)
{
    float l = sqrt((s.start.x - s.end.x) * (s.start.x - s.end.x) + (s.start.y - s.end.y) * (s.start.y - s.end.y));
    return 2 * l * (log(2 * l / s.width) + 0.5 + 0.2235 * s.width / l);
}


float z_extractor::_calc_via_l(const via& v, const std::string& layer_name1, const std::string& layer_name2)
{
    float h = _get_layer_distance(layer_name1, layer_name2);
    return h / 5 * (1 + log(4.0 * h / v.drill));
}


bool z_extractor::_is_coupled(const z_extractor::segment& s1, const z_extractor::segment& s2, float coupled_max_gap, float coupled_min_len)
{
    if (s1.is_arc() || s2.is_arc())
    {
        return false;
    }
    
    float a1 = calc_angle(s1.start.x, s1.start.y, s1.end.x, s1.end.y);
    float a2 = calc_angle(s2.start.x, s2.start.y, s2.end.x, s2.end.y);
    float a22 = calc_angle(s2.end.x, s2.end.y, s2.start.x, s2.start.y);
    if (fabs(a1 - a2) > _float_epsilon && fabs(a1 - a22) > _float_epsilon)
    {
        return false;
    }
    float dist = calc_p2line_dist(s1.start.x, s1.start.y, s1.end.x, s1.end.y, s2.start.x, s2.start.y);
    float gap = dist - s1.width * 0.5 - s2.width * 0.5;
    if (gap > coupled_max_gap)
    {
        return false;
    }
    
    float ovlen = calc_parallel_lines_overlap_len(s1.start.x, s1.start.y, s1.end.x, s1.end.y,
                                        s2.start.x, s2.start.y, s2.end.x, s2.end.y);
    if (ovlen < coupled_min_len)
    {
        return false;
    }
    return true;
}



void z_extractor::_split_segment(const z_extractor::segment& s, std::list<z_extractor::segment>& ss, float x1, float y1, float x2, float y2)
{
    float d1 = calc_dist(x1, y1, s.start.x, s.start.y);
    float d2 = calc_dist(x2, y2, s.start.x, s.start.y);
    
    float limit = 0.0254;
    
    std::uint32_t idx = 0;
    std::vector<pcb_point> ps;
    ps.push_back(s.start);
    if (d1 < d2)
    {
        if (d1 > limit)
        {
            pcb_point p;
            p.x = x1;
            p.y = y1;
            ps.push_back(p);
            idx = 1;
        }
        else
        {
            idx = 0;
        }
        float d = calc_dist(x2, y2, s.end.x, s.end.y);
        if (d > limit)
        {
            pcb_point p;
            p.x = x2;
            p.y = y2;
            ps.push_back(p);
        }
    }
    else
    {
        if (d2 > limit)
        {
            pcb_point p;
            p.x = x2;
            p.y = y2;
            ps.push_back(p);
            idx = 1;
        }
        else
        {
            idx = 0;
        }
        float d = calc_dist(x1, y1, s.end.x, s.end.y);
        if (d > limit)
        {
            pcb_point p;
            p.x = x1;
            p.y = y1;
            ps.push_back(p);
        }
    }
    
    ps.push_back(s.end);
    
    const char *str[] = {"0", "1", "2", "3", "4"};
    for (std::uint32_t i = 0; i < ps.size() - 1; i++)
    {
        z_extractor::segment tmp = s;
        tmp.tstamp = str[i] + tmp.tstamp;
        tmp.start = ps[i];
        tmp.end = ps[i + 1];
        if (i == idx)
        {
            ss.push_front(tmp);
        }
        else
        {
            ss.push_back(tmp);
        }
    }
}


std::string z_extractor::_gen_segment_Z0_ckt_openmp(const std::string& cir_name, z_extractor::segment& s, const std::map<std::string, cv::Mat>& refs_mat, std::vector<std::pair<float, float> >& v_Z0_td)
{
#if DBG_IMG
    cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC1, cv::Scalar(0, 0, 0));
    _draw_segment(img, s, 255, 255, 255);
    cv::imshow("img", img);
                    
#endif

    std::string cir;
    float s_len = _get_segment_len(s);
    if (s_len < _segment_min_len)
    {
        return  ".subckt " + cir_name + " pin1 pin2\nR1 pin1 pin2 0\n.ends\n";
    }
    
    std::vector<std::string> layers = _get_all_dielectric_layer();
    float box_w = s.width * _Z0_w_ratio;
    float box_h = _get_cu_min_thickness() * _Z0_h_ratio;
    float box_y_offset = _get_board_thickness() * -0.5;
    float atlc_pix_unit = _get_cu_min_thickness() * 0.5;
    if (box_h < _get_board_thickness() * 1.5)
    {
        box_h = _get_board_thickness() * 1.5;
    }
    
    
    struct Z0_item
    {
        float Z0;
        float v;
        float c;
        float l;
        float r;
        float pos;
    };
    
    std::vector<Z0_item> Z0s;
    
    
    for (float i = 0; i < s_len; i += _Z0_step)
    {
        Z0_item tmp;
        tmp.pos = i;
        Z0s.push_back(tmp);
    }
    
    if (Z0s.size() > 1 && s_len - Z0s.back().pos < 0.5 * _Z0_step)
    {
        Z0s.back().pos = s_len;
    }
    else
    {
        Z0_item tmp;
        tmp.pos = s_len;
        Z0s.push_back(tmp);
    }
    
    std::int32_t thread_num = omp_get_thread_num();
    std::shared_ptr<Z0_calc>& calc = _Z0_calc[thread_num];
    calc->clean_all();
    
    for (std::uint32_t i = 0; i < Z0s.size(); i++)
    {
        Z0_item& item = Z0s[i];
        float pos = item.pos;
        calc->clean();
        calc->set_precision(atlc_pix_unit);
        calc->set_box_size(box_w, box_h);
        
        for (auto& l: layers)
        {
            float y = _get_layer_z_axis(l);
            calc->add_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
        }
        
        std::set<std::string> elec_add;
        for (auto& refs: refs_mat)
        {
            std::list<std::pair<float, float> >  grounds = _get_segment_ref_plane(s, refs.second, pos, s.width * _Z0_w_ratio);
            
            for (auto& g: grounds)
            {
                if (elec_add.count(refs.first) == 0)
                {
                    elec_add.insert(refs.first);
                    calc->add_elec(0, _get_layer_z_axis(refs.first) + box_y_offset, box_w, _get_layer_thickness(refs.first), _get_cu_layer_epsilon_r(refs.first));
                }
                calc->add_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
            }
        }
        
        if (elec_add.count(s.layer_name) == 0)
        {
            elec_add.insert(s.layer_name);
            calc->add_elec(0, _get_layer_z_axis(s.layer_name) + box_y_offset, box_w, _get_layer_thickness(s.layer_name), _get_cu_layer_epsilon_r(s.layer_name));
        }
        calc->add_wire(0, _get_layer_z_axis(s.layer_name) + box_y_offset, s.width, _get_layer_thickness(s.layer_name), _conductivity);
        
        
        float Z0;
        float v;
        float c;
        float l;
        float r;
        float g;
        
        calc->calc_Z0(Z0, v, c, l, r, g);
        log_debug("Zo:%g v:%gmm/ns c:%g l:%g\n", Z0, v / 1000000, c, l);
        item.Z0 = Z0;
        item.v = v;
        item.c = c;
        item.l = l;
        item.r = r;
    }

    int pin = 1;
    int idx = 1;
    char strbuf[512];
    
    Z0_item begin = Z0s[0];
    Z0_item end;
    for (std::uint32_t i = 1; i < Z0s.size(); i++)
    {
        end = Z0s[i];
        if (fabs(end.Z0 - begin.Z0) > _Z0_threshold || i + 1 == Z0s.size())
        {
            float dist = (end.pos - begin.pos);
            log_debug("dist:%g Z0:%g\n", dist, begin.Z0);
            float td = dist * 1000000 / begin.v;
            float r = begin.r;
            if (td < _td_threshold || _lossless_tl)
            {
                r = 0;
            }
            
            v_Z0_td.push_back(std::pair<float, float>(begin.Z0, td));
        #if 0
            if (td >= 0.001)
            {
                sprintf(strbuf, "T%d pin%d 0 pin%d 0 Z0=%g TD=%gNS\n", idx++, pin, pin + 1, begin.Z0, td);
            }
            else
            {
                sprintf(strbuf, "L%d pin%d pin%d %gnH\n", idx++, pin, pin + 1, last_l * dist * 0.001);
            }
        #else
            if (!_ltra_model)
            {
                sprintf(strbuf, "***Z0:%g TD:%gNS***\n"
                            "Y%d pin%d 0 pin%d 0 ymod%d LEN=%g\n"
                            ".MODEL ymod%d txl R=%g L=%gnH G=0 C=%gpF length=1\n",
                            begin.Z0, td,
                            idx, pin, pin + 1, idx, dist * 0.001,
                            idx, r, begin.l, begin.c
                            );
            }
            else
            {
            
                sprintf(strbuf, "***Z0:%g TD:%gNS***\n"
                            "O%d pin%d 0 pin%d 0 ltra%d\n"
                            ".MODEL ltra%d LTRA R=%g L=%gnH G=0 C=%gpF LEN=%g\n",
                            begin.Z0, td,
                            idx, pin, pin + 1, idx,
                            idx, r, begin.l, begin.c, dist * 0.001
                            );
            }
            idx++;
        #endif
            pin++;
            cir += strbuf;
            begin = end;
        }
    }
    
    cir += ".ends\n";
    sprintf(strbuf, ".subckt %s pin1 pin%d\n", cir_name.c_str(), pin);
    return  strbuf + cir;
}



std::string z_extractor::_gen_segment_coupled_Z0_ckt_openmp(const std::string& cir_name, z_extractor::segment& s0, z_extractor::segment& s1, const std::map<std::string, cv::Mat>& refs_mat,
                                                                std::vector<std::pair<float, float> > v_Z0_td[2],
                                                                std::vector<std::pair<float, float> >& v_Zodd_td,
                                                                std::vector<std::pair<float, float> >& v_Zeven_td)
{
    std::string cir;
    if (calc_dist(s0.start.x, s0.start.y, s1.start.x, s1.start.y) > calc_dist(s0.start.x, s0.start.y, s1.end.x, s1.end.y))
    {
        std::swap(s1.start, s1.end);
    }
    
    z_extractor::segment s;
    s.start.x = (s0.start.x + s1.start.x) * 0.5;
    s.start.y = (s0.start.y + s1.start.y) * 0.5;
    s.end.x = (s0.end.x + s1.end.x) * 0.5;
    s.end.y = (s0.end.y + s1.end.y) * 0.5;
    s.width = calc_p2line_dist(s0.start.x, s0.start.y, s0.end.x, s0.end.y, s1.start.x, s1.start.y);
    
    float s_len = _get_segment_len(s);
    bool s0_is_left = ((s.start.y - s.end.y) * s0.start.x + (s.end.x - s.start.x) * s0.start.y + s.start.x * s.end.y - s.end.x * s.start.y) > 0;

    std::vector<std::string> layers = _get_all_dielectric_layer();
    float box_w = std::min(std::max(s0.width, s1.width) * _Z0_w_ratio, s.width * _Z0_w_ratio);
    box_w = std::max(box_w, s.width * 2);
    float box_h = _get_cu_min_thickness() * _Z0_h_ratio;
    float box_y_offset = _get_board_thickness() * - 0.5;
    float atlc_pix_unit = _get_cu_min_thickness() * 0.5;
    if (box_h < _get_board_thickness() * 1.5)
    {
        box_h = _get_board_thickness() * 1.5;
    }
    
    struct Z0_item
    {
        float c_matrix[2][2];
        float l_matrix[2][2];
        float r_matrix[2][2];
        float g_matrix[2][2];
    
        float Zodd;
        float Zeven;
        
        float pos;
    };
    
    std::vector<Z0_item> ss_Z0s;
    
    for (float i = 0; i < s_len; i += _Z0_step)
    {
        Z0_item tmp;
        tmp.pos = i;
        ss_Z0s.push_back(tmp);
    }
    
    if (ss_Z0s.size() > 1 && s_len - ss_Z0s.back().pos < 0.5 * _Z0_step)
    {
        ss_Z0s.back().pos = s_len;
    }
    else
    {
        Z0_item tmp;
        tmp.pos = s_len;
        ss_Z0s.push_back(tmp);
    }
    
    
    
    std::int32_t thread_num = omp_get_thread_num();
    std::shared_ptr<Z0_calc>& calc = _Z0_calc[thread_num];
    calc->clean_all();
    for (std::uint32_t i = 0; i < ss_Z0s.size(); i++)
    {
        Z0_item& ss_item = ss_Z0s[i];
        
        calc->clean();
        calc->set_precision(atlc_pix_unit);
        calc->set_box_size(box_w, box_h);
        
        
        for (auto& l: layers)
        {
            float y = _get_layer_z_axis(l);
            calc->add_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
        }
        
        std::set<std::string> elec_add;
        for (auto& refs: refs_mat)
        {
            std::list<std::pair<float, float> >  grounds = _get_segment_ref_plane(s, refs.second, ss_item.pos, box_w);
            
            for (auto& g: grounds)
            {
                if (elec_add.count(refs.first) == 0)
                {
                    elec_add.insert(refs.first);
                    calc->add_elec(0, _get_layer_z_axis(refs.first) + box_y_offset, box_w, _get_layer_thickness(refs.first), _get_cu_layer_epsilon_r(refs.first));
                }
                calc->add_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
            }
        }
        
        if (elec_add.count(s0.layer_name) == 0)
        {
            elec_add.insert(s0.layer_name);
            calc->add_elec(0, _get_layer_z_axis(s0.layer_name) + box_y_offset, box_w, _get_layer_thickness(s0.layer_name), _get_cu_layer_epsilon_r(s0.layer_name));
        }
        if (elec_add.count(s1.layer_name) == 0)
        {
            elec_add.insert(s1.layer_name);
            calc->add_elec(0, _get_layer_z_axis(s1.layer_name) + box_y_offset, box_w, _get_layer_thickness(s1.layer_name), _get_cu_layer_epsilon_r(s1.layer_name));
        }
        
        if (s0_is_left)
        {
                
            calc->add_wire(0 - s.width * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name), _conductivity);
            calc->add_coupler(0 + s.width * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name), _conductivity);
        }
        else
        {
            calc->add_wire(0 + s.width * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name), _conductivity);
            calc->add_coupler(0 - s.width * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name), _conductivity);
        }
        
        
        calc->calc_coupled_Z0(ss_item.Zodd, ss_item.Zeven, ss_item.c_matrix, ss_item.l_matrix, ss_item.r_matrix, ss_item.g_matrix);
    }
    
    
    float c_matrix[2][2] = {0, 0, 0, 0};
    float l_matrix[2][2] = {0, 0, 0, 0};
    float r_matrix[2][2] = {0, 0, 0, 0};
    float g_matrix[2][2] = {0, 0, 0, 0};
        
    
    for (auto& item: ss_Z0s)
    {
        for (std::int32_t i = 0; i < 2; i++)
        {
            for (std::int32_t j = 0; j < 2; j++)
            {
                c_matrix[i][j] += item.c_matrix[i][j];
                l_matrix[i][j] += item.l_matrix[i][j];
                r_matrix[i][j] += item.r_matrix[i][j];
                g_matrix[i][j] += item.g_matrix[i][j];
            }
        }
    }
    
    for (std::int32_t i = 0; i < 2; i++)
    {
        for (std::int32_t j = 0; j < 2; j++)
        {
            c_matrix[i][j] = c_matrix[i][j] / ss_Z0s.size();
            l_matrix[i][j] = l_matrix[i][j] / ss_Z0s.size();
            r_matrix[i][j] = r_matrix[i][j] / ss_Z0s.size();
            g_matrix[i][j] = g_matrix[i][j] / ss_Z0s.size();
        }
    }
    
    float Zodd = sqrt((l_matrix[0][0] - (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1000 / (c_matrix[0][0] - (c_matrix[0][1] + c_matrix[1][0]) * 0.5))
                + sqrt((l_matrix[1][1] - (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1000 / (c_matrix[1][1] - (c_matrix[0][1] + c_matrix[1][0]) * 0.5));
    Zodd = Zodd * 0.5;
    float td_Zodd = sqrt((l_matrix[0][0] - (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1e-9 * (c_matrix[0][0] - (c_matrix[0][1] + c_matrix[1][0]) * 0.5) * 1e-12)
                + sqrt((l_matrix[1][1] - (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1e-9 * (c_matrix[1][1] - (c_matrix[0][1] + c_matrix[1][0]) * 0.5) * 1e-12);
    td_Zodd = s_len * 1000000 * td_Zodd * 0.5;
    
    float Zeven = sqrt((l_matrix[0][0] + (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1000 / (c_matrix[0][0] + (c_matrix[0][1] + c_matrix[1][0]) * 0.5))
                + sqrt((l_matrix[1][1] + (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1000 / (c_matrix[1][1] + (c_matrix[0][1] + c_matrix[1][0]) * 0.5));
    Zeven = Zeven * 0.5;
    float td_Zeven = sqrt((l_matrix[0][0] + (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1e-9 * (c_matrix[0][0] + (c_matrix[0][1] + c_matrix[1][0]) * 0.5) * 1e-12)
                + sqrt((l_matrix[1][1] + (l_matrix[0][1] + l_matrix[1][0]) * 0.5) * 1e-9 * (c_matrix[1][1] + (c_matrix[0][1] + c_matrix[1][0]) * 0.5) * 1e-12);
    td_Zeven = s_len * 1000000 * td_Zeven * 0.5;
    
    v_Zodd_td.push_back(std::pair<float, float>(Zodd, td_Zodd));
    v_Zeven_td.push_back(std::pair<float, float>(Zeven, td_Zeven));
    
    float Z0_s0 = sqrt(l_matrix[0][0] * 1000 / c_matrix[0][0]);
    float td_s0 = s_len * 1000000 * sqrt(l_matrix[0][0] * 1e-9 * c_matrix[0][0] * 1e-12);
    float Z0_s1 = sqrt(l_matrix[1][1] * 1000 / c_matrix[1][1]);
    float td_s1 = s_len * 1000000 * sqrt(l_matrix[1][1] * 1e-9 * c_matrix[1][1] * 1e-12);
    v_Z0_td[0].push_back(std::pair<float, float>(Z0_s0, td_s0));
    v_Z0_td[1].push_back(std::pair<float, float>(Z0_s1, td_s1));
    
    if (_lossless_tl)
    {
        // 正常的无损传输线电阻应该为0 这里将电阻值设置为1 否则ngspice仿真非常容易出异常 无法收敛
        r_matrix[0][0] = r_matrix[1][1] = 1; 
    }
    
    char strbuf[512];
    sprintf(strbuf, "***Zodd:%g Zeven:%g Zdiff:%g Zcomm:%g***\n"
                    "P1 pin1 pin3 0 pin2 pin4 0 PLINE\n"
                    ".model PLINE CPL length=%g\n"
                    "+R=%g 0 %g\n"
                    "+L=%gnH %gnH %gnH\n"
                    "+G=0 0 0\n"
                    "+C=%gpF %gpF %gpF\n",
                    Zodd, Zeven, Zodd * 2, Zeven * 0.5,
                    s_len * 0.001,
                    r_matrix[0][0], r_matrix[1][1],
                    l_matrix[0][0], (l_matrix[0][1] + l_matrix[1][0]) * 0.5, l_matrix[1][1],
                    c_matrix[0][0], (c_matrix[0][1] + c_matrix[1][0]) * 0.5, c_matrix[1][1]);
    cir += strbuf;
    
    cir += ".ends\n";
    sprintf(strbuf, ".subckt %s pin1 pin2  pin3 pin4\n", cir_name.c_str());
    return  strbuf + cir;
}

std::string z_extractor::_gen_via_Z0_ckt(z_extractor::via& v, std::map<std::string, cv::Mat>& refs_mat, std::string& call, float& td)
{
    char buf[2048] = {0};
    std::string ckt;
    ckt = ".subckt VIA" + _get_tstamp_short(v.tstamp) + " ";
    call = "XVIA" + _get_tstamp_short(v.tstamp) + " ";
    
    std::vector<std::string> conn_layers = _get_via_conn_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)conn_layers.size(); i++)
    {
        const std::string& layer_name = conn_layers[i];
        ckt += _pos2net(v.at.x, v.at.y, layer_name) + " ";
        call += _pos2net(v.at.x, v.at.y, layer_name) + " ";
    }
    
    ckt += "\n";
    call += "VIA" + _get_tstamp_short(v.tstamp) + "\n";
    
    float radius = v.drill * 0.5;
    float box_w = v.drill * 15;
    float box_h = v.drill * 15;
    float thickness = 0.0254 * 1;
    float atlc_pix_unit = thickness * 0.5;
    
    atlc atlc_;
    atlc_.set_precision(atlc_pix_unit);
    atlc_.set_box_size(box_w, box_h);
    
    
    std::uint32_t id = 0;
    std::vector<std::string> layers = _get_via_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
    {
        const std::string& start = layers[i];
        const std::string& end = layers[i + 1];
        
        float start_anti_pad_d = _get_via_anti_pad_diameter(v, refs_mat, start);
        float end_anti_pad_d = _get_via_anti_pad_diameter(v, refs_mat, end);
        float anti_pad_diameter = std::min(start_anti_pad_d, end_anti_pad_d);
        
        float h = _get_layer_distance(start, end);
        float er = _get_layer_epsilon_r(start, end);
        
        
        float Z0;
        float v_;
        float c;
        float l;
        float r = 0.;
        float g = 0.;
        
        atlc_.clean_all();
        atlc_.add_ring_elec(0, 0, radius, radius * 10, er);
        atlc_.add_ring_wire(0, 0, radius, thickness);
        atlc_.add_ring_ground(0, 0, anti_pad_diameter, thickness);
    
        atlc_.calc_Z0(Z0, v_, c, l, r, g);
        float td_ = h * 1000000 / v_;
        
        td += td_;
        
        if (!_ltra_model)
        {
            sprintf(buf, "***Z0:%g TD:%gNS***\n"
                        "Y%u %s 0 %s 0 ymod%u LEN=%g\n"
                        ".MODEL ymod%u txl R=0 L=%gnH G=0 C=%gpF length=1\n",
                        Z0, td_,
                        id, _pos2net(v.at.x, v.at.y, start).c_str(), _pos2net(v.at.x, v.at.y, end).c_str(), id, h * 0.001,
                        id, l, c
                        );
        }
        else
        {
            sprintf(buf, "***Z0:%g TD:%gNS***\n"
                        "O%u %s 0 %s 0 ltra%u\n"
                        ".MODEL ltra%u LTRA R=0 L=%gnH G=0 C=%gpF LEN=%g\n",
                        Z0, td_,
                        id, _pos2net(v.at.x, v.at.y, start).c_str(), _pos2net(v.at.x, v.at.y, end).c_str(), id,
                        id, l, c, h * 0.001
                        );
        }
        
        id++;
        ckt += buf;
    }
    return ckt + ".ends\n";
}


std::string z_extractor::_gen_via_model_ckt(z_extractor::via& v, std::map<std::string, cv::Mat>& refs_mat, std::string& call, float& td)
{
    char buf[2048] = {0};
    std::string ckt;
    ckt = ".subckt VIA" + _get_tstamp_short(v.tstamp) + " ";
    call = "XVIA" + _get_tstamp_short(v.tstamp) + " ";
    
    std::vector<std::string> conn_layers = _get_via_conn_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)conn_layers.size(); i++)
    {
        const std::string& layer_name = conn_layers[i];
        ckt += _pos2net(v.at.x, v.at.y, layer_name) + " ";
        call += _pos2net(v.at.x, v.at.y, layer_name) + " ";
    }
    
    ckt += "\n";
    call += "VIA" + _get_tstamp_short(v.tstamp) + "\n";
    
    
    std::uint32_t id = 0;
    std::vector<std::string> layers = _get_via_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
    {
        const std::string& start = layers[i];
        const std::string& end = layers[i + 1];
        float start_anti_pad_d = _get_via_anti_pad_diameter(v, refs_mat, start);
        float end_anti_pad_d = _get_via_anti_pad_diameter(v, refs_mat, end);
        float anti_pad_diameter = std::min(start_anti_pad_d, end_anti_pad_d);
        
        float h = _get_layer_distance(start, end);
        float er = _get_layer_epsilon_r(start, end);
        
        float c = 1.41 * er * (h / 25.4) * v.size / (anti_pad_diameter - v.size); //pF
        float l = h / 5 * (1 + log(4 * h / v.drill)); //nH
        
        td += sqrt(l * c * 0.001);
        
        sprintf(buf, "Cl%u %s 0 %gpF\n"
                        "L%u %s %s %gnH\n"
                        "Cr%u %s 0 %gpF\n",
                        id, _pos2net(v.at.x, v.at.y, start).c_str(), c * 0.5,
                        id, _pos2net(v.at.x, v.at.y, start).c_str(), _pos2net(v.at.x, v.at.y, end).c_str(), l,
                        id, _pos2net(v.at.x, v.at.y, end).c_str(), c * 0.5);
        id++;
        ckt += buf;
    }
    return ckt + ".ends\n";
}
