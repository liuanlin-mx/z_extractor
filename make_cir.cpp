#include "make_cir.h"
#include <vector>
#include "z_extractor.h"

make_cir::make_cir()
{
}

make_cir::~make_cir()
{
}

std::vector<std::string> str_split(std::string str, const std::string& key)
{
    std::vector<std::string> v;
    std::size_t pos = str.find(key);
    while (pos != str.npos)
    {
        //printf("%s", str.substr(0, pos).c_str());
        v.push_back(str.substr(0, pos));
        str = str.substr(pos + 1);
        pos = str.find(key);
    }
    pos = str.find('\n');
    v.push_back(str.substr(0, pos));
    return v;
}

std::string make_cir::make(const char *cir_file, const std::set<std::string>& net_names, const std::string& net_subckt,
                const std::set<std::string>& reference_value)
{
    std::string cir;
    char buf[512];
    FILE *fp = fopen(cir_file, "rb");
    if (!fp)
    {
        return cir;
    }
    char line[512] = {0};
    int flag = 0;
    while (fgets(line, sizeof(line), fp))
    {
        if (tolower(line[0]) == 'r' || tolower(line[0]) == 'c'
            || tolower(line[0]) == 'l' || tolower(line[0]) == 'q'
            || tolower(line[0]) == 'v' || tolower(line[0]) == 'x')
        {
            if (flag == 0)
            {
                flag = 1;
            }
            std::vector<std::string> v;
            if (tolower(line[0]) == 'x')
            {
                v = str_split(std::string(line + 1), " ");
            }
            else
            {
                v = str_split(std::string(line), " ");
            }
            
            if (reference_value.count(v[0]))
            {
                if (tolower(line[0]) == 'x')
                {
                    cir += "X";
                }
                sprintf(buf, "%s ", v[0].c_str());
                cir += buf;

                for (std::uint32_t i = 1; i < v.size() - 1; i++)
                {
                    if (net_names.count(v[i]))
                    {
                        const std::string& net_name = v[i];
                        sprintf(buf, "%s ", z_extractor::gen_pad_net_name(v[0], z_extractor::format_net_name(net_name)).c_str());
                        cir += buf;
                    }
                    else
                    {
                        sprintf(buf, "%s ", v[i].c_str());
                        cir += buf;
                    }
                }
                sprintf(buf, "%s\n", v.back().c_str());
                cir += buf;
            }
            else
            {
                cir += line;
            }
        }
        else
        {
            if (flag == 1)
            {
                flag = 2;
                cir += net_subckt;
            }
            cir += line;
        }
    }
    fclose(fp);
    return cir;
}