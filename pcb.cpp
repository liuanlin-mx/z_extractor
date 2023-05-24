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

#include "calc.h"
#include "pcb.h"

#if 0
#define log_debug(fmt, args...) printf(fmt, ##args)
#else
#define log_debug(fmt, args...)
#endif

#define log_info(fmt, args...) printf(fmt, ##args)


pcb::pcb()
    : _pcb_top(10000.)
    , _pcb_bottom(0)
    , _pcb_left(10000.)
    , _pcb_right(0)
    , _float_epsilon(0.00005)
{
    
}

pcb::~pcb()
{
}

bool pcb::add_net(std::uint32_t id, std::string name)
{
    _nets.emplace(id, name);
    return true;
}


bool pcb::add_segment(const segment& s)
{
    _segments.emplace(s.net, s);
    return true;
}


bool pcb::add_via(const via& v)
{
    _vias.emplace(v.net, v);
    return true;
}


bool pcb::add_zone(const zone& z)
{
    _zones.emplace(z.net, z);
    return true;
}


bool pcb::add_footprint(const footprint& f)
{
    _footprints.push_back(f);
    for (const auto& p: f.pads)
    {
        _pads.emplace(p.net, p);
    }
    return true;
}

bool pcb::add_pad(const pad& p)
{
    _pads.emplace(p.net, p);
    return true;
}


bool pcb::add_layer(const layer& l)
{
    _layers.push_back(l);
    return true;
}

bool pcb::add_gr(const gr& g)
{
    _grs.push_back(g);
    return true;
}


void pcb::set_edge(float top, float bottom, float left, float right)
{
    _pcb_top = top;
    _pcb_bottom = bottom;
    _pcb_left = left;
    _pcb_right = right;
}


void pcb::dump()
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

cv::Mat pcb::draw(const std::string& layer_name, float pix_unit)
{
    cv::Mat img(_get_pcb_img_rows(pix_unit), _get_pcb_img_cols(pix_unit), CV_8UC3, cv::Scalar(0, 0, 0));
    for (const auto& it: _segments)
    {
        const segment& s = it.second;
        if (s.layer_name == layer_name)
        {
            _draw_segment(img, s, 0, 0, 255, pix_unit);
        }
    }
    
    for (const auto& it: _vias)
    {
        const via& v = it.second;
        _draw_via(img, v, layer_name, 0, 0, 255, pix_unit);
    }
    
    for (const auto& it: _zones)
    {
        const zone& z = it.second;
        if (z.layer_name == layer_name)
        {
            _draw_zone(img, z, 0, 0, 255, pix_unit);
        }
    }
    
    
    
    for (const auto& gr: _grs)
    {
        if (gr.layer_name == layer_name)
        {
            pcb::point at;
            at.x = 0;
            at.y = 0;
            _draw_gr(img, gr, at, 0, 0, 0, 255, pix_unit);
        }
    }
    
    
    for (const auto& fp: _footprints)
    {
        //if (fp.layer_name == layer_name)
        {
            _draw_fp(img, fp, layer_name, 0, 0, 255, pix_unit);
        }
    }
    return img;
}

/* 如果线段在焊盘内但没有连接到中心点 就添加一个等宽线段连接到中心 */
/* 如果两个线段的起点或终点没有连接在一点但距离小于线宽和的1/2 就添加一个等宽线段连接它们 */
void pcb::clean_segment()
{
    for (const auto& net: _nets)
    {
        std::uint32_t net_id = net.first;
        
        std::list<pad> pads = get_pads(net_id);
        std::list<std::pair<std::uint32_t, pcb::segment> > no_conn;
        std::list<pcb::segment> conn;
        get_no_conn_segments(net_id, no_conn, conn);
        
        for (auto it = no_conn.begin(); it != no_conn.end();)
        {
            auto& s = *it;
            for (const auto& p: pads)
            {
                std::uint32_t flag = segment_is_inside_pad(s.second, p);
                if ((s.first & 0x01) && (flag & 0x01))
                {
                    float x;
                    float y;
                    get_pad_pos(p, x, y);
                    pcb::segment new_s;
                    new_s = s.second;
                    new_s.end.x = x;
                    new_s.end.y = y;
                    new_s.tstamp = "s" + new_s.tstamp;
                    _segments.emplace(new_s.net, new_s);
                    s.first &= ~0x01;
                }
                if ((s.first & 0x02) && (flag & 0x02))
                {
                    float x;
                    float y;
                    get_pad_pos(p, x, y);
                    pcb::segment new_s;
                    new_s = s.second;
                    new_s.start.x = x;
                    new_s.start.y = y;
                    new_s.tstamp = "e" + new_s.tstamp;
                    _segments.emplace(new_s.net, new_s);
                    s.first &= ~0x02;
                }
            }
            if (s.first == 0)
            {
                it = no_conn.erase(it);
                continue;
            }
            it++;
        }
        
        while (!no_conn.empty())
        {
            auto s = no_conn.front();
            no_conn.pop_front();
            
            for (auto it = no_conn.begin(); it != no_conn.end(); it++)
            {
                if (s.first & 0x01)
                {
                    if (it->first & 0x01)
                    {
                        if (calc_dist(s.second.start.x, s.second.start.y, it->second.start.x, it->second.start.y)
                                    < s.second.width * 0.5 + it->second.width * 0.5)
                        {
                            pcb::segment new_s;
                            new_s = s.second;
                            new_s.end.x = it->second.start.x;
                            new_s.end.y = it->second.start.y;
                            new_s.tstamp = "s" + new_s.tstamp;
                            _segments.emplace(new_s.net, new_s);
                            s.first &= ~0x01;
                            it->first &= ~0x01;
                        }
                    }
                    if (it->first & 0x02)
                    {
                        if (calc_dist(s.second.start.x, s.second.start.y, it->second.end.x, it->second.end.y)
                                    < s.second.width * 0.5 + it->second.width * 0.5)
                        {
                            pcb::segment new_s;
                            new_s = s.second;
                            new_s.end.x = it->second.end.x;
                            new_s.end.y = it->second.end.y;
                            new_s.tstamp = "s" + new_s.tstamp;
                            _segments.emplace(new_s.net, new_s);
                            s.first &= ~0x01;
                            it->first &= ~0x02;
                        }
                    }
                }
                
                if (s.first & 0x02)
                {
                    if (it->first & 0x01)
                    {
                        if (calc_dist(s.second.end.x, s.second.end.y, it->second.start.x, it->second.start.y)
                                    < s.second.width * 0.5 + it->second.width * 0.5)
                        {
                            pcb::segment new_s;
                            new_s = s.second;
                            new_s.start.x = it->second.start.x;
                            new_s.start.y = it->second.start.y;
                            new_s.tstamp = "e" + new_s.tstamp;
                            _segments.emplace(new_s.net, new_s);
                            s.first &= ~0x02;
                            it->first &= ~0x01;
                        }
                    }
                    if (it->first & 0x02)
                    {
                        if (calc_dist(s.second.end.x, s.second.end.y, it->second.end.x, it->second.end.y)
                                    < s.second.width * 0.5 + it->second.width * 0.5)
                        {
                            pcb::segment new_s;
                            new_s = s.second;
                            new_s.start.x = it->second.end.x;
                            new_s.start.y = it->second.end.y;
                            new_s.tstamp = "e" + new_s.tstamp;
                            _segments.emplace(new_s.net, new_s);
                            s.first &= ~0x02;
                            it->first &= ~0x02;
                        }
                    }
                }
            }
        }
    }
}


std::list<pcb::segment> pcb::get_segments(std::uint32_t net_id)
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


const std::vector<pcb::footprint>& pcb::get_footprints()
{
    return _footprints;
}

bool pcb::get_footprint(const std::string& fp_ref, footprint& fp)
{
    for (const auto& fp_: _footprints)
    {
        if (fp_.reference == fp_ref)
        {
            fp = fp_;
            return true;
        }
    }
    return false;
}

std::list<pcb::pad> pcb::get_pads(std::uint32_t net_id)
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


bool pcb::get_pad(const std::string& footprint, const std::string& pad_number, pcb::pad& pad)
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


std::list<pcb::via> pcb::get_vias(std::uint32_t net_id)
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

std::list<pcb::via> pcb::get_vias(const std::vector<std::uint32_t>& net_ids)
{
    std::list<via> vias;
    for (const auto& net_id: net_ids)
    {
        auto v = _vias.equal_range(net_id);
        if(v.first != std::end(_vias))
        {
            for (auto it = v.first; it != v.second; ++it)
            {
                vias.push_back(it->second);
            }
        }
    }
    return vias;
}

std::list<pcb::zone> pcb::get_zones(std::uint32_t net_id)
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


std::vector<std::list<pcb::segment> > pcb::get_segments_sort(std::uint32_t net_id)
{
    std::vector<std::list<pcb::segment> > v_segments;
    std::list<pcb::segment> segments = get_segments(net_id);
    while (segments.size())
    {
        std::list<pcb::segment> s_list;
        pcb::segment first = segments.front();
        segments.pop_front();
        s_list.push_back(first);
        pcb::segment tmp = first;
        pcb::segment next;
        
        while (segments_get_next(segments, next, tmp.start.x, tmp.start.y, tmp.layer_name))
        {
            if (_point_equal(next.start.x, next.start.y, tmp.start.x, tmp.start.y))
            {
                std::swap(next.start, next.end);
            }
            tmp = next;
            s_list.push_front(next);
        }
        
        tmp = first;
        
        while (segments_get_next(segments, next, tmp.end.x, tmp.end.y, tmp.layer_name))
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

std::string pcb::get_net_name(std::uint32_t net_id)
{
    if (_nets.count(net_id))
    {
        return _nets[net_id];
    }
    return "";
}

std::uint32_t pcb::get_net_id(std::string name)
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



void pcb::get_pad_pos(const pad& p, float& x, float& y)
{
    float angle = p.ref_at_angle * M_PI / 180;
    float at_x = p.at.x;
    float at_y = -p.at.y;
                
    float x1 = cosf(angle) * at_x - sinf(angle) * at_y;
    float y1 = sinf(angle) * at_x + cosf(angle) * at_y;
    
    x = p.ref_at.x + x1;
    y = p.ref_at.y - y1;
}

void pcb::get_rotation_pos(const point& c, float rotate_angle, point& p)
{
    float angle = rotate_angle * M_PI / 180;
    float at_x = p.x;
    float at_y = -p.y;
                
    float x1 = cosf(angle) * at_x - sinf(angle) * at_y;
    float y1 = sinf(angle) * at_x + cosf(angle) * at_y;
    
    p.x = c.x + x1;
    p.y = c.y - y1;
}

std::string pcb::get_tstamp_short(const std::string& tstamp)
{
    size_t pos = tstamp.find('-');
    if (pos != tstamp.npos)
    {
        return tstamp.substr(0, pos);
    }
    return tstamp;
}


std::string pcb::format_net(const std::string& name)
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

std::string pcb::pos2net(float x, float y, const std::string& layer)
{
    char buf[128] = {0};
    sprintf(buf, "%d_%d", std::int32_t(x * 1000), std::int32_t(y * 1000));
    
    return std::string(buf) + format_net(layer);
}


std::string pcb::format_net_name(const std::string& net_name)
{
    return "NET_" + format_net(net_name);
}


std::string pcb::format_layer_name(std::string layer_name)
{
    return format_net(layer_name);
}


std::string pcb::gen_pad_net_name(const std::string& footprint, const std::string& net_name)
{
    char buf[256] = {0};
    sprintf(buf, "%s_%s", footprint.c_str(), net_name.c_str());
    return std::string(buf);
}



std::vector<std::string> pcb::get_all_cu_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != pcb::layer::COPPER)
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}

std::vector<std::string> pcb::get_all_dielectric_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != pcb::layer::CORE
            && l.type != pcb::layer::PREPREG
            && l.type != pcb::layer::TOP_SOLDER_MASK
            && l.type != pcb::layer::BOTTOM_SOLDER_MASK)
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}

std::vector<std::string> pcb::get_all_mask_layer()
{
    std::vector<std::string> layers;
    for (auto& l: _layers)
    {
        if (l.type != pcb::layer::TOP_SOLDER_MASK && l.type != pcb::layer::BOTTOM_SOLDER_MASK)
        {
            continue;
        }
        layers.push_back(l.name);
    }
    return layers;
}


std::vector<std::string> pcb::get_via_layers(const via& v)
{
    std::vector<std::string> layers;
    bool flag = false;
    for (auto& l: _layers)
    {
        if (l.type != pcb::layer::COPPER)
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


std::vector<std::string> pcb::get_via_conn_layers(const pcb::via& v)
{
    std::vector<std::string> layers;
    std::set<std::string> layer_set;
    std::list<pcb::segment> segments = get_segments(v.net);
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


float pcb::get_via_conn_len(const pcb::via& v)
{
    float min_z = 1;
    float max_z = -1;
    float len = 0;
    std::vector<std::string> conn_layers = get_via_conn_layers(v);
    for (std::int32_t i = 0; i < (std::int32_t)conn_layers.size(); i++)
    {
        const std::string& layer_name = conn_layers[i];
        float z = get_layer_z_axis(layer_name);
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

bool pcb::is_cu_layer(const std::string& layer)
{
    for (const auto& l: _layers)
    {
        if (layer == l.name)
        {
            return l.type == pcb::layer::COPPER;
        }
    }
    return false;
}

std::vector<std::string> pcb::get_pad_conn_layers(const pcb::pad& p)
{
    float x;
    float y;
    get_pad_pos(p, x, y);
    
    std::set<std::string> layer_set;
    std::list<pcb::segment> segments = get_segments(p.net);
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
            layers_tmp = get_all_cu_layer();
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

std::vector<std::string> pcb::get_pad_layers(const pad& p)
{
    std::vector<std::string> layers;
    for (const auto& layer: p.layers)
    {
        if (layer.find("*") != layer.npos)
        {
            layers = get_all_cu_layer();
        }
        else
        {
            layers.push_back(layer);
        }
    }
    return layers;
}



float pcb::get_layer_distance(const std::string& layer_name1, const std::string& layer_name2)
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


float pcb::get_layer_thickness(const std::string& layer_name)
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


float pcb::get_layer_z_axis(const std::string& layer_name)
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

float pcb::get_layer_epsilon_r(const std::string& layer_name)
{
    for (auto& l: _layers)
    {
        if (l.name == layer_name)
        {
            if (l.type == pcb::layer::COPPER)
            {
                return get_cu_layer_epsilon_r(layer_name);
            }
            return l.epsilon_r;
        }
    }
    return 1;
}

/* 取上下两层介电常数的均值 */
float pcb::get_cu_layer_epsilon_r(const std::string& layer_name)
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
    
    
    if (up.type == pcb::layer::TOP_SOLDER_MASK || up.type == pcb::layer::BOTTOM_SOLDER_MASK)
    {
        return up.epsilon_r;
    }
    
    if (down.type == pcb::layer::TOP_SOLDER_MASK || down.type == pcb::layer::BOTTOM_SOLDER_MASK)
    {
        return down.epsilon_r;
    }
    
    return (up.epsilon_r + down.epsilon_r) * 0.5;
}

float pcb::get_layer_epsilon_r(const std::string& layer_start, const std::string& layer_end)
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

float pcb::get_board_thickness()
{
    float dist = 0;
    for (auto& l: _layers)
    {
        dist += l.thickness;
    }
    return dist;
}


float pcb::get_cu_min_thickness()
{
    float thickness = 1;
    for (const auto& l: _layers)
    {
        if (l.type == pcb::layer::COPPER)
        {
            if (l.thickness < thickness)
            {
                thickness = l.thickness;
            }
        }
    }
    return thickness;
}


bool pcb::cu_layer_is_outer_layer(const std::string& layer_name)
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
    
    if (up.type == pcb::layer::TOP_SOLDER_MASK
        || up.type == pcb::layer::BOTTOM_SOLDER_MASK
        || down.type == pcb::layer::TOP_SOLDER_MASK
        || down.type == pcb::layer::BOTTOM_SOLDER_MASK)
    {
        return true;
    }
    return false;
}



bool pcb::check_segments(std::uint32_t net_id)
{
    std::list<std::pair<std::uint32_t, pcb::segment> > no_conn;
    std::list<pcb::segment> conn;
    get_no_conn_segments(net_id, no_conn, conn);
    
    for (const auto& s: no_conn)
    {
        if (s.first & 0x01)
        {
            printf("err: no connection (net:%s x:%.4f y:%.4f).\n", get_net_name(s.second.net).c_str(), s.second.start.x, s.second.start.y);
        }
        if (s.first & 0x02)
        {
            printf("err: no connection (net:%s x:%.4f y:%.4f).\n", get_net_name(s.second.net).c_str(), s.second.end.x, s.second.end.y);
        }
    }
    return no_conn.empty();
}

void pcb::get_no_conn_segments(std::uint32_t net_id, std::list<std::pair<std::uint32_t, pcb::segment> >& no_conn, std::list<pcb::segment>& conn)
{
    std::list<pcb::segment> segments = get_segments(net_id);
    std::list<pcb::pad> pads = get_pads(net_id);
    std::list<pcb::via> vias = get_vias(net_id);
    
    
    while (!segments.empty())
    {
        pcb::segment s = segments.front();
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
            if (it.second.layer_name != s.layer_name)
            {
                continue;
            }
            if (_point_equal(s.start.x, s.start.y, it.second.start.x, it.second.start.y)
                || _point_equal(s.start.x, s.start.y, it.second.end.x, it.second.end.y))
            {
                start = true;
            }
            
            if (_point_equal(s.end.x, s.end.y, it.second.start.x, it.second.start.y)
                || _point_equal(s.end.x, s.end.y, it.second.end.x, it.second.end.y))
            {
                end = true;
            }
        }
        
        for (const auto&p: pads)
        {
            std::vector<std::string> layers = get_pad_conn_layers(p);
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
            get_pad_pos(p, x, y);
            
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
            std::vector<std::string> layers = get_via_conn_layers(v);
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
            std::uint32_t flag = 0;
            if (!start)
            {
                flag |= 1;
            }
            if (!end)
            {
                flag |= 2;
            }
            
            no_conn.push_back(std::pair<std::uint32_t, pcb::segment>(flag, s));
        }
    }

}


float pcb::get_segment_len(const pcb::segment& s)
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

void pcb::get_segment_pos(const pcb::segment& s, float offset, float& x, float& y)
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


void pcb::get_segment_perpendicular(const pcb::segment& s, float offset, float w, float& x_left, float& y_left, float& x_right, float& y_right)
{
    if (s.is_arc())
    {
        float x = 0;
        float y = 0;
        double cx;
        double cy;
        double arc_radius;
        calc_arc_center_radius(s.start.x, s.start.y, s.mid.x, s.mid.y, s.end.x, s.end.y, cx, cy, arc_radius);
        
        get_segment_pos(s, offset, x, y);
        
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
        
        get_segment_pos(s, offset, x, y);
        
        x_left = x + w * 0.5 * cos(rad_left);
        y_left = -(-y + w * 0.5 * sin(rad_left));
        x_right = x + w * 0.5 * cos(rad_right);
        y_right = -(-y + w * 0.5 * sin(rad_right));
    }
}


bool pcb::segments_get_next(std::list<pcb::segment>& segments, pcb::segment& s, float x, float y, const std::string& layer_name)
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

std::uint32_t pcb::segment_is_inside_pad(const pcb::segment& s, const pcb::pad& p)
{
    std::vector<std::string> layers = get_pad_layers(p);
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
        return 0;
    }
            
    float x;
    float y;
    get_pad_pos(p, x, y);
    std::uint32_t ret = 0;
    
    if ((s.start.x >= x - p.size_w * 0.5 && s.start.x <= x + p.size_w * 0.5)
        && (s.start.y >= y - p.size_h * 0.5 && s.start.y <= y + p.size_h * 0.5))
    {
        ret |= 1;
    }
    
    if ((s.end.x >= x - p.size_w * 0.5 && s.end.x <= x + p.size_w * 0.5)
        && (s.end.y >= y - p.size_h * 0.5 && s.end.y <= y + p.size_h * 0.5))
    {
        ret |= 2;
    }
    return ret;
}

/****************************************************************/

bool pcb::_float_equal(float a, float b)
{
    return fabs(a - b) < _float_epsilon;
}


bool pcb::_point_equal(float x1, float y1, float x2, float y2)
{
    return _float_equal(x1, x2) && _float_equal(y1, y2);
}


void pcb::_draw_segment(cv::Mat& img, const pcb::segment& s, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    if (s.is_arc())
    {
        float s_len = get_segment_len(s);
        for (float i = 0; i <= s_len - 0.01; i += 0.01)
        {
            float x1 = 0;
            float y1 = 0;
            float x2 = 0;
            float y2 = 0;
            get_segment_pos(s, i, x1, y1);
            get_segment_pos(s, i + 0.01, x2, y2);
            
            cv::line(img,
                cv::Point(_cvt_img_x(x1, pix_unit), _cvt_img_y(y1, pix_unit)),
                cv::Point(_cvt_img_x(x2, pix_unit), _cvt_img_y(y2, pix_unit)),
                cv::Scalar(b, g, r), _cvt_img_len(s.width, pix_unit), cv::LINE_4);
        }
    }
    else
    {
        cv::line(img,
            cv::Point(_cvt_img_x(s.start.x, pix_unit), _cvt_img_y(s.start.y, pix_unit)),
            cv::Point(_cvt_img_x(s.end.x, pix_unit), _cvt_img_y(s.end.y, pix_unit)),
            cv::Scalar(b, g, r), _cvt_img_len(s.width, pix_unit), cv::LINE_4);
    }
}

void pcb::_draw_via(cv::Mat& img, const pcb::via& v, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    std::vector<std::string> layers = get_via_layers(v);
    
    for (const auto& layer: layers)
    {
        if (layer_name == layer)
        {
            pcb::point c(v.at);
            
            cv::Point center(_cvt_img_x(c.x, pix_unit), _cvt_img_y(c.y, pix_unit));
            float radius = v.size / 2;
            radius = _cvt_img_len(radius, pix_unit);
            cv::circle(img, center, radius, cv::Scalar(b, g, r), -1);
        }
    }
    
}

void pcb::_draw_zone(cv::Mat& img, const pcb::zone& z, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    std::vector<cv::Point> pts;
    for (const auto& xy : z.pts)
    {
        cv::Point p(_cvt_img_x(xy.x, pix_unit), _cvt_img_y(xy.y, pix_unit));
        pts.push_back(p);
    }
    cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(b, g, r));
}

void pcb::_draw_gr(cv::Mat& img, const pcb::gr& gr, pcb::point at, float angle, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    if (gr.gr_type == pcb::gr::GR_POLY)
    {
        std::vector<cv::Point> pts;
        for (auto xy : gr.pts)
        {
            get_rotation_pos(at, angle, xy);
            cv::Point p(_cvt_img_x(xy.x, pix_unit), _cvt_img_y(xy.y, pix_unit));
            pts.push_back(p);
        }
        
        cv::polylines(img, std::vector<std::vector<cv::Point>>{pts}, true, cv::Scalar(b, g, r), _cvt_img_len(gr.stroke_width, pix_unit));
        if (gr.fill_type == pcb::gr::FILL_SOLID)
        {
            cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(b, g, r));
        }
    }
    else if (gr.gr_type == pcb::gr::GR_RECT)
    {
        point p1 = gr.start;
        point p2; p2.x = gr.end.x; p2.y = gr.start.y;
        point p3 = gr.end;
        point p4; p4.x = gr.start.x; p4.y = gr.end.y;
        
        get_rotation_pos(at, angle, p1);
        get_rotation_pos(at, angle, p2);
        get_rotation_pos(at, angle, p3);
        get_rotation_pos(at, angle, p4);
        
        std::vector<cv::Point> pts;
        pts.push_back(cv::Point(_cvt_img_x(p1.x, pix_unit), _cvt_img_y(p1.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p2.x, pix_unit), _cvt_img_y(p2.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p3.x, pix_unit), _cvt_img_y(p3.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p4.x, pix_unit), _cvt_img_y(p4.y, pix_unit)));
        
        cv::polylines(img, std::vector<std::vector<cv::Point>>{pts}, true, cv::Scalar(b, g, r), _cvt_img_len(gr.stroke_width, pix_unit));
        if (gr.fill_type == pcb::gr::FILL_SOLID)
        {
            cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(b, g, r));
        }
        
    }
    else if (gr.gr_type == pcb::gr::GR_LINE)
    {
        point start = gr.start;
        point end = gr.end;
        get_rotation_pos(at, angle, start);
        get_rotation_pos(at, angle, end);
        cv::Point p1(_cvt_img_x(start.x, pix_unit), _cvt_img_y(start.y, pix_unit));
        cv::Point p2(_cvt_img_x(end.x, pix_unit), _cvt_img_y(end.y, pix_unit));
        float thickness = _cvt_img_len(gr.stroke_width, pix_unit);
        cv::line(img, p1, p2, cv::Scalar(b, g, r), thickness);
    }
    else if (gr.gr_type == pcb::gr::GR_CIRCLE)
    {
        point start = gr.start;
        point end = gr.end;
        get_rotation_pos(at, angle, start);
        get_rotation_pos(at, angle, end);
        cv::Point center(_cvt_img_x(start.x, pix_unit), _cvt_img_y(start.y, pix_unit));
        float radius = calc_dist(start.x, start.y, end.x, end.y);
        radius = _cvt_img_len(radius, pix_unit);
        float thickness = _cvt_img_len(gr.stroke_width, pix_unit);
        cv::circle(img, center, radius, cv::Scalar(b, g, r), thickness);
        if (gr.fill_type == pcb::gr::FILL_SOLID)
        {
            cv::circle(img, center, radius, cv::Scalar(b, g, r), -1);
        }
    }
}

void pcb::_draw_fp(cv::Mat& img, const pcb::footprint& fp, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    for (const auto& gr: fp.grs)
    {
        if (gr.layer_name == layer_name)
        {
            _draw_gr(img, gr, fp.at, fp.at_angle, b, g, r, pix_unit);
        }
    }
    for (const auto& p: fp.pads)
    {
        _draw_pad(img, fp, p, layer_name, b, g, r, pix_unit);
    }
}

void pcb::_draw_pad(cv::Mat& img, const pcb::footprint& fp, const pcb::pad& p, const std::string& layer_name, std::uint8_t b, std::uint8_t g, std::uint8_t r, float pix_unit)
{
    std::vector<std::string> layers = get_pad_layers(p);
    bool draw = false;
    for (const auto& layer: layers)
    {
        if (layer == layer_name)
        {
            draw = true;
            break;
        }
    }
    
    if (!draw)
    {
        return;
    }
    
    if (p.shape == pcb::pad::SHAPE_RECT || p.shape == pcb::pad::SHAPE_ROUNDRECT)
    {
        point p1(p.at.x - p.size_w / 2, p.at.y + p.size_h / 2);
        point p2(p.at.x + p.size_w / 2, p.at.y + p.size_h / 2);
        point p3(p.at.x + p.size_w / 2, p.at.y - p.size_h / 2);
        point p4(p.at.x - p.size_w / 2, p.at.y - p.size_h / 2);
        
        
        get_rotation_pos(fp.at, fp.at_angle, p1);
        get_rotation_pos(fp.at, fp.at_angle, p2);
        get_rotation_pos(fp.at, fp.at_angle, p3);
        get_rotation_pos(fp.at, fp.at_angle, p4);
        
        std::vector<cv::Point> pts;
        pts.push_back(cv::Point(_cvt_img_x(p1.x, pix_unit), _cvt_img_y(p1.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p2.x, pix_unit), _cvt_img_y(p2.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p3.x, pix_unit), _cvt_img_y(p3.y, pix_unit)));
        pts.push_back(cv::Point(_cvt_img_x(p4.x, pix_unit), _cvt_img_y(p4.y, pix_unit)));
        
        cv::fillPoly(img, std::vector<std::vector<cv::Point>>{pts}, cv::Scalar(b, g, r));
    }
    else if (p.shape == pcb::pad::SHAPE_CIRCLE)
    {
        point c(p.at);
        
        get_rotation_pos(fp.at, fp.at_angle, c);
        cv::Point center(_cvt_img_x(c.x, pix_unit), _cvt_img_y(c.y, pix_unit));
        float radius = p.size_w / 2;
        radius = _cvt_img_len(radius, pix_unit);
        cv::circle(img, center, radius, cv::Scalar(b, g, r), -1);
    }
    else if (p.shape == pcb::pad::SHAPE_OVAL)
    {
        
    }
}