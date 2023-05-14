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

#ifndef __KICAD_PCB_PARSER_H__
#define __KICAD_PCB_PARSER_H__

#include "pcb.h"

class kicad_pcb_parser
{
public:
    kicad_pcb_parser();
    ~kicad_pcb_parser();
    
public:
    bool parse(const char *filepath, std::shared_ptr<pcb> z_extr);
    
private:
    bool _parse(const char *str);
    const char *_parse_label(const char *str, std::string& label);
    const char *_skip(const char *str);
    const char *_parse_zone(const char *str, std::vector<pcb::zone>& zones);
    const char *_parse_filled_polygon(const char *str, pcb::zone& z);
    const char *_parse_net(const char *str, std::uint32_t& id, std::string& name);
    const char *_parse_segment(const char *str, pcb::segment& s);
    const char *_parse_via(const char *str, pcb::via& v);
    const char *_parse_number(const char *str, float &num);
    const char *_parse_string(const char *str, std::string& text);
    const char *_parse_postion(const char *str, float &x, float& y);
    const char *_parse_tstamp(const char *str, std::string& tstamp);
    const char *_parse_layers(const char *str, std::list<std::string>& layers);
    const char *_parse_footprint(const char *str);
    const char *_parse_at(const char *str, float &x, float& y, float& angle);
    const char *_parse_pad_size(const char *str, float& w, float& h);
    const char *_parse_reference(const char *str, std::string& footprint_name);
    const char *_parse_pad(const char *str, pcb::pad& v);
    
    const char *_parse_setup(const char *str);
    const char *_parse_stackup(const char *str);
    const char *_parse_stackup_layer(const char *str);
    const char *_parse_edge(const char *str);
    
private:
    std::shared_ptr<pcb> _z_extr;
    
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
    std::uint32_t _layers;
};

#endif