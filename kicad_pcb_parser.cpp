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
    , _pcb_version(20221018)
    , _uuid_label("tstamp")
{
}

kicad_pcb_parser::~kicad_pcb_parser()
{
}

bool kicad_pcb_parser::parse(const char * filepath, std::shared_ptr<pcb> pcb)
{
    _pcb = pcb;
    
    
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
    
    bool ret = _parse_pcb(buf);
    
    free(buf);
    if (ret)
    {
        _update_pcb_version();
        _update_tstamp_label();
        _add_to_pcb();
        _pcb->set_edge(_pcb_top, _pcb_bottom, _pcb_left, _pcb_right);
    }
    return ret;
}



void kicad_pcb_parser::print_pcb()
{
    _print_object(_root);
}

bool kicad_pcb_parser::_parse_pcb(const char *str)
{
    _root.reset(new pcb_object());
    _parse_object(_root, str);
    return true;
}

const char *kicad_pcb_parser::_parse_object(std::shared_ptr<pcb_object> obj, const char *str)
{
    str = _parse_string2(str + 1, obj->label);
    
    while (str && *str)
    {
        str = _skip_space(str);
        if (!str || *str == 0)
        {
            break;
        }
        
        if (*str == '(')
        {
            std::shared_ptr<pcb_object> child(new pcb_object());
            str = _parse_object(child, str);
            obj->childs.push_back(child);
        }
        else if (*str == ')')
        {
            str++;
            break;
        }
        else
        {
            str = _parse_param(obj, str);
        }
    }
    return str;
}

const char *kicad_pcb_parser::_parse_param(std::shared_ptr<pcb_object> obj, const char *str)
{
    std::string param;
    str = _parse_string2(str, param);
    obj->params.push_back(param);
    return str;
}

const char *kicad_pcb_parser::_parse_string2(const char *str, std::string& text)
{
    str = _skip_space(str);
    if (*str == '\"')
    {
        str++;
        while (*str && *str != '\"')
        {
            text.push_back(*str);
            str++;
        }
        str++;
    }
    else
    {
        while (*str && *str != ' ' && *str != '\r' && *str != '\n' && *str != ')')
        {
            text.push_back(*str);
            str++;
        }
    }
    return str;
}
    
const char *kicad_pcb_parser::_skip_space(const char *str)
{
    while (*str == ' ' || *str == '\r' || *str == '\n' || *str == '\t')
    {
        str++;
    }
    return str;
}



void kicad_pcb_parser::_print_object(std::shared_ptr<pcb_object> obj, std::int32_t tabs)
{
    for (std::int32_t i = 0; i < tabs; i++)
    {
        printf(" ");
    }
    
    printf("(%s ", obj->label.c_str());
    for (const auto& param: obj->params)
    {
        printf(" %s ", param.c_str());
    }
    
    for (const auto& child: obj->childs)
    {
        _print_object(child, tabs + 1);
    }
    
    for (std::int32_t i = 0; i < tabs; i++)
    {
        printf(" ");
    }
    printf(")\n");
}
    

void kicad_pcb_parser::_update_pcb_version()
{
    std::shared_ptr<pcb_object> version = _root->find_child("version");
    if (version && version->params.size())
    {
        _pcb_version = atol(version->params[0].c_str());
    }
}

void kicad_pcb_parser::_update_tstamp_label()
{
    if (_pcb_version >= 20240108)
    {
        _uuid_label = "uuid";
    }
}

    
void kicad_pcb_parser::_add_to_pcb()
{
    _add_layers();
    _add_net_to_pcb();
    _add_segment_to_pcb();
    _add_via_to_pcb();
    _add_zone_to_pcb();
    _add_footprint_to_pcb();
    _add_gr_to_pcb();
}

void kicad_pcb_parser::_add_layers()
{
    std::map<std::string, std::string> aname_map;
    std::shared_ptr<pcb_object> layers = _root->find_child("layers");
    for (auto& child: layers->childs)
    {
        if (child->params.size() >= 3)
        {
            aname_map.emplace(std::pair<std::string, std::string>(child->params[0], child->params[2]));
        }
    }
    
    std::shared_ptr<pcb_object> setup = _root->find_child("setup");
    if (!setup)
    {
        return;
    }
    
    std::shared_ptr<pcb_object> stackup = setup->find_child("stackup");
    if (!stackup)
    {
        return;
    }
    
    for (const auto& layer: stackup->find_childs("layer"))
    {
        std::shared_ptr<pcb_object> type = layer->find_child("type");
        std::shared_ptr<pcb_object> thickness = layer->find_child("thickness");
        std::shared_ptr<pcb_object> epsilon_r = layer->find_child("epsilon_r");
        std::shared_ptr<pcb_object> loss_tangent = layer->find_child("loss_tangent");
        
        if (layer->params.size() > 0 && type && type->params.size() == 1)
        {
            pcb::layer l;
            l.name = layer->params[0];
            if (aname_map.count(l.name))
            {
                l.aname = aname_map[l.name];
            }
            
            std::string layer_type = _strip_string(type->params[0]);
            
            if (thickness && thickness->params.size() > 0)
            {
                l.thickness = atof(thickness->params[0].c_str());
            }
            
            if (epsilon_r && epsilon_r->params.size() > 0)
            {
                l.epsilon_r = atof(epsilon_r->params[0].c_str());
            }
            
            if (loss_tangent && loss_tangent->params.size() > 0)
            {
                l.loss_tangent = atof(loss_tangent->params[0].c_str());
            }
    
            if (layer_type == "copper"
                || layer_type == "core"
                || layer_type == "prepreg"
                || layer_type == "Top Solder Mask"
                || layer_type == "Bottom Solder Mask")
            {
                if ((layer_type == "Top Solder Mask" || layer_type == "Bottom Solder Mask") && l.epsilon_r == 0)
                {
                    printf("warn: not found epsilon r (%s). use default 3.8.\n", l.name.c_str());
                    l.epsilon_r  = 3.8;
                }
                
                if (l.thickness == 0)
                {
                    printf("warn: not found thickness (%s). use default 0.1.\n", l.name.c_str());
                    l.thickness = 0.035;
                }
                
                if (layer_type == "copper")
                {
                    l.type = pcb::layer::COPPER;
                }
                else if (layer_type == "core")
                {
                    l.type = pcb::layer::CORE;
                }
                else if (layer_type == "prepreg")
                {
                    l.type = pcb::layer::PREPREG;
                }
                else if (layer_type == "Top Solder Mask")
                {
                    l.type = pcb::layer::TOP_SOLDER_MASK;
                }
                else if (layer_type == "Bottom Solder Mask")
                {
                    l.type = pcb::layer::BOTTOM_SOLDER_MASK;
                }
                _pcb->add_layer(l);
                _layers++;
            }
        }
    }
}

void kicad_pcb_parser::_add_net_to_pcb()
{
    for (const auto& child: _root->childs)
    {
        if (child->label == "net" && child->params.size() >= 2)
        {
            _pcb->add_net(atoi(child->params[0].c_str()), _strip_string(child->params[1]));
        }
    }
}

void kicad_pcb_parser::_add_segment_to_pcb()
{
    for (const auto& segment: _root->childs)
    {
        if (segment->label == "segment" || segment->label == "arc")
        {
            std::shared_ptr<pcb_object> start = segment->find_child("start");
            std::shared_ptr<pcb_object> mid = segment->find_child("mid");
            std::shared_ptr<pcb_object> end = segment->find_child("end");
            std::shared_ptr<pcb_object> width = segment->find_child("width");
            std::shared_ptr<pcb_object> layer = segment->find_child("layer");
            std::shared_ptr<pcb_object> net = segment->find_child("net");
            std::shared_ptr<pcb_object> tstamp = segment->find_child(_uuid_label);
            if (start && start->params.size() == 2
                && end && end->params.size() == 2
                && width && width->params.size() == 1
                && layer && layer->params.size() == 1
                && net && net->params.size() == 1
                && tstamp && tstamp->params.size())
            {
                pcb::segment s;
                s.start.x = atof(start->params[0].c_str());
                s.start.y = atof(start->params[1].c_str());
                
                s.end.x = atof(end->params[0].c_str());
                s.end.y = atof(end->params[1].c_str());
                
                s.width = atof(width->params[0].c_str());
                
                s.layer_name = _strip_string(layer->params[0]);
                s.net = atoi(net->params[0].c_str());
                s.tstamp = tstamp->params[0];
                
                if (segment->label == "arc" && mid && mid->params.size() == 2)
                {
                    s.mid.x = atof(mid->params[0].c_str());
                    s.mid.y = atof(mid->params[1].c_str());
                }
                _pcb->add_segment(s);
            }
        }
    }
}


void kicad_pcb_parser::_add_via_to_pcb()
{
    for (const auto& child: _root->childs)
    {
        if (child->label == "via")
        {
            std::shared_ptr<pcb_object> at = child->find_child("at");
            std::shared_ptr<pcb_object> size = child->find_child("size");
            std::shared_ptr<pcb_object> drill = child->find_child("drill");
            std::shared_ptr<pcb_object> layers = child->find_child("layers");
            std::shared_ptr<pcb_object> net = child->find_child("net");
            std::shared_ptr<pcb_object> tstamp = child->find_child(_uuid_label);
            if (at && at->params.size() == 2
                && size && size->params.size() == 1
                && drill && drill->params.size() == 1
                && layers && layers->params.size() >= 1
                && net && net->params.size() == 1
                && tstamp && tstamp->params.size())
            {
                pcb::via v;
                v.at.x = atof(at->params[0].c_str());
                v.at.y = atof(at->params[1].c_str());
                
                v.size = atof(size->params[0].c_str());
                
                v.drill = atof(drill->params[0].c_str());
                
                for (const auto& layer_name: layers->params)
                {
                    v.layers.push_back(_strip_string(layer_name));
                }
                v.net = atoi(net->params[0].c_str());
                v.tstamp = tstamp->params[0];
                
                _pcb->add_via(v);
            }
        }
    }
}


void kicad_pcb_parser::_add_zone_to_pcb()
{
    for (const auto& child: _root->find_childs("zone"))
    {
        std::shared_ptr<pcb_object> net = child->find_child("net");
        std::shared_ptr<pcb_object> tstamp = child->find_child(_uuid_label);
        
        if (net && net->params.size() == 1
                && tstamp && tstamp->params.size())
        {
            for (const auto& filled_polygon: child->find_childs("filled_polygon"))
            {
                pcb::zone z;
                std::shared_ptr<pcb_object> layer = filled_polygon->find_child("layer");
                std::shared_ptr<pcb_object> pts = filled_polygon->find_child("pts");
                
                if (layer && layer->params.size() >= 1 && pts)
                {
                    z.layer_name = _strip_string(layer->params[0]);
                    z.net = atoi(net->params[0].c_str());
                    z.tstamp = tstamp->params[0];
                    for (const auto& xy: pts->find_childs("xy"))
                    {
                        if (xy->params.size() == 2)
                        {
                            pcb::point p;
                            p.x = atof(xy->params[0].c_str());
                            p.y = atof(xy->params[1].c_str());
                            z.pts.push_back(p);
                        }
                    }
                    _pcb->add_zone(z);
                }
            }
        }
    }
}


void kicad_pcb_parser::_add_footprint_to_pcb()
{
    for (const auto& child: _root->find_childs("footprint"))
    {
        std::shared_ptr<pcb_object> layer = child->find_child("layer");
        std::shared_ptr<pcb_object> tstamp = child->find_child(_uuid_label);
        std::shared_ptr<pcb_object> at = child->find_child("at");
        
        pcb::footprint footprint;
        if (layer && layer->params.size() == 1
                && tstamp && tstamp->params.size() == 1
                && at && at->params.size() >= 2)
        {
            footprint.tstamp = tstamp->params[0];
            footprint.at.x = atof(at->params[0].c_str());
            footprint.at.y = atof(at->params[1].c_str());
            footprint.layer = _strip_string(layer->params[0]);
            if (at->params.size() >= 3)
            {
                footprint.at_angle = atof(at->params[2].c_str());
            }
            
            if (_pcb_version >= 20240108)
            {
                for (const auto& property: child->find_childs("property"))
                {
                    if (property->params.size() < 2)
                    {
                        continue;
                    }
                    if (property->params[0] == "Reference")
                    {
                        footprint.reference = _strip_string(property->params[1]);
                    }
                    if (property->params[0] == "Value")
                    {
                        footprint.value = _strip_string(property->params[1]);
                    }
                }
            }
            else
            {
                for (const auto& fp_text: child->find_childs("fp_text"))
                {
                    if (fp_text->params.size() < 2)
                    {
                        continue;
                    }
                    if (fp_text->params[0] == "reference")
                    {
                        footprint.reference = _strip_string(fp_text->params[1]);
                    }
                    if (fp_text->params[0] == "value")
                    {
                        footprint.value = _strip_string(fp_text->params[1]);
                    }
                }
            }
            

            for (const auto& pad: child->find_childs("pad"))
            {
                if (pad->params.size() < 3)
                {
                    continue;
                }
                std::shared_ptr<pcb_object> layers = pad->find_child("layers");
                std::shared_ptr<pcb_object> pad_at = pad->find_child("at");
                std::shared_ptr<pcb_object> size = pad->find_child("size");
                std::shared_ptr<pcb_object> drill = pad->find_child("drill");
                std::shared_ptr<pcb_object> pad_uuid = pad->find_child(_uuid_label);
                std::shared_ptr<pcb_object> net = pad->find_child("net");
                
                pcb::pad p;
                p.footprint = footprint.reference;
                p.pad_number = pad->params[0];
                p.ref_at = footprint.at;
                p.ref_at_angle = footprint.at_angle;
                
                if (pad->params[2] == "rect")
                {
                    p.shape = pcb::pad::SHAPE_RECT;
                }
                else if (pad->params[2] == "circle")
                {
                    p.shape = pcb::pad::SHAPE_CIRCLE;
                }
                else if (pad->params[2] == "roundrect")
                {
                    p.shape = pcb::pad::SHAPE_ROUNDRECT;
                }
                else if (pad->params[2] == "trapezoid")
                {
                    p.shape = pcb::pad::SHAPE_TRAPEZOID;
                }
                
                if (pad->params[1] == "thru_hole")
                {
                    p.type = pcb::pad::TYPE_THRU_HOLE;
                }
                else if (pad->params[1] == "connect")
                {
                    p.type = pcb::pad::TYPE_CONNECT;
                }
                else if (pad->params[1] == "smd")
                {
                    p.type = pcb::pad::TYPE_SMD;
                }
                
                if (pad_uuid && !pad_uuid->params.empty())
                {
                    p.tstamp = pad_uuid->params[0];
                }
                
                if (layers && !layers->params.empty())
                {
                    std::string str = layers->params[0];
                    p.layers.push_back(_strip_string(str));
                }
                
                if (pad_at && pad_at->params.size() >= 2)
                {
                    p.at.x = atof(pad_at->params[0].c_str());
                    p.at.y = atof(pad_at->params[1].c_str());
                }
                
                if (pad_at && pad_at->params.size() >= 3)
                {
                    p.at_angle = atof(pad_at->params[2].c_str());
                }
                
                if (size && size->params.size() == 2)
                {
                    p.size_w = atof(size->params[0].c_str());
                    p.size_h = atof(size->params[1].c_str());
                }
                
                if (drill && drill->params.size() == 1)
                {
                    p.drill = atof(drill->params[0].c_str());
                }
                
                if (net && net->params.size() > 0)
                {
                    p.net = atoi(net->params[0].c_str());
                }
                
                if (net && net->params.size() > 1)
                {
                    p.net_name = _strip_string(net->params[1]);
                }
                
                footprint.pads.push_back(p);
            }
            
            
            _add_gr_to_footprint(child, footprint);
            _pcb->add_footprint(footprint);
        }
    }
}

void kicad_pcb_parser::_add_gr_to_footprint(std::shared_ptr<pcb_object> fp_obj, pcb::footprint& fp)
{
    
    for (const auto& child: fp_obj->childs)
    {
        if (child->label == "fp_arc"
            || child->label == "fp_rect"
            || child->label == "fp_line"
            || child->label == "fp_poly"
            || child->label == "fp_circle")
        {
            std::shared_ptr<pcb_object> start = child->find_child("start");
            std::shared_ptr<pcb_object> center = child->find_child("center");
            std::shared_ptr<pcb_object> mid = child->find_child("mid");
            std::shared_ptr<pcb_object> end = child->find_child("end");
            std::shared_ptr<pcb_object> layer = child->find_child("layer");
            std::shared_ptr<pcb_object> fill = child->find_child("fill");
            std::shared_ptr<pcb_object> stroke = child->find_child("stroke");
            std::shared_ptr<pcb_object> tstamp = child->find_child(_uuid_label);
            std::shared_ptr<pcb_object> pts = child->find_child("pts");
            
            if (layer && layer->params.size() == 1
                && tstamp && tstamp->params.size())
            {
                std::shared_ptr<pcb_object> stroke_width;
                std::shared_ptr<pcb_object> stroke_type;
                if (stroke)
                {
                    stroke_width = stroke->find_child("width");
                    stroke_type = stroke->find_child("type");
                }
                
                pcb::gr gr;
                if (start && start->params.size() == 2)
                {
                    gr.start.x = atof(start->params[0].c_str());
                    gr.start.y = atof(start->params[1].c_str());
                }
                else if (center && center->params.size() == 2)
                {
                    gr.start.x = atof(center->params[0].c_str());
                    gr.start.y = atof(center->params[1].c_str());
                }
                
                if (mid && mid->params.size() == 2)
                {
                    gr.mid.x = atof(mid->params[0].c_str());
                    gr.mid.y = atof(mid->params[1].c_str());
                }
                if (end && end->params.size() == 2)
                {
                    gr.end.x = atof(end->params[0].c_str());
                    gr.end.y = atof(end->params[1].c_str());
                }
                
                gr.layer_name = _strip_string(layer->params[0]);
                gr.tstamp = tstamp->params[0];
                
                if (child->label == "fp_arc")
                {
                    gr.gr_type = pcb::gr::GR_ARC;
                }
                else if (child->label == "fp_rect")
                {
                    gr.gr_type = pcb::gr::GR_RECT;
                }
                else if (child->label == "fp_line")
                {
                    gr.gr_type = pcb::gr::GR_LINE;
                }
                else if (child->label == "fp_poly")
                {
                    gr.gr_type = pcb::gr::GR_POLY;
                }
                else if (child->label == "fp_circle")
                {
                    gr.gr_type = pcb::gr::GR_CIRCLE;
                }
                
                if (fill && fill->params.size() == 1)
                {
                    if (fill->params[0] == "solid")
                    {
                        gr.fill_type = pcb::gr::FILL_SOLID;
                    }
                    else
                    {
                        gr.fill_type = pcb::gr::FILL_NONE;
                    }
                }
                
                
                if (stroke_width && stroke_width->params.size() == 1)
                {
                    gr.stroke_width = atof(stroke_width->params[0].c_str());
                }
                
                if (stroke_type && stroke_type->params.size() == 1)
                {
                    if (stroke_type->params[0] == "solid")
                    {
                        gr.stroke_type = pcb::gr::STROKE_SOLID;
                    }
                    else
                    {
                        gr.stroke_type = pcb::gr::STROKE_NONE;
                    }
                }
                
                if (pts)
                {
                    for (const auto& xy: pts->find_childs("xy"))
                    {
                        if (xy->params.size() == 2)
                        {
                            pcb::point p;
                            p.x = atof(xy->params[0].c_str());
                            p.y = atof(xy->params[1].c_str());
                            gr.pts.push_back(p);
                        }
                    }
                }
                fp.grs.push_back(gr);
            }
        }
    }
}

void kicad_pcb_parser::_add_gr_to_pcb()
{
    for (const auto& child: _root->childs)
    {
        if (child->label == "gr_arc"
            || child->label == "gr_rect"
            || child->label == "gr_line"
            || child->label == "gr_poly"
            || child->label == "gr_circle")
        {
            std::shared_ptr<pcb_object> start = child->find_child("start");
            std::shared_ptr<pcb_object> center = child->find_child("center");
            std::shared_ptr<pcb_object> mid = child->find_child("mid");
            std::shared_ptr<pcb_object> end = child->find_child("end");
            std::shared_ptr<pcb_object> layer = child->find_child("layer");
            std::shared_ptr<pcb_object> fill = child->find_child("fill");
            std::shared_ptr<pcb_object> stroke = child->find_child("stroke");
            std::shared_ptr<pcb_object> tstamp = child->find_child(_uuid_label);
            std::shared_ptr<pcb_object> pts = child->find_child("pts");
            
            if (layer && layer->params.size() == 1
                && tstamp && tstamp->params.size())
            {
                std::shared_ptr<pcb_object> stroke_width;
                std::shared_ptr<pcb_object> stroke_type;
                if (stroke)
                {
                    stroke_width = stroke->find_child("width");
                    stroke_type = stroke->find_child("type");
                }
                
                pcb::gr gr;
                if (start && start->params.size() == 2)
                {
                    gr.start.x = atof(start->params[0].c_str());
                    gr.start.y = atof(start->params[1].c_str());
                }
                else if (center && center->params.size() == 2)
                {
                    gr.start.x = atof(center->params[0].c_str());
                    gr.start.y = atof(center->params[1].c_str());
                }
                
                if (mid && mid->params.size() == 2)
                {
                    gr.mid.x = atof(mid->params[0].c_str());
                    gr.mid.y = atof(mid->params[1].c_str());
                }
                if (end && end->params.size() == 2)
                {
                    gr.end.x = atof(end->params[0].c_str());
                    gr.end.y = atof(end->params[1].c_str());
                }
                
                gr.layer_name = _strip_string(layer->params[0]);
                gr.tstamp = tstamp->params[0];
                
                if (child->label == "gr_arc")
                {
                    gr.gr_type = pcb::gr::GR_ARC;
                }
                else if (child->label == "gr_rect")
                {
                    gr.gr_type = pcb::gr::GR_RECT;
                }
                else if (child->label == "gr_line")
                {
                    gr.gr_type = pcb::gr::GR_LINE;
                }
                else if (child->label == "gr_poly")
                {
                    gr.gr_type = pcb::gr::GR_POLY;
                }
                else if (child->label == "gr_circle")
                {
                    gr.gr_type = pcb::gr::GR_CIRCLE;
                }
                
                if (fill && fill->params.size() == 1)
                {
                    if (fill->params[0] == "solid")
                    {
                        gr.fill_type = pcb::gr::FILL_SOLID;
                    }
                    else
                    {
                        gr.fill_type = pcb::gr::FILL_NONE;
                    }
                }
                
                
                if (stroke_width && stroke_width->params.size() == 1)
                {
                    gr.stroke_width = atof(stroke_width->params[0].c_str());
                }
                
                if (stroke_type && stroke_type->params.size() == 1)
                {
                    if (stroke_type->params[0] == "solid")
                    {
                        gr.stroke_type = pcb::gr::STROKE_SOLID;
                    }
                    else
                    {
                        gr.stroke_type = pcb::gr::STROKE_NONE;
                    }
                }
                
                if (pts)
                {
                    for (const auto& xy: pts->find_childs("xy"))
                    {
                        if (xy->params.size() == 2)
                        {
                            pcb::point p;
                            p.x = atof(xy->params[0].c_str());
                            p.y = atof(xy->params[1].c_str());
                            gr.pts.push_back(p);
                        }
                    }
                }
                _pcb->add_gr(gr);
                _update_edge(gr);
            }
        }
    }
}


void kicad_pcb_parser::_update_edge(const pcb::gr& g)
{
    pcb::point center;
    
    if (g.layer_name == "Edge.Cuts")
    {
        float left = _pcb_left;
        float right = _pcb_right;
        float top = _pcb_top;
        float bottom = _pcb_bottom;
        
        _pcb->add_edge_gr(g);
        if (g.gr_type == pcb::gr::GR_ARC || g.gr_type == pcb::gr::GR_CIRCLE || g.gr_type == pcb::gr::GR_LINE)
        {
            bool is_arc = g.gr_type == pcb::gr::GR_ARC;
            bool is_circle = g.gr_type == pcb::gr::GR_CIRCLE;
            
            if (is_arc)
            {
                double x;
                double y;
                double r;
                calc_arc_center_radius(g.start.x, g.start.y, g.mid.x, g.mid.y, g.end.x, g.end.y, x, y, r);
                center.x = x;
                center.y = y;
                is_circle = true;
                is_arc = false;
            }
            
            if (is_circle)
            {
                float r = calc_dist(center.x, center.y, g.end.x, g.end.y);
                left = center.x - r;
                right = center.x + r;
                top = center.y - r;
                bottom = center.y + r;
            }
            else
            {
                left = std::min(g.start.x, g.end.x);
                right = std::max(g.start.x, g.end.x);
                top = std::min(g.start.y, g.end.y);
                bottom = std::max(g.start.y, g.end.y);
            }
        }
        else if (g.gr_type == pcb::gr::GR_RECT)
        {
            left = std::min(g.start.x, g.end.x);
            right = std::max(g.start.x, g.end.x);
            top = std::min(g.start.y, g.end.y);
            bottom = std::max(g.start.y, g.end.y);
        }
        else if (g.gr_type == pcb::gr::GR_POLY)
        {
            for (const auto& p: g.pts)
            {
                if (left > p.x)
                {
                    left = p.x;
                }
                if (right < p.x)
                {
                    right = p.x;
                }
                if (top > p.y)
                {
                    top = p.y;
                }
                if (bottom < p.y)
                {
                    bottom = p.y;
                }
            }
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
}

std::string kicad_pcb_parser::_strip_string(const std::string& str)
{
    std::string tmp = str;
    if (tmp.front() == '\"')
    {
        tmp = tmp.substr(1);
    }
    if (tmp.back() == '\"')
    {
        tmp.pop_back();
    }
    return tmp;
}
