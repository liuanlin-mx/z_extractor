#include "kicad_pcb_sim.h"
#include <math.h>
#include <opencv2/opencv.hpp>
#include <fasthenry.h>

#if 0
#define log_debug(fmt, args...) printf(fmt, ##args)
#else
#define log_debug(fmt, args...)
#endif

#define log_info(fmt, args...) printf(fmt, ##args)

#define DBG_IMG 0
kicad_pcb_sim::kicad_pcb_sim()
{
    _Z0_setup = 0.5;
    _Z0_w_ratio = 8;
    _Z0_h_ratio = 1.5;
    
    _coupled_max_d = 5;
    _coupled_min_len = 0.5;
    
    _img_ratio = 15;
    _pcb_top = 10000.;
    _pcb_bottom = 0;
    _pcb_left = 10000.;
    _pcb_right = 0;
    _atlc_pix_unit = 0.035 / 4;
}

kicad_pcb_sim::~kicad_pcb_sim()
{
}


bool kicad_pcb_sim::parse(const char *str)
{
    while (*str)
    {
        if (*str == '(')
        {
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "kicad_pcb")
            {
                continue;
            }
            else if (label == "net")
            {
                std::uint32_t id = 0;
                std::string name;
                str = _parse_net(str, id, name);
                _nets.emplace(id, name);
                continue;
            }
            else if (label == "segment")
            {
                segment s;
                str = _parse_segment(str, s);
                _segments.emplace(s.net, s);
                continue;
            }
            else if (label == "gr_line")
            {
                str = _parse_edge(str);
                continue;
            }
            else if (label == "via")
            {
                via v;
                str = _parse_via(str, v);
                _vias.emplace(v.net, v);
                continue;
            }
            else if (label == "footprint")
            {
                str = _parse_footprint(str);
                continue;
            }
            else if (label == "setup")
            {
                str = _parse_setup(str);
                continue;
            }
            else if (label == "zone")
            {
                zone z;
                str = _parse_zone(str, z);
                _zones.emplace(z.net, z);
                continue;
            }
            
            str = _skip(str + 1);
            continue;
        }
        str++;
    }
    return true;
}



std::list<kicad_pcb_sim::segment> kicad_pcb_sim::get_segments(std::uint32_t net_id)
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


std::list<kicad_pcb_sim::pad> kicad_pcb_sim::get_pads(std::uint32_t net_id)
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


std::list<kicad_pcb_sim::via> kicad_pcb_sim::get_vias(std::uint32_t net_id)
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


std::list<kicad_pcb_sim::zone> kicad_pcb_sim::get_zones(std::uint32_t net_id)
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


std::vector<std::list<kicad_pcb_sim::segment> > kicad_pcb_sim::get_segments_sort(std::uint32_t net_id)
{
    std::vector<std::list<kicad_pcb_sim::segment> > v_segments;
    std::list<kicad_pcb_sim::segment> segments = get_segments(net_id);
    while (segments.size())
    {
        std::list<kicad_pcb_sim::segment> s_list;
        kicad_pcb_sim::segment first = segments.front();
        segments.pop_front();
        s_list.push_back(first);
        kicad_pcb_sim::segment tmp = first;
        kicad_pcb_sim::segment next;
        
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

std::string kicad_pcb_sim::get_net_name(std::uint32_t net_id)
{
    if (_nets.count(net_id))
    {
        return _nets[net_id];
    }
    return "";
}

std::uint32_t kicad_pcb_sim::get_net_id(std::string name)
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


bool kicad_pcb_sim::gen_subckt(std::uint32_t net_id, std::string& ckt, std::set<std::string>& reference_value)
{
    std::string call;
    std::string sub;
    char buf[512] = {0};
    
    std::list<pad> pads = get_pads(net_id);
    std::vector<std::list<kicad_pcb_sim::segment> > v_segments = get_segments_sort(net_id);
    std::list<via> vias = get_vias(net_id);
    

    /* 生成子电路参数和调用 */
    ckt += ".subckt " + _format_net_name(_nets[net_id]) + " ";
    call += "X" + _format_net_name(_nets[net_id]) + " ";
    
    for (auto& p: pads)
    {
        float x;
        float y;
        _get_pad_pos(p, x, y);
        sprintf(buf, "%s ", _pos2net(x, y, p.layers.front()).c_str());

        ckt += buf;
        call += _gen_pad_net_name(p.reference_value, _format_net_name(_nets[net_id]));
        call += " ";
        reference_value.insert(p.reference_value);
    }
    ckt += "\n";
    call += _format_net_name(_nets[net_id]);
    call += "\n";
    ckt = call + ckt;
    
    /* 构建fasthenry */
    fasthenry henry;
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
    henry.dump();
    
    /* 生成走线参数 */
    for (auto& s_list: v_segments)
    {
        std::string ckt_net_name;
            
        for (auto& s: s_list)
        {
            sub += henry.gen_ckt(_get_tstamp_short(s.tstamp).c_str(), ("RL" + _get_tstamp_short(s.tstamp)).c_str());
            
            sprintf(buf, "X%s %s %s RL%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
            ckt += buf;
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
            std::string name = _get_tstamp_short(v.tstamp) + _format_layer_name(start) + _format_layer_name(end);
            sub += henry.gen_ckt(name.c_str(), ("RL" + name).c_str());
            
            sprintf(buf, "X%s %s %s RL%s\n", name.c_str(),
                                    _pos2net(v.at.x, v.at.y, start).c_str(),
                                    _pos2net(v.at.x, v.at.y, end).c_str(),
                                    name.c_str());
            ckt += buf;
        }
    }
        
    ckt += ".ends\n";
    ckt += sub;
    return true;
}

bool kicad_pcb_sim::gen_subckt(std::vector<std::uint32_t> net_ids, std::vector<std::set<std::string> > mutual_ind_tstamp,
            std::string& ckt, std::set<std::string>& reference_value)
{
    std::string call;
    std::string sub;
    std::string tmp;
    fasthenry henry;
    char buf[512] = {0};
    
    //生成子电路参数和调用代码
    ckt += ".subckt ";
    call += "X";
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
            call += _gen_pad_net_name(p.reference_value, _format_net_name(_nets[net_id]));
            call += " ";
            reference_value.insert(p.reference_value);
        }
    }
    
    ckt += "\n";
    call += tmp;
    call += "\n";
    
    ckt = call + ckt;
    
    
    //构建fasthenry
    for (auto& net_id: net_ids)
    {
        std::vector<std::list<kicad_pcb_sim::segment> > v_segments = get_segments_sort(net_id);
        
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
    henry.dump();
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
            std::vector<std::list<kicad_pcb_sim::segment> > v_segments = get_segments_sort(net_id);
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
        std::vector<std::list<kicad_pcb_sim::segment> > v_segments = get_segments_sort(net_id);
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



bool kicad_pcb_sim::gen_subckt_zo(std::uint32_t net_id, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value)
{
    std::string call;
    std::string sub;
    char buf[512] = {0};
    
    std::list<pad> pads = get_pads(net_id);
    std::vector<std::list<kicad_pcb_sim::segment> > v_segments = get_segments_sort(net_id);
    std::list<via> vias = get_vias(net_id);
    

    /* 生成子电路参数和调用 */
    ckt += ".subckt " + _format_net_name(_nets[net_id]) + " ";
    call += "X" + _format_net_name(_nets[net_id]) + " ";
    
    for (auto& p: pads)
    {
        float x;
        float y;
        _get_pad_pos(p, x, y);
        sprintf(buf, "%s ", _pos2net(x, y, p.layers.front()).c_str());

        ckt += buf;
        call += _gen_pad_net_name(p.reference_value, _format_net_name(_nets[net_id]));
        call += " ";
        reference_value.insert(p.reference_value);
    }
    ckt += "\n";
    call += _format_net_name(_nets[net_id]);
    call += "\n";
    ckt = call + ckt;
    
    /* 构建fasthenry */
    fasthenry henry;
    for (auto& v: vias)
    {
        std::vector<std::string> layers = _get_via_layers(v);
        for (std::int32_t i = 0; i < (std::int32_t)layers.size() - 1; i++)
        {
            const std::string& start = layers[i];
            const std::string& end = layers[i + 1];
            std::string name = _get_tstamp_short(v.tstamp) + _format_layer_name(start) + _format_layer_name(end);
            henry.add_via(name.c_str(),
                            fasthenry::point(v.at.x, v.at.y, _get_layer_z_axis(start)),
                            fasthenry::point(v.at.x, v.at.y, _get_layer_z_axis(end)),
                            v.drill, v.size);
        }
    }
    henry.dump();
    
    /* 生成走线参数 */
    std::map<std::string, cv::Mat> refs_mat;
    _create_refs_mat(refs_id, refs_mat);
    for (auto& s_list: v_segments)
    {
        std::string ckt_net_name;
        for (auto& s: s_list)
        {
            std::string tstamp = _get_tstamp_short(s.tstamp);
            
            sub += _gen_segment_zo_ckt(("ZO" + _get_tstamp_short(s.tstamp)).c_str(), s, refs_mat);
            
            
            sprintf(buf, "X%s %s %s ZO%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
            ckt += buf;
            
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
            std::string name = _get_tstamp_short(v.tstamp) + _format_layer_name(start) + _format_layer_name(end);
            sub += henry.gen_ckt(name.c_str(), ("RL" + name).c_str());
            sprintf(buf, "X%s %s %s RL%s\n", name.c_str(),
                                    _pos2net(v.at.x, v.at.y, start).c_str(),
                                    _pos2net(v.at.x, v.at.y, end).c_str(),
                                    name.c_str());
            ckt += buf;
        }
    }
        
    ckt += ".ends\n";
    ckt += sub;
    return true;
}


bool kicad_pcb_sim::gen_subckt_coupled_tl(std::uint32_t net_id0, std::uint32_t net_id1, std::vector<std::uint32_t> refs_id,
                        std::string& ckt, std::set<std::string>& reference_value)
{
    
    std::list<pad> pads0 = get_pads(net_id0);
    std::vector<std::list<kicad_pcb_sim::segment> > v_segments0 = get_segments_sort(net_id0);
    std::list<via> vias0 = get_vias(net_id0);
    
    std::list<pad> pads1 = get_pads(net_id1);
    std::vector<std::list<kicad_pcb_sim::segment> > v_segments1 = get_segments_sort(net_id1);
    std::list<via> vias1 = get_vias(net_id1);
    
#if DBG_IMG
    cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC3, cv::Scalar(0, 0, 0));
#endif

    std::multimap<float, std::pair<kicad_pcb_sim::segment, kicad_pcb_sim::segment> > coupler_segment;
    /* 找到符合耦合条件的走线 */
    for (auto& s_list0: v_segments0)
    {
        for (auto it0 = s_list0.begin(); it0 != s_list0.end(); it0++)
        {
            auto& s0 = *it0;
            bool _brk = false;
            for (auto& s_list1: v_segments1)
            {
                for (auto it1 = s_list1.begin(); it1 != s_list1.end(); it1++)
                {
                    auto& s1 = *it1;
                    if (_is_coupled(s0, s1, _coupled_max_d, _coupled_min_len))
                    {
                        float aox1;
                        float aoy1;
                        float aox2;
                        float aoy2;
                        float box1;
                        float boy1;
                        float box2;
                        float boy2;
                        
                        if (_calc_parallel_lines_overlap(s0.start.x, s0.start.y, s0.end.x, s0.end.y,
                                                        s1.start.x, s1.start.y, s1.end.x, s1.end.y,
                                                        aox1, aoy1, aox2, aoy2,
                                                        box1, boy1, box2, boy2))
                        {
                            std::list<kicad_pcb_sim::segment> ss0;
                            std::list<kicad_pcb_sim::segment> ss1;
                            _split_segment(s0, ss0, aox1, aoy1, aox2, aoy2);
                            _split_segment(s1, ss1, box1, boy1, box2, boy2);
                            float couple_len = _calc_dist(aox1, aoy1, aox2, aoy2);
                            coupler_segment.emplace(1.0 / couple_len, std::pair<kicad_pcb_sim::segment, kicad_pcb_sim::segment>(ss0.front(), ss1.front()));
                            
                            ss0.pop_front();
                            ss1.pop_front();
                            s_list0.splice(s_list0.end(), ss0);
                            s_list1.splice(s_list1.end(), ss1);
                            s_list1.erase(it1);
                            it0 = s_list0.erase(it0);
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
            
            
        }
    }

    /* 显示 */
#if DBG_IMG
    for (auto& s_list0: v_segments0)
    {
        for (auto& s0: s_list0)
        {
            cv::line(img, cv::Point(_cvt_img_x(s0.start.x), _cvt_img_y(s0.start.y)),
                    cv::Point(_cvt_img_x(s0.end.x), _cvt_img_y(s0.end.y)),
                    cv::Scalar(255, 0, 0), _cvt_img_len(s0.width), cv::LINE_4);
            
            for (auto& s_list1: v_segments1)
            {
                for (auto& s1: s_list1)
                {
                    cv::line(img, cv::Point(_cvt_img_x(s1.start.x), _cvt_img_y(s1.start.y)),
                            cv::Point(_cvt_img_x(s1.end.x), _cvt_img_y(s1.end.y)),
                            cv::Scalar(0, 255, 0), _cvt_img_len(s1.width), cv::LINE_4);
                }
            }
        }
    }
    for (auto& ss_item: coupler_segment)
    {
        auto& ss = ss_item.second;
        cv::line(img, cv::Point(_cvt_img_x(ss.first.start.x), _cvt_img_y(ss.first.start.y)),
                    cv::Point(_cvt_img_x(ss.first.end.x), _cvt_img_y(ss.first.end.y)),
                    cv::Scalar(0, 0, 255), _cvt_img_len(ss.first.width), cv::LINE_4);
        cv::line(img, cv::Point(_cvt_img_x(ss.second.start.x), _cvt_img_y(ss.second.start.y)),
                    cv::Point(_cvt_img_x(ss.second.end.x), _cvt_img_y(ss.second.end.y)),
                    cv::Scalar(0, 0, 255), _cvt_img_len(ss.second.width), cv::LINE_4);
    }
    
    cv::imshow("img", img);
    cv::waitKey(0);
#endif
    
    std::string call;
    std::string sub;
    std::string tmp;
    fasthenry henry;
    char buf[512] = {0};
    std::uint32_t net_ids[2] = {net_id0, net_id1};
    
    //生成子电路参数和调用代码
    ckt += ".subckt ";
    call += "X";
    
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
            call += _gen_pad_net_name(p.reference_value, _format_net_name(_nets[net_id]));
            call += " ";
            reference_value.insert(p.reference_value);
        }
    }
    
    ckt += "\n";
    call += tmp;
    call += "\n";
    
    ckt = call + ckt;
    
    
    //构建fasthenry
    for (auto& net_id: net_ids)
    {
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
    henry.dump();
    
    /* 生成走线参数 */
    std::map<std::string, cv::Mat> refs_mat;
    _create_refs_mat(refs_id, refs_mat);
    
    for (auto& ss_item: coupler_segment)
    {
        kicad_pcb_sim::segment& s0 = ss_item.second.first;
        kicad_pcb_sim::segment& s1 = ss_item.second.second;
        
        sub += _gen_segment_coupled_zo_ckt(("CPL" + _get_tstamp_short(s0.tstamp)).c_str(), s0, s1, refs_mat);
        sprintf(buf, "X%s %s %s %s %s CPL%s\n", _get_tstamp_short(s0.tstamp).c_str(),
                        _pos2net(s0.start.x, s0.start.y, s0.layer_name).c_str(),
                        _pos2net(s0.end.x, s0.end.y, s0.layer_name).c_str(),
                        _pos2net(s1.start.x, s1.start.y, s1.layer_name).c_str(),
                        _pos2net(s1.end.x, s1.end.y, s1.layer_name).c_str(),
                        _get_tstamp_short(s0.tstamp).c_str());
        ckt += buf;
            
    }
    
    for (auto& s_list: v_segments0)
    {
        std::string ckt_net_name;
        for (auto& s: s_list)
        {
            std::string tstamp = _get_tstamp_short(s.tstamp);
            
            sub += _gen_segment_zo_ckt(("ZO" + _get_tstamp_short(s.tstamp)).c_str(), s, refs_mat);
            
            
            sprintf(buf, "X%s %s %s ZO%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
            ckt += buf;
            
        }
    }
    
    for (auto& s_list: v_segments1)
    {
        std::string ckt_net_name;
        for (auto& s: s_list)
        {
            std::string tstamp = _get_tstamp_short(s.tstamp);
            
            sub += _gen_segment_zo_ckt(("ZO" + _get_tstamp_short(s.tstamp)).c_str(), s, refs_mat);
            
            
            sprintf(buf, "X%s %s %s ZO%s\n", _get_tstamp_short(s.tstamp).c_str(),
                                    _pos2net(s.start.x, s.start.y, s.layer_name).c_str(),
                                    _pos2net(s.end.x, s.end.y, s.layer_name).c_str(),
                                    _get_tstamp_short(s.tstamp).c_str());
            ckt += buf;
            
        }
    }
    
    
    
    
    /* 生成过孔参数 */
    for (auto& net_id: net_ids)
    {
        std::list<via> vias = get_vias(net_id);
    
        for (auto& v: vias)
        {
            std::string tstamp = _get_tstamp_short(v.tstamp);
                
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


std::string kicad_pcb_sim::gen_zone_fasthenry(std::uint32_t net_id, std::set<kicad_pcb_sim::pcb_point>& points)
{
    std::string text;
    char buf[512];
    std::list<kicad_pcb_sim::zone> zones = get_zones(net_id);
    std::uint32_t i = 0;
    points.clear();
    for (auto& z: zones)
    {
        std::list<kicad_pcb_sim::cond> conds;
        
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


void kicad_pcb_sim::dump()
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


const char *kicad_pcb_sim::_parse_label(const char *str, std::string& label)
{
    while (*str != ' ' && *str != '\r' && *str != '\n')
    {
        label += *str;
        str++;
    }
    return str;
}

const char *kicad_pcb_sim::_skip(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
        }
        if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}

const char *kicad_pcb_sim::_parse_zone(const char *str, zone& z)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "net")
            {
                float id;
                str = _parse_number(str, id);
                z.net = (std::uint32_t)id;
            }
            else if (label == "tstamp")
            {
                str = _parse_tstamp(str, z.tstamp);
            }
            else if (label == "filled_polygon")
            {
                _parse_filled_polygon(str, z);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_filled_polygon(const char *str, zone& z)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "layer")
            {
                while (*str != '"') str++;
                str = _parse_string(str, z.layer_name);
            }
            else if (label == "xy")
            {
                pcb_point p;
                str = _parse_postion(str, p.x, p.y);
                z.pts.push_back(p);
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_net(const char *str, std::uint32_t& id, std::string& name)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '"')
        {
            str = _parse_string(str, name);
            continue;
        }
        else if (*str >= '0' && *str <= '9')
        {
            float num = 0;
            str = _parse_number(str, num);
            id = num;
            continue;
        }
        else if (*str == '(')
        {
            left++;
        }
        else if (*str == ')')
        {
            right++;
        }
        
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_segment(const char *str, segment& s)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "start")
            {
                float x;
                float y;
                str = _parse_postion(str, x, y);
                s.start.x = x;
                s.start.y = y;
            }
            else if (label == "end")
            {
                float x;
                float y;
                str = _parse_postion(str, x, y);
                s.end.x = x;
                s.end.y = y;
            }
            else if (label == "layer")
            {
                while (*str != '"') str++;
                str = _parse_string(str, s.layer_name);
            }
            else if (label == "width")
            {
                float w;
                str = _parse_number(str, w);
                s.width = w;
            }
            else if (label == "net")
            {
                float id;
                str = _parse_number(str, id);
                s.net = (std::uint32_t)id;
            }
            else if (label == "tstamp")
            {
                str = _parse_tstamp(str, s.tstamp);
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}

const char *kicad_pcb_sim::_parse_via(const char *str, via& v)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "at")
            {
                float x;
                float y;
                str = _parse_postion(str, x, y);
                v.at.x = x;
                v.at.y = y;
            }
            else if (label == "size")
            {
                float size;
                str = _parse_number(str, size);
                v.size = size;
            }
            else if (label == "drill")
            {
                float drill;
                str = _parse_number(str, drill);
                v.drill = drill;
            }
            else if (label == "layers")
            {
                str = _parse_layers(str, v.layers);
            }
            else if (label == "net")
            {
                float id;
                str = _parse_number(str, id);
                v.net = (std::uint32_t)id;
            }
            else if (label == "tstamp")
            {
                str = _parse_tstamp(str, v.tstamp);
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_number(const char *str, float &num)
{
    char tmp[32] = {0};
    std::uint32_t i = 0;
    
    while (*str == ' ') str++;
    
    while (((*str >= '0' && *str <= '9') || *str == '.' || *str == '-') && i < sizeof(tmp) - 1)
    {
        tmp[i++] = *str;
        str++;
    }
    tmp[i] = 0;
    sscanf(tmp, "%f", &num);
    return str;
}


const char *kicad_pcb_sim::_parse_string(const char *str, std::string& text)
{
    if (*str == '"')
    {
        str++;
        while (*str != '"')
        {
            text += *str;
            str++;
        }
        return str + 1;
    }
    else
    {
        while (*str != ' ' && *str != ')')
        {
            text += *str;
            str++;
        }
        return str;
    }
}


const char *kicad_pcb_sim::_parse_postion(const char *str, float &x, float& y)
{
    while (*str == ' ') str++;
    str = _parse_number(str, x);
    while (*str == ' ') str++;
    str = _parse_number(str, y);
    return str;
}



const char *kicad_pcb_sim::_parse_tstamp(const char *str, std::string& tstamp)
{
    while (*str == ' ') str++;
    while (*str != ')')
    {
        tstamp += *str;
        str++;
    }
    return str;
}


const char *kicad_pcb_sim::_parse_layers(const char *str, std::list<std::string>& layers)
{
    while (*str != ')')
    {
        std::string text;
        while (*str == ' ') str++;
        str = _parse_string(str, text);
        if (text.npos != text.find(".Cu"))
        {
            layers.push_back(text);
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_footprint(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    std::string reference_value;
    float x;
    float y;
    float angle;
                
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            
            if (label == "at")
            {
                str = _parse_at(str, x, y, angle);
            }
            else if (label == "fp_text")
            {
                std::string label;
                while (*str == ' ') str++;
                str = _parse_label(str, label);
                if (label == "reference")
                {
                    str = _parse_reference(str, reference_value);
                }
                str = _skip(str);
                right++;
            }
            else if (label == "pad")
            {
                pad p;
                p.ref_at.x = x;
                p.ref_at.y = y;
                p.ref_at_angle = angle;
                p.net = 0xffffffff;
                p.reference_value = reference_value;
                str = _parse_pad(str, p);
                if (p.net != 0xffffffff)
                {
                    _pads.emplace(p.net, p);
                }
                right++;
            }
            else
            {
                str = _skip(str);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_at(const char *str, float &x, float& y, float& angle)
{
    while (*str == ' ') str++;
    str = _parse_number(str, x);
    while (*str == ' ') str++;
    str = _parse_number(str, y);
    
    while (*str == ' ') str++;
    if (*str == ')')
    {
        angle = 0.0;
        return str;
    }
    
    str = _parse_number(str, angle);
    return str;
}



const char *kicad_pcb_sim::_parse_reference(const char *str, std::string& footprint_name)
{
    while (*str == ' ') str++;
    str = _parse_string(str, footprint_name);
    return str;
}


const char *kicad_pcb_sim::_parse_pad(const char *str, pad& p)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    float x = 0.;
    float y = 0.;
    float angle = 0.;
    while (*str)
    {
        if (*str == '"')
        {
            str = _parse_string(str, p.pad_number);
            continue;
        }
        else if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "at")
            {
                str = _parse_at(str, x, y, angle);
                p.at.x = x;
                p.at.y = y;
                p.at_angle = angle;
            }
            else if (label == "net")
            {
                str = _parse_net(str, p.net, p.net_name);
                right++;
            }
            else if (label == "tstamp")
            {
                std::string tstamp;
                str = _parse_tstamp(str, p.tstamp);
            }
            else if (label == "layers")
            {
                str = _parse_layers(str, p.layers);
            }
            else
            {
                str = _skip(str + 1);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_setup(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
                
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            
            if (label == "stackup")
            {
                str = _parse_stackup(str);
                right++;
            }
            else
            {
                str = _skip(str);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


const char *kicad_pcb_sim::_parse_stackup(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
                
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            
            if (label == "layer")
            {
                str = _parse_stackup_layer(str);
                right++;
            }
            else
            {
                str = _skip(str);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}

const char *kicad_pcb_sim::_parse_stackup_layer(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    layer l;
    
    while (*str != '"') str++;
    str = _parse_string(str, l.name);
    
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            
            if (label == "type")
            {
                while (*str != '"') str++;
                str = _parse_string(str, l.type);
            }
            else if (label == "thickness")
            {
                while (*str == ' ') str++;
                str = _parse_number(str, l.thickness);
            }
            else if (label == "epsilon_r")
            {
                while (*str == ' ') str++;
                str = _parse_number(str, l.epsilon_r);
            }
            else
            {
                str = _skip(str);
                right++;
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        
        str++;
        if (left == right)
        {
            break;
        }
    }
    
    if (l.type == "copper" || l.type == "core" || l.type == "prepreg")
    {
        _layers.push_back(l);
        log_debug("layer name:%s type:%s t:%f e:%f\n", l.name.c_str(), l.type.c_str(), l.thickness, l.epsilon_r);
    }
    return str;
}


const char *kicad_pcb_sim::_parse_edge(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "start" || label == "end")
            {
                float x;
                float y;
                str = _parse_postion(str, x, y);
                if (x < _pcb_left)
                {
                    _pcb_left = x;
                }
                if (x > _pcb_right)
                {
                    _pcb_right = x;
                }
                if (y < _pcb_top)
                {
                    _pcb_top = y;
                }
                if (y > _pcb_bottom)
                {
                    _pcb_bottom = y;
                }
                
            }
            else if (label == "layer")
            {
                while (*str != '"') str++;
                std::string layer_name;
                str = _parse_string(str, layer_name);
            }
            else if (label == "width")
            {
                float w;
                str = _parse_number(str, w);
            }
            else if (label == "tstamp")
            {
                std::string tstamp;
                str = _parse_tstamp(str, tstamp);
            }
            continue;
        }
        else if (*str == ')')
        {
            right++;
        }
        str++;
        if (left == right)
        {
            break;
        }
    }
    return str;
}


void kicad_pcb_sim::_get_pad_pos(pad& p, float& x, float& y)
{
    float angle = p.ref_at_angle * M_PI / 180;
    float at_x = p.at.x;
    float at_y = -p.at.y;
                
    float x1 = cosf(angle) * at_x - sinf(angle) * at_y;
    float y1 = sinf(angle) * at_x + cosf(angle) * at_y;
    
    x = p.ref_at.x + x1;
    y = p.ref_at.y - y1;
}

std::string kicad_pcb_sim::_get_tstamp_short(const std::string& tstamp)
{
    size_t pos = tstamp.find('-');
    if (pos != tstamp.npos)
    {
        return tstamp.substr(0, pos);
    }
    return tstamp;
}


std::string kicad_pcb_sim::_pos2net(float x, float y, const std::string& layer)
{
    char buf[128] = {0};
    sprintf(buf, "%d_%d", std::int32_t(x * 1000), std::int32_t(y * 1000));
    
    std::string tmp = layer;
    return std::string(buf) + tmp.replace(tmp.find('.'), 1, "_");
}


std::string kicad_pcb_sim::_format_net_name(const std::string& net_name)
{
    std::string tmp = net_name;
    std::size_t pos = tmp.find('-');
    
    if ((pos = tmp.find('(')) != tmp.npos)
    {
        tmp.replace(pos, 1, "_");
    }
    
    if ((pos = tmp.find(')')) != tmp.npos)
    {
        tmp.replace(pos, 1, "_");
    }
    
    while ((pos = tmp.find('/')) != tmp.npos)
    {
        tmp.replace(pos, 1, "_");
    }
    
    return "NET_" + tmp;
}


std::string kicad_pcb_sim::_format_layer_name(std::string layer_name)
{
    return layer_name.substr(0, layer_name.find('.'));
}


std::string kicad_pcb_sim::_gen_pad_net_name(const std::string& reference_value, const std::string& net_name)
{
    char buf[256] = {0};
    sprintf(buf, "%s_%s", reference_value.c_str(), net_name.c_str());
    return std::string(buf);
}


std::vector<std::string> kicad_pcb_sim::_get_all_cu_layer()
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

std::vector<std::string> kicad_pcb_sim::_get_all_dielectric_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != "core" && l.type != "prepreg")
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}

std::vector<std::string> kicad_pcb_sim::_get_via_layers(const via& v)
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


float kicad_pcb_sim::_get_layer_distance(const std::string& layer_name1, const std::string& layer_name2)
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


float kicad_pcb_sim::_get_layer_thickness(const std::string& layer_name)
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


float kicad_pcb_sim::_get_layer_z_axis(const std::string& layer_name)
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

float kicad_pcb_sim::_get_layer_epsilon_r(const std::string& layer_name)
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

float kicad_pcb_sim::_get_board_thickness()
{
    float dist = 0;
    for (auto& l: _layers)
    {
        dist += l.thickness;
    }
    return dist;
}


bool kicad_pcb_sim::_float_equal(float a, float b)
{
    return fabs(a - b) < 0.00001;
}


bool kicad_pcb_sim::_point_equal(float x1, float y1, float x2, float y2)
{
    return _float_equal(x1, x2) && _float_equal(y1, y2);
}


bool kicad_pcb_sim::_segments_get_next(std::list<kicad_pcb_sim::segment>& segments, kicad_pcb_sim::segment& s, float x, float y, const std::string& layer_name)
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



float kicad_pcb_sim::_calc_segment_r(const segment& s)
{
    //R = ρ * l / (w * h)  ρ:电导率(cu:0.0172) l:长度(m) w:线宽(mm) h:线厚度(mm) R:电阻(欧姆)
    float l = sqrt((s.start.x - s.end.x) * (s.start.x - s.end.x) + (s.start.y - s.end.y) * (s.start.y - s.end.y));
    
    return _resistivity * l * 0.001 / (s.width * _get_layer_thickness(s.layer_name));
}


float kicad_pcb_sim::_calc_segment_l(const segment& s)
{
    float l = sqrt((s.start.x - s.end.x) * (s.start.x - s.end.x) + (s.start.y - s.end.y) * (s.start.y - s.end.y));
    return 2 * l * (log(2 * l / s.width) + 0.5 + 0.2235 * s.width / l);
}


float kicad_pcb_sim::_calc_via_l(const via& v, const std::string& layer_name1, const std::string& layer_name2)
{
    float h = _get_layer_distance(layer_name1, layer_name2);
    return h / 5 * (1 + log(4.0 * h / v.drill));
}



#define IMG_RATIO (10.0)
#define IMG_RATIO_R (0.1)
#define IMG_ROWS (210.0 * IMG_RATIO)
#define IMG_COLS (297.0 * IMG_RATIO)
#define GRID_SIZE (1 * IMG_RATIO)

void kicad_pcb_sim::_get_zone_cond(const kicad_pcb_sim::zone& z, std::list<cond>& conds, std::set<kicad_pcb_sim::pcb_point>& points)
{
    std::uint32_t w_n = IMG_COLS / GRID_SIZE;
    std::uint32_t h_n = IMG_ROWS / GRID_SIZE;
    float h = _get_layer_thickness(z.layer_name);
    cv::Mat img(IMG_ROWS, IMG_COLS, CV_8UC1);
    cv::Mat img2(IMG_ROWS, IMG_COLS, CV_8UC3);
    std::vector<cv::Point> pts;
    for (auto& i : z.pts)
    {
        cv::Point p(i.x * IMG_RATIO, i.y * IMG_RATIO);
        pts.push_back(p);
    }
    cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(255, 255, 255));
    
    for (std::uint32_t y = 0; y < h_n; y++)
    {
        for (std::uint32_t x = 0; x < w_n; x++)
        {
            std::uint32_t x1 = x * IMG_COLS / w_n;
            std::uint32_t y1 = y * IMG_ROWS / h_n;
            std::uint32_t x2 = (x + 1) * IMG_COLS / w_n;
            std::uint32_t y2 = (y + 1) * IMG_ROWS / h_n;
            
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
            
            if (count >= (std::uint32_t)roi.rows * roi.cols * 0.3)
            {
                cv::line(img2, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 255));
            
                pcb_point p;
                cond c;
                c.start.x = x1 * IMG_RATIO_R;
                c.start.y = y1 * IMG_RATIO_R;
                c.end.x =x2 * IMG_RATIO_R;
                c.end.y = y1 * IMG_RATIO_R;
                c.w = (y2 - y1) * IMG_RATIO_R;
                c.h = h;
                conds.push_back(c);
                p.x = c.start.x;
                p.y = c.start.y;
                points.insert(p);
                
                p.x = c.end.x;
                p.y = c.end.y;
                points.insert(p);
                
                c.start.x = x1 * IMG_RATIO_R;
                c.start.y = y1 * IMG_RATIO_R;
                c.end.x = x1 * IMG_RATIO_R;
                c.end.y = y2 * IMG_RATIO_R;
                c.w = (x2 - x1) * IMG_RATIO_R;
                c.h = h;
                conds.push_back(c);
                
                p.x = c.start.x;
                p.y = c.start.y;
                points.insert(p);
                
                p.x = c.end.x;
                p.y = c.end.y;
                points.insert(p);
            }
        }
    }
}


void kicad_pcb_sim::_create_refs_mat(std::vector<std::uint32_t> refs_id, std::map<std::string, cv::Mat>& refs_mat)
{
    for (auto ref_id: refs_id)
    {
        std::list<kicad_pcb_sim::zone> zones = get_zones(ref_id);
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
            //cv::resize(refs_mat[s.layer_name], tmp, cv::Size(1280, 960));
            //cv::namedWindow("mat", cv::WINDOW_NORMAL);
            //cv::imshow("mat", refs_mat[zone.layer_name]);
            //cv::waitKey();
        }
        
        std::vector<std::list<kicad_pcb_sim::segment> > segments = get_segments_sort(ref_id);
        for (auto& segment: segments)
        {
            for (auto& s: segment)
            {
                if (refs_mat.count(s.layer_name) == 0)
                {
                    cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC1, cv::Scalar(0, 0, 0));
                    refs_mat.emplace(s.layer_name, img);
                }
                
                cv::line(refs_mat[s.layer_name],
                    cv::Point(_cvt_img_x(s.start.x), _cvt_img_y(s.start.y)),
                    cv::Point(_cvt_img_x(s.end.x), _cvt_img_y(s.end.y)),
                    cv::Scalar(255, 255, 255), _cvt_img_len(s.width), cv::LINE_4);
                    
            }
            
        }
    }
    
}


std::list<std::pair<float, float> > kicad_pcb_sim::_get_mat_line(cv::Mat& img, float x1, float y1, float x2, float y2)
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
    float angle = _calc_angle(x1, y1, x2, y2);
    
    bool flag = false;
    float start = 0;
    float end = -1;
    
    for (float l = 0; l < len; l += 0.01)
    {
        float x = x1 + l * cos(angle);
        float y = y1 + l * sin(angle);
        
        if (img.at<std::uint8_t>(_cvt_img_y(y), _cvt_img_x(x)) > 0)
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


std::string kicad_pcb_sim::_gen_segment_zo_ckt(const std::string& cir_name, kicad_pcb_sim::segment& s, std::map<std::string, cv::Mat>& refs_mat)
{
    std::string cir;
    float s_len = sqrt((s.start.x - s.end.x) * (s.start.x - s.end.x) + (s.start.y - s.end.y) * (s.start.y - s.end.y));
    float s_w = s.width;
    float angle = _calc_angle(s.start.x, s.start.y, s.end.x, s.end.y);
    float rad_left = angle + (float)M_PI_2;
    float rad_right = angle - (float)M_PI_2;
    
    
    float begin = 0;
    float last_Z0 = -100;
    float last_v;
    float last_c;
    float last_l;
    float i;
    int pin = 1;
    int idx = 1;
    char strbuf[512];
    
#if DBG_IMG
    cv::Mat img(_get_pcb_img_rows(), _get_pcb_img_cols(), CV_8UC1, cv::Scalar(0, 0, 0));
    
    cv::line(img, cv::Point(_cvt_img_x(s.start.x), _cvt_img_y(s.start.y)),
                    cv::Point(_cvt_img_x(s.end.x), _cvt_img_y(s.end.y)),
                    cv::Scalar(255, 255, 255), _cvt_img_len(s.width), cv::LINE_4);
                    
#endif
    std::vector<std::string> layers = _get_all_dielectric_layer();
    float box_w = s.width * _Z0_w_ratio;
    float box_h = _get_board_thickness() * _Z0_h_ratio;
    float box_y_offset = _get_board_thickness() * - 0.5;
    
    float sl = 0;
    float sr = 0;
    (void)sl;
    fasthenry::calc_wire_lr(s.width, _get_layer_thickness(s.layer_name), 1000, sl, sr);
    _atlc.clean_all();
    for (i = 0; i < s_len; i += _Z0_setup)
    {
        float x = s.start.x + i * cos(angle);
        float y = s.start.y + i * sin(angle);
        float x_left = x + (s_w * _Z0_w_ratio * 0.5) * cos(rad_left);
        float y_left = y + (s_w * _Z0_w_ratio * 0.5) * sin(rad_left);
        float x_right = x + (s_w * _Z0_w_ratio * 0.5) * cos(rad_right);
        float y_right = y + (s_w * _Z0_w_ratio * 0.5) * sin(rad_right);
        
    #if DBG_IMG
        cv::circle(img, cv::Point(_cvt_img_x(x), _cvt_img_y(y)), 2, cv::Scalar(100, 0, 0));
        cv::line(img, cv::Point(_cvt_img_x(x_left), _cvt_img_y(y_left)),
                    cv::Point(_cvt_img_x(x_right), _cvt_img_y(y_right)),
                    cv::Scalar(255, 255, 255), 1, cv::LINE_4);
    #endif
        
        
        _atlc.clean();
        _atlc.set_pix_unit(_atlc_pix_unit);
        _atlc.set_box_size(box_w, box_h);
        for (auto& l: layers)
        {
            float y = _get_layer_z_axis(l);
            _atlc.draw_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
        }
        
        for (auto& refs: refs_mat)
        {
            std::list<std::pair<float, float> >  grounds = _get_mat_line(refs.second, x_left, y_left, x_right, y_right);
            
            for (auto& g: grounds)
            {
                _atlc.draw_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
            }
        }
        
        _atlc.draw_wire(0, _get_layer_z_axis(s.layer_name) + box_y_offset, s.width, _get_layer_thickness(s.layer_name));
        
        
        float Zo;
        float v;
        float c;
        float l;
        
        if (!_atlc.calc_zo(Zo, v, c, l) && fabs(last_Z0 - Zo) > 0.1)
        {
            log_debug("Zo:%f v:%fmm/ns c:%f l:%f\n", Zo, v / 1000000, c, l);
            if (i - begin > 0.001)
            {
                float dist = (i - begin);
                printf("dist:%f last_v:%f\n", dist, last_v);
                float td = dist * 1000000 / last_v;
            #if 0
                if (td >= 0.001)
                {
                    sprintf(strbuf, "T%d pin%d 0 pin%d 0 Z0=%f TD=%fNS\n", idx++, pin, pin + 1, last_Z0, td);
                }
                else
                {
                    sprintf(strbuf, "L%d pin%d pin%d %gnH\n", idx++, pin, pin + 1, last_l * dist * 0.001);
                }
            #else
                #if 1
                sprintf(strbuf, "***Z0:%f TD:%fNS***\n"
                                "Y%d pin%d 0 pin%d 0 ymod%d LEN=%f\n"
                                ".MODEL ymod%d txl R=%f L=%fnH G=0 C=%fpF length=1\n",
                                last_Z0, td,
                                idx, pin, pin + 1, idx, dist * 0.001,
                                idx, sr, last_l, last_c
                                );
                #else
                
                sprintf(strbuf, "***Z0:%f TD:%fNS***\n"
                                "O%d pin%d 0 pin%d 0 ltra%d\n"
                                ".MODEL ltra%d LTRA R=%f L=%fnH G=0 C=%fpF LEN=%g\n",
                                last_Z0, td,
                                idx, pin, pin + 1, idx,
                                idx, sr, last_l, last_c, dist * 0.001
                                );
                #endif
                idx++;
            #endif
                pin++;
                cir += strbuf;
            }
            begin = i;
            last_Z0 = Zo;
            last_v = v;
            last_c = c;
            last_l = l;
        }
        
    }
    if (i > s_len)
    {
        i = s_len;
    }
    if (i - begin >= 0.001)
    {
        float dist = (i - begin);
        printf("dist:%f last_v:%f\n", dist, last_v);
        float td = dist * 1000000 / last_v;
    #if 0
        if (td > 0.001)
        {
            sprintf(strbuf, "T%d pin%d 0 pin%d 0 Z0=%f TD=%fNS\n", idx++, pin, pin + 1, last_Z0, dist * 1000000 / last_v);
        }
        else
        {
            sprintf(strbuf, "L%d pin%d pin%d %gnH\n", idx++, pin, pin + 1, last_l * dist * 0.001);
        }
    #else
        #if 1
        sprintf(strbuf, "***Z0:%f TD:%fNS***\n"
                        "Y%d pin%d 0 pin%d 0 ymod%d LEN=%f\n"
                        ".MODEL ymod%d txl R=%f L=%fnH G=0 C=%fpF length=1\n",
                        last_Z0, td,
                        idx, pin, pin + 1, idx, dist * 0.001,
                        idx, sr, last_l, last_c
                        );
        #else
        sprintf(strbuf, "***Z0:%f TD:%fNS***\n"
                        "O%d pin%d 0 pin%d 0 ltra%d\n"
                        ".MODEL ltra%d LTRA R=%f L=%fnH G=0 C=%fpF LEN=%g\n",
                        last_Z0, td,
                        idx, pin, pin + 1, idx,
                        idx, sr, last_l, last_c, dist * 0.001
                        );
        #endif
        idx++;
    #endif
        pin++;
        cir += strbuf;
    }
    cir += ".ends\n";
    sprintf(strbuf, ".subckt %s pin1 pin%d\n", cir_name.c_str(), pin);
    return  strbuf + cir;
}


std::string kicad_pcb_sim::_gen_segment_coupled_zo_ckt(const std::string& cir_name, kicad_pcb_sim::segment& s0, kicad_pcb_sim::segment& s1, std::map<std::string, cv::Mat>& refs_mat)
{
    std::string cir;
    if (_calc_dist(s0.start.x, s0.start.y, s1.start.x, s1.start.y) > _calc_dist(s0.start.x, s0.start.y, s1.end.x, s1.end.y))
    {
        std::swap(s1.start, s1.end);
    }
    float ss_start_x = (s0.start.x + s1.start.x) * 0.5;
    float ss_start_y = (s0.start.y + s1.start.y) * 0.5;
    
    float ss_end_x = (s0.end.x + s1.end.x) * 0.5;
    float ss_end_y = (s0.end.y + s1.end.y) * 0.5;
    
    float ss_len = sqrt((ss_start_x - ss_end_x) * (ss_start_x - ss_end_x) + (ss_start_y - ss_end_y) * (ss_start_y - ss_end_y));
    float ss_dist = _calc_dist(s0.start.x, s0.start.y, s1.start.x, s1.start.y);
    
    bool s0_is_left = ((ss_start_y - ss_end_y) * s0.start.x + (ss_end_x - ss_start_x) * s0.start.y + ss_start_x * ss_end_y - ss_end_x * ss_start_y) > 0;

    
    float angle = _calc_angle(ss_start_x, ss_start_y, ss_end_x, ss_end_y);
    float rad_left = angle + (float)M_PI_2;
    float rad_right = angle - (float)M_PI_2;
    
    
    char strbuf[512];
    
    
    std::vector<std::string> layers = _get_all_dielectric_layer();
    float box_w = ss_dist * _Z0_w_ratio;
    float box_h = _get_board_thickness() * _Z0_h_ratio;
    float box_y_offset = _get_board_thickness() * - 0.5;
    
    float tmp_l = 0;
    float s0_r = 0;
    float s1_r = 0;
    (void)tmp_l;
    fasthenry::calc_wire_lr(s0.width, _get_layer_thickness(s0.layer_name), 1000, tmp_l, s0_r);
    fasthenry::calc_wire_lr(s1.width, _get_layer_thickness(s1.layer_name), 1000, tmp_l, s1_r);
    _atlc.clean_all();
    _atlc1.clean_all();
    _atlc_coupled.clean_all();
    
    
    struct Z0_item
    {
        float Lodd;
        float Leven;
        float Codd;
        float Ceven;
    
        float Zodd;
        float Zeven;
        float Zdiff;
        float Zcomm;
        float Z0;
        float v;
        float c;
        float l;
    };
    std::vector<Z0_item> s0_Z0s;
    std::vector<Z0_item> s1_Z0s;
    std::vector<Z0_item> ss_Z0s;
    
    for (float i = 0; i < ss_len; i += _Z0_setup)
    {
        float x = ss_start_x + i * cos(angle);
        float y = ss_start_y + i * sin(angle);
        float x_left = x + (ss_dist * _Z0_w_ratio * 0.5) * cos(rad_left);
        float y_left = y + (ss_dist * _Z0_w_ratio * 0.5) * sin(rad_left);
        float x_right = x + (ss_dist * _Z0_w_ratio * 0.5) * cos(rad_right);
        float y_right = y + (ss_dist * _Z0_w_ratio * 0.5) * sin(rad_right);
        
        
        _atlc.clean();
        _atlc.set_pix_unit(_atlc_pix_unit);
        _atlc.set_box_size(box_w, box_h);
        
        _atlc1.clean();
        _atlc1.set_pix_unit(_atlc_pix_unit);
        _atlc1.set_box_size(box_w, box_h);
        
        _atlc_coupled.clean();
        _atlc_coupled.set_pix_unit(_atlc_pix_unit);
        _atlc_coupled.set_box_size(box_w, box_h);
        
        for (auto& l: layers)
        {
            float y = _get_layer_z_axis(l);
            _atlc.draw_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
            _atlc1.draw_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
            _atlc_coupled.draw_elec(0, y + box_y_offset, box_w, _get_layer_thickness(l), _get_layer_epsilon_r(l));
        }
        
        for (auto& refs: refs_mat)
        {
            std::list<std::pair<float, float> > grounds = _get_mat_line(refs.second, x_left, y_left, x_right, y_right);
            
            for (auto& g: grounds)
            {
                _atlc.draw_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
                _atlc1.draw_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
                _atlc_coupled.draw_ground(g.first, _get_layer_z_axis(refs.first) + box_y_offset, g.second, _get_layer_thickness(refs.first));
            }
        }
        
        if (s0_is_left)
        {
            _atlc.draw_wire(0 - ss_dist * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name));
            _atlc1.draw_wire(0 + ss_dist * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name));
            
            _atlc_coupled.draw_wire(0 - ss_dist * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name));
            _atlc_coupled.draw_coupler(0 + ss_dist * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name));
        }
        else
        {
            _atlc.draw_wire(0 + ss_dist * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name));
            _atlc1.draw_wire(0 - ss_dist * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name));
            
            _atlc_coupled.draw_wire(0 + ss_dist * 0.5, _get_layer_z_axis(s0.layer_name) + box_y_offset, s0.width, _get_layer_thickness(s0.layer_name));
            _atlc_coupled.draw_coupler(0 - ss_dist * 0.5, _get_layer_z_axis(s1.layer_name) + box_y_offset, s1.width, _get_layer_thickness(s1.layer_name));
        }
        
        
        Z0_item s0_item;
        Z0_item s1_item;
        Z0_item ss_item;
        
        
        _atlc.calc_zo(s0_item.Z0, s0_item.v, s0_item.c, s0_item.l);
        _atlc1.calc_zo(s1_item.Z0, s1_item.v, s1_item.c, s1_item.l);
        
        _atlc_coupled.calc_coupled_zo(ss_item.Zodd, ss_item.Zeven, ss_item.Zdiff, ss_item.Zcomm, ss_item.Lodd, ss_item.Leven, ss_item.Codd, ss_item.Ceven);
        s0_Z0s.push_back(s0_item);
        s1_Z0s.push_back(s1_item);
        ss_Z0s.push_back(ss_item);
    }
    
    float ss_Zodd = 0;
    float ss_Zeven = 0;
    float ss_Zdiff = 0;
    float ss_Zcomm = 0;
    float ss_Lodd = 0;
    float ss_Leven = 0;
    float ss_Codd = 0;
    float ss_Ceven = 0;
    
    for (auto& item: ss_Z0s)
    {
        ss_Zodd += item.Zodd;
        ss_Zeven += item.Zeven;
        ss_Zdiff += item.Zdiff;
        ss_Zcomm += item.Zcomm;
        
        ss_Lodd += item.Lodd;
        ss_Leven += item.Leven;
        ss_Codd += item.Codd;
        ss_Ceven += item.Ceven;
    }
    
    ss_Zodd = ss_Zodd / ss_Z0s.size();
    ss_Zeven = ss_Zeven / ss_Z0s.size();
    ss_Zdiff = ss_Zdiff / ss_Z0s.size();
    ss_Zcomm = ss_Zcomm / ss_Z0s.size();
    
    ss_Lodd = ss_Lodd / ss_Z0s.size();
    ss_Leven = ss_Leven / ss_Z0s.size();
    ss_Codd = ss_Codd / ss_Z0s.size();
    ss_Ceven = ss_Ceven / ss_Z0s.size();
    
    
    
    float s0_Z0 = 0;
    float s0_v = 0;
    float s0_c = 0;
    float s0_l = 0;
    
    for (auto& item: s0_Z0s)
    {
        s0_Z0 += item.Z0;
        s0_v += item.v;
        s0_c += item.c;
        s0_l += item.l;
    }
    
    s0_Z0 = s0_Z0 / s0_Z0s.size();
    s0_v = s0_v / s0_Z0s.size();
    s0_c = s0_c / s0_Z0s.size();
    s0_l = s0_l / s0_Z0s.size();
    
    float s1_Z0 = 0;
    float s1_v = 0;
    float s1_c = 0;
    float s1_l = 0;
    
    for (auto& item: s1_Z0s)
    {
        s1_Z0 += item.Z0;
        s1_v += item.v;
        s1_c += item.c;
        s1_l += item.l;
    }
    
    s1_Z0 = s1_Z0 / s1_Z0s.size();
    s1_v = s1_v / s1_Z0s.size();
    s1_c = s1_c / s1_Z0s.size();
    s1_l = s1_l / s1_Z0s.size();

    sprintf(strbuf, "***Zodd:%f Zeven:%f Zdiff:%f Zcomm:%f***\n"
                    "P1 pin1 pin3 0 pin2 pin4 0 PLINE\n"
                    ".model PLINE CPL length=%f\n"
                    "+R=%f 0 %f\n"
                    "+L=%fnH %fnH %fnH\n"
                    "+G=0 0 0\n"
                    "+C=%fpF %fpF %fpF\n",
                    ss_Zodd, ss_Zeven, ss_Zdiff, ss_Zcomm,
                    ss_len * 0.001,
                    s0_r, s1_r,
                    s0_l, (ss_Leven - ss_Lodd) / 2, s1_l,
                    s0_c, (ss_Ceven - ss_Codd) / 2, s1_c);
    cir += strbuf;
    
    cir += ".ends\n";
    sprintf(strbuf, ".subckt %s pin1 pin2  pin3 pin4\n", cir_name.c_str());
    return  strbuf + cir;
}

float kicad_pcb_sim::_calc_angle(float ax, float ay, float bx, float by, float cx, float cy)
{
    float theta = atan2(ax - cx, ay - cy) - atan2(bx - cx, by - cy);
    if (theta > M_PI)
    {
        theta -= 2 * M_PI;
    }
    if (theta < -M_PI)
    {
        theta += 2 * M_PI;
    }
    return fabs(theta);
}

float kicad_pcb_sim::_calc_p2line_dist(float x1, float y1, float x2, float y2, float x, float y)
{
    float angle = _calc_angle(x2, y2, x, y, x1, y1);
    float d = sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));
    if (angle > (float)M_PI_2)
    {
        angle = (float)M_PI - angle;
    }
    if (fabs(angle - (float)M_PI_2) < 0.00001)
    {
        return d;
    }
    return sin(angle) * d;
}


bool kicad_pcb_sim::_calc_p2line_intersection(float x1, float y1, float x2, float y2, float x, float y, float& ix, float& iy)
{
    float line_angle = _calc_angle(x1, y1, x2, y2);
    float angle = _calc_angle(x2, y2, x, y, x1, y1);
    float d = sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));
    float line_d = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
    
    if (angle > (float)M_PI_2)
    {
        return false;
    }
    float len = cos(angle) * d;
    if (len > line_d)
    {
        return false;
    }
    
    ix = x1 + len * cos(line_angle);
    iy = y1 + len * sin(line_angle);
    return true;
}

bool kicad_pcb_sim::_calc_parallel_lines_overlap(float ax1, float ay1, float ax2, float ay2,
                                                float bx1, float by1, float bx2, float by2,
                                                float& aox1, float& aoy1, float& aox2, float& aoy2,
                                                float& box1, float& boy1, float& box2, float& boy2)
{
    std::vector<std::pair<float, float> > ao;
    std::vector<std::pair<float, float> > bo;
    
    float x;
    float y;
    
    /* 计算过点(ax1, ay1)到线段b垂线的交点 */
    if (_calc_p2line_intersection(bx1, by1, bx2, by2, ax1, ay1, x, y))
    {
        ao.push_back(std::pair<float, float>(ax1, ay1));
        bo.push_back(std::pair<float, float>(x, y));
    }
    else
    {
        /* 计算过点(bx1, by1)到线段a垂线的交点 */
        if (_calc_p2line_intersection(ax1, ay1, ax2, ay2, bx1, by1, x, y))
        {
            bo.push_back(std::pair<float, float>(bx1, by1));
            ao.push_back(std::pair<float, float>(x, y));
        }
    }
    
    
    /* 计算过点(ax2, ay2)到线段b垂线的交点 */
    if (_calc_p2line_intersection(bx1, by1, bx2, by2, ax2, ay2, x, y))
    {
        ao.push_back(std::pair<float, float>(ax2, ay2));
        bo.push_back(std::pair<float, float>(x, y));
    }
    else
    {
        /* 计算过点(bx2, by2)到线段a垂线的交点 */
        if (_calc_p2line_intersection(ax1, ay1, ax2, ay2, bx2, by2, x, y))
        {
            bo.push_back(std::pair<float, float>(bx2, by2));
            ao.push_back(std::pair<float, float>(x, y));
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
    float alen = (aox2 - aox1) * (aox2 - aox1) + (aoy2 - aoy1) * (aoy2 - aoy1);
    float blen = (box2 - box1) * (box2 - box1) + (boy2 - boy1) * (boy2 - boy1);
    return fabs(alen - blen) < 0.0001;
}
               
    
float kicad_pcb_sim::_calc_parallel_lines_overlap_len(float ax1, float ay1, float ax2, float ay2,
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
    if (_calc_parallel_lines_overlap(ax1, ay1, ax2, ay2,
                                    bx1, by1, bx2, by2,
                                    aox1, aoy1, aox2, aoy2,
                                    box1, boy1, box2, boy2))
    {
        return sqrt((aox2 - aox1) * (aox2 - aox1) + (aoy2 - aoy1) * (aoy2 - aoy1));
    }
    return 0;
}

bool kicad_pcb_sim::_is_coupled(const kicad_pcb_sim::segment& s1, const kicad_pcb_sim::segment& s2, float coupled_max_d, float coupled_min_len)
{
    float a1 = _calc_angle(s1.start.x, s1.start.y, s1.end.x, s1.end.y);
    float a2 = _calc_angle(s2.start.x, s2.start.y, s2.end.x, s2.end.y);
    float a22 = _calc_angle(s2.end.x, s2.end.y, s2.start.x, s2.start.y);
    if (fabs(a1 - a2) > 0.0001 && fabs(a1 - a22) > 0.0001)
    {
        return false;
    }
    float dist = _calc_p2line_dist(s1.start.x, s1.start.y, s1.end.x, s1.end.y, s2.start.x, s2.start.y);
    if (dist > coupled_max_d)
    {
        return false;
    }
    
    float ovlen = _calc_parallel_lines_overlap_len(s1.start.x, s1.start.y, s1.end.x, s1.end.y,
                                        s2.start.x, s2.start.y, s2.end.x, s2.end.y);
    if (ovlen < coupled_min_len)
    {
        return false;
    }
    return true;
}



void kicad_pcb_sim::_split_segment(const kicad_pcb_sim::segment& s, std::list<kicad_pcb_sim::segment>& ss, float x1, float y1, float x2, float y2)
{
    float d1 = _calc_dist(x1, y1, s.start.x, s.start.y);
    float d2 = _calc_dist(x2, y2, s.start.x, s.start.y);
    
    std::uint32_t idx = 0;
    std::vector<pcb_point> ps;
    ps.push_back(s.start);
    if (d1 < d2)
    {
        if (d1 > _Z0_setup)
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
        float d = _calc_dist(x2, y2, s.end.x, s.end.y);
        if (d > _Z0_setup)
        {
            pcb_point p;
            p.x = x2;
            p.y = y2;
            ps.push_back(p);
        }
    }
    else
    {
        if (d2 > _Z0_setup)
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
        float d = _calc_dist(x1, y1, s.end.x, s.end.y);
        if (d > _Z0_setup)
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
        kicad_pcb_sim::segment tmp = s;
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
