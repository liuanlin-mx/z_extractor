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

#include "calc.h"
#include "kicad_pcb_parser.h"

kicad_pcb_parser::kicad_pcb_parser()
    : _pcb_top(10000.)
    , _pcb_bottom(0)
    , _pcb_left(10000.)
    , _pcb_right(0)
    , _layers(0)
{
}

kicad_pcb_parser::~kicad_pcb_parser()
{
}

bool kicad_pcb_parser::parse(const char * filepath, std::shared_ptr<pcb> z_extr)
{
    _z_extr = z_extr;
    
    
    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL)
    {
        return false;
    }
    
    fseek(fp, 0, SEEK_END);
    std::uint32_t flen = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buf = (char *)malloc(flen + 1);
    if (buf == NULL)
    {
        fclose(fp);
        return false;
    }
    
    std::int32_t rlen = fread(buf, 1, flen, fp);
    fclose(fp);
    
    if (rlen < 0)
    {
        free(buf);
        fclose(fp);
        return false;
    }
    buf[rlen] = 0;
    
    bool ret = _parse(buf);
    free(buf);
    if (ret)
    {
        _z_extr->set_edge(_pcb_top, _pcb_bottom, _pcb_left, _pcb_right);
    }
    return ret;
}


bool kicad_pcb_parser::_parse(const char *str)
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
                _z_extr->add_net(id, name);
                continue;
            }
            else if (label == "segment" || label == "arc")
            {
                pcb::segment s;
                str = _parse_segment(str, s);
                _z_extr->add_segment(s);
                continue;
            }
            else if (label == "gr_line" || label == "gr_circle" || label == "gr_rect" || label == "gr_arc")
            {
                str = _parse_edge(str);
                continue;
            }
            else if (label == "via")
            {
                pcb::via v;
                str = _parse_via(str, v);
                _z_extr->add_via(v);
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
                std::vector<pcb::zone> zones;
                str = _parse_zone(str, zones);
                for (auto& z: zones)
                {
                    _z_extr->add_zone(z);
                }
                continue;
            }
            
            str = _skip(str + 1);
            continue;
        }
        str++;
    }
    
    if (_pcb_top > _pcb_bottom || _pcb_left > _pcb_right)
    {
        printf("err: not found pcb edge.\n");
        return false;
    }
    
    if (_layers == 0)
    {
        printf("err: not found physical stackup.\n");
        return false;
    }
    
    return true;
}

const char *kicad_pcb_parser::_parse_label(const char *str, std::string& label)
{
    while (*str != ' ' && *str != ')' && *str != '\r' && *str != '\n')
    {
        label += *str;
        str++;
    }
    return str;
}
const char *kicad_pcb_parser::_skip(const char *str)
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

const char *kicad_pcb_parser::_parse_zone(const char *str, std::vector<pcb::zone>& zones)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    pcb::zone z;
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
                str = _parse_filled_polygon(str, z);
                zones.push_back(z);
                z.pts.clear();
                z.layer_name.clear();
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


const char *kicad_pcb_parser::_parse_filled_polygon(const char *str, pcb::zone& z)
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
                pcb::point p;
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


const char *kicad_pcb_parser::_parse_net(const char *str, std::uint32_t& id, std::string& name)
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


const char *kicad_pcb_parser::_parse_segment(const char *str, pcb::segment& s)
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
            else if (label == "mid")
            {
                float x;
                float y;
                str = _parse_postion(str, x, y);
                s.mid.x = x;
                s.mid.y = y;
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

const char *kicad_pcb_parser::_parse_via(const char *str, pcb::via& v)
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


const char *kicad_pcb_parser::_parse_number(const char *str, float &num)
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


const char *kicad_pcb_parser::_parse_string(const char *str, std::string& text)
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


const char *kicad_pcb_parser::_parse_postion(const char *str, float &x, float& y)
{
    while (*str == ' ') str++;
    str = _parse_number(str, x);
    while (*str == ' ') str++;
    str = _parse_number(str, y);
    return str;
}



const char *kicad_pcb_parser::_parse_tstamp(const char *str, std::string& tstamp)
{
    while (*str == ' ') str++;
    while (*str != ')')
    {
        tstamp += *str;
        str++;
    }
    return str;
}


const char *kicad_pcb_parser::_parse_layers(const char *str, std::list<std::string>& layers)
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


const char *kicad_pcb_parser::_parse_footprint(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    std::string footprint;
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
                    str = _parse_reference(str, footprint);
                }
                str = _skip(str);
                right++;
            }
            else if (label == "pad")
            {
                pcb::pad p;
                p.ref_at.x = x;
                p.ref_at.y = y;
                p.ref_at_angle = angle;
                p.net = 0xffffffff;
                p.footprint = footprint;
                str = _parse_pad(str, p);
                if (p.net != 0xffffffff)
                {
                    _z_extr->add_pad(p);
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


const char *kicad_pcb_parser::_parse_at(const char *str, float &x, float& y, float& angle)
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

const char *kicad_pcb_parser::_parse_pad_size(const char *str, float& w, float& h)
{
    while (*str == ' ') str++;
    str = _parse_number(str, w);
    while (*str == ' ') str++;
    str = _parse_number(str, h);
    return str;
}


const char *kicad_pcb_parser::_parse_reference(const char *str, std::string& footprint_name)
{
    while (*str == ' ') str++;
    str = _parse_string(str, footprint_name);
    return str;
}


const char *kicad_pcb_parser::_parse_pad(const char *str, pcb::pad& p)
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
            else if (label == "size")
            {
                str = _parse_pad_size(str, p.size_w, p.size_h);
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


const char *kicad_pcb_parser::_parse_setup(const char *str)
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


const char *kicad_pcb_parser::_parse_stackup(const char *str)
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

const char *kicad_pcb_parser::_parse_stackup_layer(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    pcb::layer l;
    
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
    
    if (l.type == "copper"
        || l.type == "core"
        || l.type == "prepreg"
        || l.type == "Top Solder Mask"
        || l.type == "Bottom Solder Mask")
    {
        if ((l.type == "Top Solder Mask" || l.type == "Bottom Solder Mask") && l.epsilon_r == 0)
        {
            printf("warn: not found epsilon r (%s). use default 3.8.\n", l.name.c_str());
            l.epsilon_r  = 3.8;
        }
        
        if (l.thickness == 0)
        {
            printf("warn: not found thickness (%s). use default 0.1.\n", l.name.c_str());
            l.thickness = 0.1;
        }
        
        _z_extr->add_layer(l);
        _layers++;
    }
    return str;
}


const char *kicad_pcb_parser::_parse_edge(const char *str)
{
    std::uint32_t left = 1;
    std::uint32_t right = 0;
    pcb::point start;
    pcb::point mid;
    pcb::point end;
    pcb::point center;
    std::string layer_name;
    
    bool is_arc = false;
    bool is_circle = false;
    
    while (*str)
    {
        if (*str == '(')
        {
            left++;
            std::string label;
            str = _parse_label(str + 1, label);
            if (label == "start")
            {
                str = _parse_postion(str, start.x, start.y);
                
            }
            else if (label == "end")
            {
                str = _parse_postion(str, end.x, end.y);
                
            }
            else if (label == "mid")
            {
                str = _parse_postion(str, mid.x, mid.y);
                is_arc = true;
            }
            else if (label == "center")
            {
                str = _parse_postion(str, center.x, center.y);
                is_circle = true;
            }
            else if (label == "layer")
            {
                while (*str != '"') str++;
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
    
    if (layer_name == "Edge.Cuts")
    {
        float left = _pcb_left;
        float right = _pcb_right;
        float top = _pcb_top;
        float bottom = _pcb_bottom;
            
        if (is_arc)
        {
            double x;
            double y;
            double r;
            calc_arc_center_radius(start.x, start.y, mid.x, mid.y, end.x, end.y, x, y, r);
            center.x = x;
            center.y = y;
            is_circle = true;
            is_arc = false;
        }
        
        if (is_circle)
        {
            float r = calc_dist(center.x, center.y, end.x, end.y);
            left = center.x - r;
            right = center.x + r;
            top = center.y - r;
            bottom = center.y + r;
        }
        else
        {
            
            left = std::min(start.x, end.x);
            right = std::max(start.x, end.x);
            top = std::min(start.y, end.y);
            bottom = std::max(start.y, end.y);
            
        }
        
        if (left < _pcb_left)
        {
            _pcb_left = left;
        }
            
        if (right > _pcb_right)
        {
            _pcb_right = right;
        }
            
        if (top < _pcb_top)
        {
            _pcb_top = top;
        }
            
        if (bottom > _pcb_bottom)
        {
            _pcb_bottom = bottom;
        }
        
    }
                
    return str;
}

