#include <string.h>
#include "fasthenry.h"

fasthenry::fasthenry()
{
}


fasthenry::~fasthenry()
{
}

void fasthenry::clear()
{
    _inp.clear();
}


bool fasthenry::add_wire(const char *name, point start, point end, float w, float h)
{
    char buf[256] = {0};
    sprintf(buf, "N%s_0 x=%f y=%f z=%f\n", name, start.x, start.y, start.z);
    _inp += std::string(buf);
    
    sprintf(buf, "N%s_1 x=%f y=%f z=%f\n", name, end.x, end.y, end.z);
    _inp += std::string(buf);
    
    sprintf(buf, "E%s N%s_0 N%s_1 w=%f h=%f\n", name, name, name, w, h);
    _inp += std::string(buf);
    
    
    return false;
}

bool fasthenry::add_via(const char *name, point start, point end, float drill, float size)
{
    char buf[256] = {0};
    sprintf(buf, "N%s_0 x=%f y=%f z=%f\n", name, start.x, start.y, start.z);
    _inp += std::string(buf);
    
    sprintf(buf, "N%s_1 x=%f y=%f z=%f\n", name, end.x, end.y, end.z);
    _inp += std::string(buf);
    
    sprintf(buf, "E%s N%s_0 N%s_1 w=%f h=%f nhinc=5 nwinc=5\n", name, name, name, drill, drill);
    _inp += std::string(buf);
    
    
    return false;
}


std::string fasthenry::gen_ckt(const char *wire_name, const std::string& name)
{
    std::list<std::string> wire_names;
    wire_names.push_back(std::string(wire_name));
    _call_fasthenry(wire_names);
    return _make_cir(name, 2);
}

std::string fasthenry::gen_ckt2(std::list<std::string> wire_names, const std::string& name)
{
    _call_fasthenry(wire_names);
    return _make_cir(name, 4);
}


void fasthenry::dump()
{
    std::string tmp;
    tmp = "*****\n"
                            ".units mm\n"
                            ".default nwinc=8 nhinc=3 sigma=5.8e4\n";
    tmp += _inp;
    
    tmp += ".freq fmin=1e9 fmax=1e9 ndec=1\n.end\n";
    printf("\n\n\n%s\n\n\n", tmp.c_str());
}


void fasthenry::calc_wire_lr(float w, float h, float len, float& l, float& r)
{
    std::string tmp;
    
    char buf[512] = {0};
    
    tmp = "*****\n"
                            ".units mm\n"
                            ".default nwinc=1 nhinc=1 sigma=5.8e4\n";
    tmp += "N0 x=0 y=0 z=0\n";
    
    sprintf(buf, "N1 x=%f y=0 z=0\n", len);
    tmp += std::string(buf);
    
    sprintf(buf, "E0 N0 N1 w=%f h=%f\n", w, h);
    tmp += std::string(buf);
        
    tmp += ".external N0 N1\n.freq fmin=1e8 fmax=1e8 ndec=1\n.end\n";
    
    FILE *fp = popen("fasthenry > /dev/null", "w");
    if (fp)
    {
        fwrite(tmp.c_str(), 1, tmp.length(), fp);
        while (1)
        {
            if(fgets(buf, sizeof(buf), fp))
            {
                //printf("%s\n", buf);
            }
            else
            {
                break;
            }
        }
        
        pclose(fp);
    }
    fp = popen("ReadOutput Zc.mat", "r");
    if (fp)
    {
        buf[sizeof(buf) - 1] = 0;
        
        while(fgets(buf, sizeof(buf) - 1, fp));
        char *p = strstr(buf, "Row 0: ");
        if (p)
        {
            p += strlen("Row 0: ");
            r = atof(p);
            
            p = strstr(p, "+");
            if (p)
            {
                p += 1;
                l = atof(p);
            }
        }
        fclose(fp);
    }
}

void fasthenry::_call_fasthenry(std::list<std::string> wire_name)
{
    std::string tmp;
    char buf[512];
    tmp = "*****\n"
                            ".units mm\n"
                            ".default nwinc=8 nhinc=3 sigma=5.8e4\n";
    tmp += _inp;
    
    for (auto& name: wire_name)
    {
        sprintf(buf, ".external N%s_0 N%s_1\n", name.c_str(), name.c_str());
        tmp += buf;
    }
    
    
    tmp += ".freq fmin=1e9 fmax=1e9 ndec=1\n.end\n";
    
        
    FILE *fp = popen("fasthenry > /dev/null", "w");
    if (fp)
    {
        fwrite(tmp.c_str(), 1, tmp.length(), fp);
        while (1)
        {
            if(fgets(buf, sizeof(buf), fp))
            {
                //printf("%s\n", buf);
            }
            else
            {
                break;
            }
        }
        
        pclose(fp);
    }
    
}

std::string fasthenry::_make_cir(const std::string& name, std::uint32_t pins)
{
    char buf[512];
    std::string cir;
    cir = ".subckt " + name;
    for (std::uint32_t i = 1; i <= pins; i++)
    {
        sprintf(buf, " %u", i);
        cir += buf;
    }
    cir += "\n";
    
    FILE *fp = popen("MakeLcircuit Zc.mat", "r");
    while (1)
    {
        if(fgets(buf, sizeof(buf), fp))
        {
            if ((buf[0] == 'L' && buf[1] =='Z')
                    || (buf[0] == 'R' && buf[1] =='Z')
                    || (buf[0] == 'K' && buf[1] =='Z')
                    || (buf[0] == 'H' && buf[1] =='Z')
                    || (buf[0] == 'V' && buf[1] =='a'))
            {
                cir += buf;
            }
        }
        else
        {
            break;
        }
    }
    pclose(fp);
    cir += ".ends\n";
    return cir;
}