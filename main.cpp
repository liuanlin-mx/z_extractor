#include <stdio.h>
#include <math.h>
#include "z_extractor.h"
#include "kicad_pcb_parser.h"
#include <opencv2/opencv.hpp>
#include "make_cir.h"





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



int main(int argc, char **argv)
{
    std::shared_ptr<z_extractor> z_extr(new z_extractor);
    
    kicad_pcb_parser parser;
    static char buf[16 * 1024 * 1024];
    std::list<std::string> nets;
    std::list<std::pair<std::string, std::string> > coupled_nets;
    std::list<std::pair<std::string, std::string> > pads;
    std::list<std::string> refs;
    std::vector<std::string> current;
    const char *pcb_file = NULL;
    bool tl = true;
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
        else if (std::string(arg) == "-pcb" && i < argc)
        {
            pcb_file = arg_next;
        }
        else if (std::string(arg) == "-o" && i < argc)
        {
            oname = arg_next;
        }
        else if (std::string(arg) == "-t")
        {
            tl = true;
        }
        else if (std::string(arg) == "-rl")
        {
            tl = false;
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
    
    if (pcb_file == NULL)
    {
        return 0;
    }
    
    if (tl)
    {
        if (refs.empty() || (nets.empty() && coupled_nets.empty()))
        {
            return 0;
        }
    }
    else
    {
        if (pads.empty() && nets.empty())
        {
            return 0;
        }
    }
    
    if (!parser.parse(pcb_file, z_extr))
    {
        return 0;
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
    z_extr->set_calc((use_mmtl)? Z0_calc::Z0_CALC_MMTL: Z0_calc::Z0_CALC_ATLC);
    
    std::string spice;
    std::string info;
    if (tl)
    {
        std::vector<std::uint32_t> v_refs;
        
        for (const auto& net: refs)
        {
            v_refs.push_back(z_extr->get_net_id(net.c_str()));
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
            if (!z_extr->gen_subckt_zo(z_extr->get_net_id(net.c_str()), v_refs, ckt, footprint, call, Z0_avg, td, velocity_avg))
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
            if (!z_extr->gen_subckt_coupled_tl(z_extr->get_net_id(coupled.first.c_str()), z_extr->get_net_id(coupled.second.c_str()),
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
    else
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
            
            if (z_extr->gen_subckt(z_extr->get_net_id(net.c_str()), ckt, footprint, call))
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