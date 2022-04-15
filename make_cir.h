#ifndef __MAKE_CIR_H__
#define __MAKE_CIR_H__
#include <string>
#include <set>

class make_cir
{
public:
    make_cir();
    ~make_cir();
    
public:
    std::string make(const char *cir_file, const std::set<std::string>& net_name, const std::string& net_subckt,
                const std::set<std::string>& reference_value);
    
};

#endif