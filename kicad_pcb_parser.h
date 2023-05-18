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
    struct pcb_object
    {
        std::string label;
        std::vector<std::string> params;
        std::list<std::shared_ptr<pcb_object> > childs;
        
        std::shared_ptr<pcb_object> find_child(const std::string& label)
        {
            for (auto& child: childs)
            {
                if (child->label == label)
                {
                    return child;
                }
            }
            return NULL;
        }
        
        std::list<std::shared_ptr<pcb_object> > find_childs(const std::string& label)
        {
            std::list<std::shared_ptr<pcb_object> > tmp;
            for (auto& child: childs)
            {
                if (child->label == label)
                {
                    tmp.push_back(child);
                }
            }
            return tmp;
        }
    };
    
public:
    kicad_pcb_parser();
    ~kicad_pcb_parser();
    
public:
    bool parse(const char *filepath, std::shared_ptr<pcb> z_extr);
    void print_pcb();
    
private:
    bool _parse_pcb(const char *str);
    const char *_parse_object(std::shared_ptr<pcb_object> obj, const char *str);
    const char *_parse_param(std::shared_ptr<pcb_object> obj, const char *str);
    const char *_parse_string2(const char *str, std::string& text);
    const char *_skip_space(const char *str);
    void _print_object(std::shared_ptr<pcb_object> obj, std::int32_t tabs = 0);
    
    
    void _add_to_pcb();
    void _add_layers();
    void _add_net_to_pcb();
    void _add_segment_to_pcb();
    void _add_via_to_pcb();
    void _add_zone_to_pcb();
    void _add_footprint_to_pcb();
    void _add_gr_to_footprint(std::shared_ptr<pcb_object> fp_obj, pcb::footprint& fp);
    void _add_gr_to_pcb();
    void _update_edge(const pcb::gr& g);
    
    std::string _strip_string(const std::string& str);
private:
    std::shared_ptr<pcb> _pcb;
    std::shared_ptr<pcb_object> _root;
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
    std::uint32_t _layers;
};

#endif