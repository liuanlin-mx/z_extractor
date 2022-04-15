#include <stdio.h>
#include <math.h>
#include "kicad_pcb_sim.h"
#include <opencv2/opencv.hpp>
#include "make_cir.h"

int main(int argc, char **argv)
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
    std::set<std::string> reference_value;
    std::vector<std::uint32_t> refs;
    refs.push_back(pcb.get_net_id("GND"));
    refs.push_back(pcb.get_net_id("0"));
    pcb.gen_subckt_coupled_tl(pcb.get_net_id("/SN"), pcb.get_net_id("/SP"), refs, ckt, reference_value);
    
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
