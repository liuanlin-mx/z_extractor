#include <stdio.h>
#include <math.h>
#include "kicad_pcb_sim.h"
#include <opencv2/opencv.hpp>
#include "make_cir.h"

#include <wx/app.h>
#include <wx/event.h>
#include "gui/simulation_gui.h"
#include <wx/image.h>
#include "atlc.h"





static void _parse_net(const char *str, std::list<std::string>& nets)
{
    char *save_ptr = NULL;
    static char str_[8192];
    strncpy(str_, str, sizeof(str_));
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


int main(int argc, char **argv)
{
    kicad_pcb_sim pcb;
    static char buf[16 * 1024 * 1024];
    std::list<std::string> nets;
    std::list<std::pair<std::string, std::string> > coupled_nets;
    std::list<std::string> refs;
    const char *pcb_file = NULL;
    bool tl = false;
    const char *oname = NULL;
    float v_ratio = 0.7;
    
    float coupled_max_d = 2;
    float coupled_min_len = 0.2;
    bool lossless_tl = true;
    bool ltra = true;
    float conductivity = 5e7;
    float setup = 0.5;
    float anti_pad_diameter = 0;
    bool via_tl_mode = false;
    bool use_mmtl = true;
    bool enable_openmp = true;
    
    for (std::int32_t i = 1; i < argc; i++)
    {
        const char *arg = argv[i];
        const char *arg_next = argv[i + 1];
        
        printf("argv[%d]:%s\n", i, arg);
        
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
        else if (std::string(arg) == "-v_ratio" && i < argc)
        {
            v_ratio = atof(arg_next);
        }
        else if (std::string(arg) == "-coupled_max_d" && i < argc)
        {
            coupled_max_d = atof(arg_next);
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
        else if (std::string(arg) == "-setup" && i < argc)
        {
            setup = atof(arg_next);
        }
        else if (std::string(arg) == "-anti_pad" && i < argc)
        {
            anti_pad_diameter = atof(arg_next);
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
    
    for (auto& net: nets)
    {
        printf("%s\n", net.c_str());
    }
    
    if (pcb_file == NULL || (nets.empty() && coupled_nets.empty()) || (tl == true && refs.empty()))
    {
        return 0;
    }
    
    FILE *fp = fopen(pcb_file, "rb");
    if (fp == NULL)
    {
        return 0;
    }
    
    fread(buf, 1, sizeof(buf), fp);
    fclose(fp);
    
    pcb.parse(buf);
    pcb.set_coupled_max_d(coupled_max_d);
    pcb.set_coupled_min_len(coupled_min_len);
    pcb.enable_lossless_tl(lossless_tl);
    pcb.enable_ltra_model(ltra);
    pcb.enable_via_tl_mode(via_tl_mode);
    pcb.enable_openmp(enable_openmp);
    pcb.set_conductivity(conductivity);
    pcb.set_setup(setup);
    pcb.set_anti_pad_diameter(anti_pad_diameter);
    pcb.set_calc((use_mmtl)? Z0_calc::Z0_CALC_MMTL: Z0_calc::Z0_CALC_ATLC);
    
    std::string spice;
    std::string info;
    if (tl)
    {
        std::vector<std::uint32_t> v_refs;
        
        for (const auto& net: refs)
        {
            v_refs.push_back(pcb.get_net_id(net.c_str()));
        }
        
        
        for (const auto& net: nets)
        {
            std::string ckt;
            std::string call;
            std::set<std::string> reference_value;
            float td = 0;
            pcb.gen_subckt_zo(pcb.get_net_id(net.c_str()), v_refs, ckt, reference_value, call, td);
            printf("ckt:%s\n", ckt.c_str());
            spice += ckt;
            char str[4096] = {0};
            float c = 299792458000 * v_ratio;
            float len = c * (td / 1000000000.);
            sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", net.c_str(), td, len, len / 0.0254);
            info += str;
        }
        
        for (const auto& coupled: coupled_nets)
        {
            printf("%s %s\n", coupled.first.c_str(), coupled.second.c_str());
            std::string ckt;
            std::string call;
            std::set<std::string> reference_value;
            //float td = 0;
            
            pcb.gen_subckt_coupled_tl(pcb.get_net_id(coupled.first.c_str()), pcb.get_net_id(coupled.second.c_str()), v_refs, ckt, reference_value, call);
    
            printf("ckt:%s\n", ckt.c_str());
            spice += ckt;
            
            //char str[4096] = {0};
            //float c = 299792458000 * v_ratio;
            //float len = c * (td / 1000000000.);
            //sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", coupled.first.c_str(), td, len, len / 0.0254);
            //sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", coupled.second.c_str(), td, len, len / 0.0254);
            //fwrite(str, 1, strlen(str), info_fp);
        }
        
    }
    
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

int mainx(int argc, char **argv)
{
    kicad_pcb_sim pcb;
    static char buf[8 * 1024 * 1024];
    FILE *fp = fopen("../testatlc/testatlc.kicad_pcb", "rb");
    if (fp)
    {
        fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
    }
    pcb.parse(buf);
    pcb.dump();
    
    
    std::string ckt;
    std::string call;
    std::set<std::string> reference_value;
    std::vector<std::uint32_t> refs;
    
    refs.push_back(pcb.get_net_id("GND"));
    refs.push_back(pcb.get_net_id("0"));
    pcb.gen_subckt_coupled_tl(pcb.get_net_id("/SN"), pcb.get_net_id("/SP"), refs, ckt, reference_value, call);
    
    make_cir mcir;
    std::set<std::string> net_names;
    net_names.insert("/SN");
    net_names.insert("/SP");
    printf("ckt:%s\n", ckt.c_str());
    std::string cir = mcir.make("../testatlc/testatlc.cir", net_names, ckt, reference_value);
    
    cir += ".control\n";
    cir += "plot v(\"R4_NET__SN\") v(\"R3_NET__SN\") v(\"R2_NET__SP\") v(\"R1_NET__SP\")";
    cir += "\n.endc\n";
    
	printf("%s\n", cir.c_str());
    {
        FILE *fp = fopen("otest.cir", "wb");
        if (fp)
        {
            fwrite(cir.c_str(), 1, cir.length(), fp);
            fclose(fp);
        }
    }
    
    cv::waitKey();
	return 0;
}

int main2(int argc, char **argv)
{
    kicad_pcb_sim pcb;
    static char buf[8 * 1024 * 1024];
    FILE *fp = fopen("/home/mx/work/pcba/a33_core/a33_core.kicad_pcb", "rb");
    if (fp)
    {
        fread(buf, 1, sizeof(buf), fp);
        fclose(fp);
    }
    pcb.parse(buf);
    pcb.dump();
    
    
    std::string ckt;
    std::string call;
    std::set<std::string> reference_value;
    std::vector<std::uint32_t> refs;
    float td = 0;
    refs.push_back(pcb.get_net_id("GND"));
    refs.push_back(pcb.get_net_id("0"));
    refs.push_back(pcb.get_net_id("VCC_DRAM-1V5"));
    //pcb.gen_subckt_coupled_tl(pcb.get_net_id("/ddr3/DCK_P"), pcb.get_net_id("/SP"), refs, ckt, reference_value);
    
    pcb.gen_subckt_zo(pcb.get_net_id("/ddr3/DCK_N"), refs, ckt, reference_value, call, td);

    printf("ckt:%s\n", ckt.c_str());
    
    cv::waitKey();
	return 0;
}
