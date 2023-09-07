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

#include <stdio.h>
#include <math.h>
#include <iostream>
#include "z_extractor.h"
#include "kicad_pcb_parser.h"
#include <opencv2/opencv.hpp>
#include "make_cir.h"

#include "openems_model_gen.h"



static void _parse_net(const char *str, std::list<std::string>& nets)
{
    char *save_ptr = NULL;
    static char str_[4096];
    strncpy(str_, str, sizeof(str_) - 1);
    const char *p = strtok_r(str_, ",", &save_ptr);
    while (p)
    {
        nets.push_back(p);
        p = strtok_r(NULL, ",", &save_ptr);
    }
}

static void _parse_coupled_net(const char *str, std::list<std::pair<std::string, std::string> >& coupled_nets)
{
    std::list<std::string> nets;
    _parse_net(str, nets);
    std::string tmp;
    for (auto& net: nets)
    {
        size_t pos = net.find(":");
        if (pos != net.npos)
        {
            std::string s1 = net.substr(0, pos);
            std::string s2 = net.substr(pos + 1);
            coupled_nets.push_back(std::pair<std::string, std::string>(s1, s2));
        }
    }
}


static std::vector<std::string> _string_split(std::string str, const std::string& key)
{
    std::vector<std::string> out;
    std::string::size_type begin = 0;
    std::string::size_type end = 0;
    while ((end = str.find(key, begin)) != str.npos)
    {
        out.push_back(str.substr(begin, end - begin));
        begin = end + key.size();
    }
    if (begin < str.size())
    {
        out.push_back(str.substr(begin, end - begin));
    }
    
    return out;
}


enum
{
    MODE_NONE,
    MODE_TL,
    MODE_RL,
    MODE_ANT,
    MODE_SP
};

static std::shared_ptr<pcb> pcb_(new pcb());
static std::shared_ptr<z_extractor> z_extr(new z_extractor(pcb_));
    
static std::uint32_t mode = MODE_NONE;

static const char *pcb_file = NULL;


static int main_tl_rl(int argc, char **argv)
{
    const char *oname = NULL;
    float coupled_max_gap = 2;
    float coupled_min_len = 0.5;
    bool lossless_tl = true;
    bool ltra = false;
    float freq = 1e0;
    float conductivity = 5.8e7;
    float step = 0.5;
    bool via_tl_mode = false;
    bool use_mmtl = true;
    bool enable_openmp = false;
    
    std::list<std::string> nets;
    std::list<std::pair<std::string, std::string> > coupled_nets;
    std::list<std::pair<std::string, std::string> > pads;
    std::list<std::string> refs;
    std::vector<std::string> current;
    
    static char buf[4 * 1024 * 1024];
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        
        if (std::string(arg) == "-net" && i < argc)
        {
            _parse_net(arg_next, nets);
        }
        else if (std::string(arg) == "-ref" && i < argc)
        {
            _parse_net(arg_next, refs);
        }
        else if (std::string(arg) == "-coupled" && i < argc)
        {
            _parse_coupled_net(arg_next, coupled_nets);
        }
        else if (std::string(arg) == "-pad" && i < argc)
        {
            _parse_coupled_net(arg_next, pads);
        }
        else if (std::string(arg) == "-o" && i < argc)
        {
            oname = arg_next;
        }
        else if (std::string(arg) == "-coupled_max_gap" && i < argc)
        {
            coupled_max_gap = atof(arg_next);
        }
        else if (std::string(arg) == "-coupled_min_len" && i < argc)
        {
            coupled_min_len = atof(arg_next);
        }
        else if (std::string(arg) == "-lossless_tl" && i < argc)
        {
            lossless_tl = (atoi(arg_next) == 0)? false: true;
        }
        else if (std::string(arg) == "-ltra" && i < argc)
        {
            ltra = (atoi(arg_next) == 0)? false: true;
        }
        else if (std::string(arg) == "-conductivity" && i < argc)
        {
            conductivity = atof(arg_next);
        }
        else if (std::string(arg) == "-I" && i < argc)
        {
            current = _string_split(arg_next, ",");
        }
        else if (std::string(arg) == "-freq" && i < argc)
        {
            freq = atof(arg_next);
        }
        else if (std::string(arg) == "-step" && i < argc)
        {
            step = atof(arg_next);
        }
        else if (std::string(arg) == "-via_tl_mode" && i < argc)
        {
            via_tl_mode = (atoi(arg_next) == 0)? false: true;
        }
        else if (std::string(arg) == "-mmtl" && i < argc)
        {
            use_mmtl = (atoi(arg_next) == 0)? false: true;
        }
        else if (std::string(arg) == "-openmp" && i < argc)
        {
            enable_openmp = (atoi(arg_next) == 0)? false: true;
        }
        
    }
    if (mode == MODE_TL)
    {
        if (refs.empty() || (nets.empty() && coupled_nets.empty()))
        {
            return 0;
        }
    }
    else if (mode == MODE_RL)
    {
        if (pads.empty() && nets.empty())
        {
            return 0;
        }
    }
    
    pcb_->clean_segment(nets);
    for (const auto& net: coupled_nets)
    {
        pcb_->clean_segment(pcb_->get_net_id(net.first));
        pcb_->clean_segment(pcb_->get_net_id(net.second));
    }
    
    z_extr->set_coupled_max_gap(coupled_max_gap);
    z_extr->set_coupled_min_len(coupled_min_len);
    z_extr->enable_lossless_tl(lossless_tl);
    z_extr->enable_ltra_model(ltra);
    z_extr->enable_via_tl_mode(via_tl_mode);
    z_extr->enable_openmp(enable_openmp);
    z_extr->set_freq(freq);
    z_extr->set_conductivity(conductivity);
    z_extr->set_step(step);
    z_extr->set_calc((use_mmtl)? Z0_calc::Z0_CALC_MMTL: Z0_calc::Z0_CALC_FDM);
    
    std::string spice;
    std::string info;
    if (mode == MODE_TL)
    {
        std::vector<std::uint32_t> v_refs;
        
        for (const auto& net: refs)
        {
            v_refs.push_back(pcb_->get_net_id(net.c_str()));
        }
        
        bool first = true;
        float velocity = 0;
        char str[4096] = {0};
        for (const auto& net: nets)
        {
            std::string ckt;
            std::string call;
            std::set<std::string> footprint;
            float Z0_avg = 0;
            float td = 0;
            float velocity_avg = 0;
            if (!z_extr->gen_subckt_zo(pcb_->get_net_id(net.c_str()), v_refs, ckt, footprint, call, Z0_avg, td, velocity_avg))
            {
                continue;
            }
            //printf("ckt:%s\n", ckt.c_str());
            spice += "*" + call + ckt + "\n\n\n";
            
            if (first)
            {
                first = false;
                velocity = velocity_avg;
            }
            float len = velocity * td;
            sprintf(str, "net: \"%s\"  Z0:%.1f  td:%.4fNS  len:(%.1fmil)\n", net.c_str(), Z0_avg, td, len / 0.0254);
            info += str;
        }
        
        for (const auto& coupled: coupled_nets)
        {
            //printf("%s %s\n", coupled.first.c_str(), coupled.second.c_str());
            std::string ckt;
            std::string call;
            std::set<std::string> footprint;
            //float td = 0;
            float Z0_avg[2] = {0., 0.};
            float td_sum[2] = {0., 0.};
            float velocity_avg[2] = {0., 0.};
            float Zodd_avg = 0.;
            float Zeven_avg = 0.;
            if (!z_extr->gen_subckt_coupled_tl(pcb_->get_net_id(coupled.first.c_str()), pcb_->get_net_id(coupled.second.c_str()),
                                        v_refs, ckt, footprint, call,
                                        Z0_avg, td_sum, velocity_avg, Zodd_avg, Zeven_avg))
            {
                continue;
            }
    
            //printf("ckt:%s\n", ckt.c_str());
            spice += "*" + call + ckt + "\n\n\n";
            
            if (first)
            {
                first = false;
                velocity = velocity_avg[0];
            }
            
            sprintf(str, "net: \"%s:%s\"  Zodd:%.1f  Zeven:%.f  Zdiff:%.1f  Zcomm:%.1f\n",
                coupled.first.c_str(), coupled.second.c_str(),
                Zodd_avg, Zeven_avg, Zodd_avg * 2., Zeven_avg * 0.5);
            info += str;
            
            sprintf(str, "net: \"%s\"  Z0:%.1f  td:%.4fNS  len:(%.1fmil)\n", coupled.first.c_str(), Z0_avg[0], td_sum[0], velocity * td_sum[0] / 0.0254);
            info += str;
            sprintf(str, "net: \"%s\"  Z0:%.1f  td:%.4fNS  len:(%.1fmil)\n", coupled.second.c_str(), Z0_avg[1], td_sum[1], velocity * td_sum[1] / 0.0254);
            info += str;
            
            //char str[4096] = {0};
            //float c = 299792458000 * v_ratio;
            //float len = c * (td / 1000000000.);
            //sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", coupled.first.c_str(), td, len, len / 0.0254);
            //sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", coupled.second.c_str(), td, len, len / 0.0254);
            //fwrite(str, 1, strlen(str), info_fp);
        }
    }
    else if (mode == MODE_RL)
    {
        char str[4096] = {0};
        for (const auto& pad: pads)
        {
            std::string ckt;
            std::string call;
            std::vector<std::string> footprint;
            float r = 0;
            float l = 0;
            
            std::vector<std::string> pad1 = _string_split(pad.first, ".");
            std::vector<std::string> pad2 = _string_split(pad.second, ".");
            if (z_extr->gen_subckt_rl(pad1.front(), pad1.back(), pad2.front(), pad2.back(), ckt, call, r, l))
            {
                spice += ckt;
                sprintf(str, "pad-pad: %s.%s:%s.%s R=%.4e L=%.4gnH",
                            pad1.front().c_str(), pad1.back().c_str(), pad2.front().c_str(), pad2.back().c_str(), r, l * 1e9);
                info += str;
                
                if (!current.empty())
                {
                    sprintf(str, " voltage drop: ");
                    info += str;
                    for (auto I: current)
                    {
                        sprintf(str, "(%.3eV@%gA) ", r * atof(I.c_str()), atof(I.c_str()));
                        info += str;
                    }
                }
                info += "\n";
            }
        }
        
        
        for (const auto& net: nets)
        {
            std::string ckt;
            std::string call;
            std::set<std::string> footprint;
            
            if (z_extr->gen_subckt(pcb_->get_net_id(net.c_str()), ckt, footprint, call))
            {
                spice += ckt;
            }
        }
    }
    
    printf("%s\n", info.c_str());
    if (oname == NULL)
    {
        oname = "out";
    }
    sprintf(buf, "%s.lib", oname);
    FILE *spice_lib_fp = fopen(buf, "wb");
    if (spice_lib_fp)
    {
        fwrite(spice.c_str(), 1, spice.length(), spice_lib_fp);
        fclose(spice_lib_fp);
    }
    
    sprintf(buf, "%s.info", oname);
    FILE *info_fp = fopen(buf, "wb");
    if (info_fp)
    {
        fwrite(info.c_str(), 1, info.length(), info_fp);
        fclose(info_fp);
    }
    return 0;
}




static int main_sparameter(int argc, char **argv)
{
    float max_freq = 3e9;
    float criteria = -50;
    const char *prefix = "";
    std::list<std::string> nets;
    std::vector<std::string> footprints;
    std::vector<std::string> ports;
    std::vector<std::string> lumped_elements;
    std::vector<std::string> mesh_range;
    std::map<std::string, float> mesh_net;
    std::vector<std::string> freq;
    std::string bc = "MUR";
    
    openems_model_gen ems(pcb_);
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        
        if (std::string(arg) == "-net" && i < argc)
        {
            _parse_net(arg_next, nets);
        }
        else if (std::string(arg) == "-max_freq" && i < argc)
        {
            max_freq = atof(arg_next);
        }
        else if (std::string(arg) == "-freq" && i < argc)
        {
            freq = _string_split(arg_next, ",");
        }
        else if (std::string(arg) == "-fp" && i < argc)
        {
            footprints.push_back(arg_next);
        }
        else if (std::string(arg) == "-port" && i < argc)
        {
            ports.push_back(arg_next);
        }
        else if (std::string(arg) == "-le" && i < argc)
        {
            lumped_elements.push_back(arg_next);
        }
        else if (std::string(arg) == "-mesh_range" && i < argc)
        {
            mesh_range.push_back(arg_next);
        }
        else if (std::string(arg) == "-mesh_net" && i < argc)
        {
            std::vector<std::string> arg_v = _string_split(arg_next, ":");
            if (arg_v.size() == 2)
            {
                mesh_net.emplace(arg_v[0], atof(arg_v[1].c_str()));
            }
        }
        else if (std::string(arg) == "-bc" && i < argc)
        {
            bc = arg_next;
        }
        else if (std::string(arg) == "-criteria" && i < argc)
        {
            criteria = atof(arg_next);
        }
        else if (std::string(arg) == "-o" && i < argc)
        {
            prefix = arg_next;
        }
    }
    
    
    pcb_->clean_segment(nets);
    
    ems.set_boundary_cond((bc == "PML")? openems_model_gen::BC_PML: openems_model_gen::BC_MUR);
    for (const auto& f1: freq)
    {
        float f2 = atof(f1.c_str());
        if (f2 > 1)
        {
            ems.add_freq(f2);
        }
    }
    
    if (max_freq > 1)
    {
        ems.set_excitation_freq(0, max_freq);
    }
    if (criteria < 0)
    {
        ems.set_end_criteria(criteria);
    }
    
    for (const auto& net: nets)
    {
        if (mesh_net.count(net))
        {
            ems.add_net(pcb_->get_net_id(net), mesh_net[net], mesh_net[net], false, 1);
        }
        else
        {
            ems.add_net(pcb_->get_net_id(net), false);
        }
    }
    
    for (const auto& fp: footprints)
    {
        ems.add_footprint(fp, false);
    }
    
    if (nets.empty() && footprints.empty())
    {
        printf("err: net class empty, footprint empty\n");
        return 0;
    }
    
    ems.set_mesh_min_gap(0.01, 0.01, 0.01);
    if (mesh_range.empty() && mesh_net.empty())
    {
        ems.add_mesh_range(pcb_->get_edge_left(), pcb_->get_edge_right(), 0.1, openems_model_gen::mesh::DIR_X);
        ems.add_mesh_range(pcb_->get_edge_top(), pcb_->get_edge_bottom(), 0.1, openems_model_gen::mesh::DIR_Y);
    }
    
    for (const auto& range: mesh_range)
    {
        std::vector<std::string> arg_v = _string_split(range, ":");
        if (4 == arg_v.size())
        {
            ems.add_mesh_range(atof(arg_v[0].c_str()), atof(arg_v[1].c_str()), atof(arg_v[2].c_str()), arg_v[3] == "x"? openems_model_gen::mesh::DIR_X: openems_model_gen::mesh::DIR_Y);
        }
    }
    
    
    for (const auto& le: lumped_elements)
    {
        std::vector<std::string> arg_v = _string_split(le, ":");
        if (1 == arg_v.size())
        {
            ems.add_lumped_element(arg_v[0], false);
        }
    }
    
    
    bool no_port = true;
    for (const auto& port: ports)
    {
        std::vector<std::string> arg_v = _string_split(port, ":");
        //eg R1:1:F.Cu:R1:2:F.Cu:y:50:1
        if (9 == arg_v.size())
        {
            std::uint32_t ex_dir = openems_model_gen::excitation::DIR_X;
            if (arg_v[6] == "y")
            {
                ex_dir = openems_model_gen::excitation::DIR_Y;
            }
            else if (arg_v[6] == "z")
            {
                ex_dir = openems_model_gen::excitation::DIR_Z;
            }
            
            ems.add_lumped_port(arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[2]),
                                    arg_v[3], arg_v[4], pcb_->get_layer_name(arg_v[5]),
                                    ex_dir, atof(arg_v[7].c_str()), arg_v[8] == "1", false);
            no_port = false;
        }
        else if (6  == arg_v.size())//eg R1:1:F.Cu:F.Cu:50:1
        {
            ems.add_lumped_port(arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[2]),
                                arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[3]),
                                    openems_model_gen::excitation::DIR_Z, atof(arg_v[4].c_str()), arg_v[5] == "1", false);
            no_port = false;
        }
        else if (3 == arg_v.size())//eg R1:50:1
        {
            ems.add_lumped_port(arg_v[0], atof(arg_v[1].c_str()), arg_v[2] == "1", false);
            no_port = false;
        }
    }
    if (no_port)
    {
        printf("err: no valid port exists\n");
        return 0;
    }
    
    ems.gen_sparameter_scripts(prefix);
    return 0;
}


static int main_antenna(int argc, char **argv)
{
    float max_freq = 3e9;
    float criteria = -50;
    const char *prefix = "";
    std::list<std::string> nets;
    std::vector<std::string> footprints;
    std::vector<std::string> ports;
    std::vector<std::string> lumped_elements;
    std::vector<std::string> mesh_range;
    std::vector<std::string> freq;
    std::string nf2ff_fp;
    std::string bc = "MUR";
    
    openems_model_gen ems(pcb_);
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        
        if (std::string(arg) == "-net" && i < argc)
        {
            _parse_net(arg_next, nets);
        }
        else if (std::string(arg) == "-max_freq" && i < argc)
        {
            max_freq = atof(arg_next);
        }
        else if (std::string(arg) == "-freq" && i < argc)
        {
            freq = _string_split(arg_next, ",");
        }
        else if (std::string(arg) == "-fp" && i < argc)
        {
            footprints.push_back(arg_next);
        }
        else if (std::string(arg) == "-port" && i < argc)
        {
            ports.push_back(arg_next);
        }
        else if (std::string(arg) == "-le" && i < argc)
        {
            lumped_elements.push_back(arg_next);
        }
        else if (std::string(arg) == "-mesh_range" && i < argc)
        {
            mesh_range.push_back(arg_next);
        }
        else if (std::string(arg) == "-bc" && i < argc)
        {
            bc = arg_next;
        }
        else if (std::string(arg) == "-criteria" && i < argc)
        {
            criteria = atof(arg_next);
        }
        else if (std::string(arg) == "-nf2ff" && i < argc)
        {
            nf2ff_fp = arg_next;
        }
        else if (std::string(arg) == "-o" && i < argc)
        {
            prefix = arg_next;
        }
    }
    
    pcb_->clean_segment(nets);
    
    ems.set_boundary_cond((bc == "PML")? openems_model_gen::BC_PML: openems_model_gen::BC_MUR);
    for (const auto& f1: freq)
    {
        float f2 = atof(f1.c_str());
        if (f2 > 1)
        {
            ems.add_freq(f2);
        }
    }
    
    if (max_freq > 1)
    {
        ems.set_excitation_freq(0, max_freq);
    }
    if (criteria < 0)
    {
        ems.set_end_criteria(criteria);
    }
    
    for (const auto& net: nets)
    {
        ems.add_net(pcb_->get_net_id(net), false);
    }
    
    if (footprints.empty())
    {
        printf("err: footprint empty\n");
        return 0;
    }
    
    for (const auto& fp: footprints)
    {
        ems.add_footprint(fp, false);
    }
    
    if (!nf2ff_fp.empty())
    {
        ems.set_nf2ff_footprint(nf2ff_fp);
    }
    
    ems.set_mesh_min_gap(0.01, 0.01, 0.01);
    if (mesh_range.empty())
    {
        ems.add_mesh_range(pcb_->get_edge_left(), pcb_->get_edge_right(), 0.1, openems_model_gen::mesh::DIR_X);
        ems.add_mesh_range(pcb_->get_edge_top(), pcb_->get_edge_bottom(), 0.1, openems_model_gen::mesh::DIR_Y);
    }
    
    for (const auto& range: mesh_range)
    {
        std::vector<std::string> arg_v = _string_split(range, ":");
        if (4 == arg_v.size())
        {
            ems.add_mesh_range(atof(arg_v[0].c_str()), atof(arg_v[1].c_str()), atof(arg_v[2].c_str()), arg_v[3] == "x"? openems_model_gen::mesh::DIR_X: openems_model_gen::mesh::DIR_Y);
        }
    }
    
    for (const auto& le: lumped_elements)
    {
        std::vector<std::string> arg_v = _string_split(le, ":");
        if (1 == arg_v.size())
        {
            ems.add_lumped_element(arg_v[0], false);
        }
    }
    
    bool no_excitation = true;
    for (const auto& port: ports)
    {
        std::vector<std::string> arg_v = _string_split(port, ":");
        //eg R1:1:F.Cu:R1:2:F.Cu:y:50:1
        if (9 == arg_v.size())
        {
            std::uint32_t ex_dir = openems_model_gen::excitation::DIR_X;
            if (arg_v[6] == "y")
            {
                ex_dir = openems_model_gen::excitation::DIR_Y;
            }
            else if (arg_v[6] == "z")
            {
                ex_dir = openems_model_gen::excitation::DIR_Z;
            }
            
            ems.add_lumped_port(arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[2]),
                                arg_v[3], arg_v[4], pcb_->get_layer_name(arg_v[5]),
                                ex_dir, atof(arg_v[7].c_str()), arg_v[8] == "1", false);
            no_excitation = false;
        }
        else if (6  == arg_v.size())//eg R1:1:F.Cu:F.Cu:50:1
        {
            ems.add_lumped_port(arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[2]),
                                    arg_v[0], arg_v[1], pcb_->get_layer_name(arg_v[3]),
                                    openems_model_gen::excitation::DIR_Z, atof(arg_v[4].c_str()), arg_v[5] == "1", false);
            no_excitation = false;
        }
        else if (3 == arg_v.size())//eg R1:50:1
        {
            ems.add_lumped_port(arg_v[0], atof(arg_v[1].c_str()), arg_v[2] == "1", false);
            no_excitation = false;
        }
    }
    if (no_excitation)
    {
        printf("err: no valid excitation exists\n");
        return 0;
    }
    
    ems.gen_antenna_simulation_scripts(prefix);
    return 0;
}


int main(int argc, char **argv)
{
    //std::shared_ptr<pcb> pcb_(new pcb());
    //std::shared_ptr<z_extractor> z_extr(new z_extractor(pcb_));
    
    kicad_pcb_parser parser;
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        if (std::string(arg) == "-pcb" && i < argc)
        {
            pcb_file = arg_next;
        }
        else if (std::string(arg) == "-t")
        {
            mode = MODE_TL;
        }
        else if (std::string(arg) == "-rl")
        {
            mode = MODE_RL;
        }
        else if (std::string(arg) == "-sp")
        {
            mode = MODE_SP;
        }
        else if (std::string(arg) == "-ant")
        {
            mode = MODE_ANT;
        }
    }
    
    if (pcb_file == NULL)
    {
        return 0;
    }
    
    if (!parser.parse(pcb_file, pcb_))
    {
        return 0;
    }
    
    if (mode == MODE_TL || mode == MODE_RL)
    {
        return main_tl_rl(argc, argv);
    }
    else if (mode == MODE_SP)
    {
        return main_sparameter(argc, argv);
    }
    else if (mode == MODE_ANT)
    {
        return main_antenna(argc, argv);
    }
    
    
    return 0;
}