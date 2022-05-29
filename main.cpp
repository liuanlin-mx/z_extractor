#include <stdio.h>
#include <math.h>
#include "kicad_pcb_sim.h"
#include <opencv2/opencv.hpp>
#include "make_cir.h"

#include <wx/app.h>
#include <wx/event.h>
#include "gui/simulation_gui.h"
#include <wx/image.h>

class MainApp : public wxApp
{
public:
    MainApp() {}
    virtual ~MainApp() {}

    virtual bool OnInit() {
        // Add the common image handlers
        wxImage::AddHandler( new wxPNGHandler );
        wxImage::AddHandler( new wxJPEGHandler );

        simulation_gui *main_frame = new simulation_gui(NULL);
        SetTopWindow(main_frame);
        return GetTopWindow()->Show();
    }
};

//DECLARE_APP(MainApp)
//IMPLEMENT_APP(MainApp)


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

int main(int argc, char **argv)
{
    kicad_pcb_sim pcb;
    static char buf[16 * 1024 * 1024];
    std::list<std::string> nets;
    std::list<std::string> refs;
    const char *pcb_file = NULL;
    bool tl = false;
    const char *oname = NULL;
    
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
    }
    
    for (auto& net: nets)
    {
        printf("%s\n", net.c_str());
    }
    
    if (pcb_file == NULL || nets.empty() || (tl == true && refs.empty()))
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
    
    if (oname == NULL)
    {
        oname = "test";
    }
    
    FILE *spice_lib_fp = fopen("test.lib", "wb");
    if (spice_lib_fp == NULL)
    {
        return 0;
    }
    
    FILE *info_fp = fopen("info.txt", "wb");
    if (info_fp == NULL)
    {
        fclose(spice_lib_fp);
        return 0;
    }
    
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
            fwrite(ckt.c_str(), 1, ckt.length(), spice_lib_fp);
            char str[4096] = {0};
            float c = 299792458000;
            float len = c * (td / 1000000000.);
            sprintf(str, "net:%s td:%fNS len:(%fmm %fmil)\n", net.c_str(), td, len, len / 0.0254);
            fwrite(str, 1, strlen(str), info_fp);
        }
    }
    
    fclose(spice_lib_fp);
    fclose(info_fp);
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
