#ifndef __KICAD_PCB_PARSER_H__
#define __KICAD_PCB_PARSER_H__

#include "z_extractor.h"

class kicad_pcb_parser
{
public:
    kicad_pcb_parser();
    ~kicad_pcb_parser();
    
public:
    bool parse(const char *filepath, std::shared_ptr<z_extractor> z_extr);
    
private:
    bool _parse(const char *str);
    const char *_parse_label(const char *str, std::string& label);
    const char *_skip(const char *str);
    const char *_parse_zone(const char *str, std::vector<z_extractor::zone>& zones);
    const char *_parse_filled_polygon(const char *str, z_extractor::zone& z);
    const char *_parse_net(const char *str, std::uint32_t& id, std::string& name);
    const char *_parse_segment(const char *str, z_extractor::segment& s);
    const char *_parse_via(const char *str, z_extractor::via& v);
    const char *_parse_number(const char *str, float &num);
    const char *_parse_string(const char *str, std::string& text);
    const char *_parse_postion(const char *str, float &x, float& y);
    const char *_parse_tstamp(const char *str, std::string& tstamp);
    const char *_parse_layers(const char *str, std::list<std::string>& layers);
    const char *_parse_footprint(const char *str);
    const char *_parse_at(const char *str, float &x, float& y, float& angle);
    const char *_parse_reference(const char *str, std::string& footprint_name);
    const char *_parse_pad(const char *str, z_extractor::pad& v);
    
    const char *_parse_setup(const char *str);
    const char *_parse_stackup(const char *str);
    const char *_parse_stackup_layer(const char *str);
    const char *_parse_edge(const char *str);
    
private:
    std::shared_ptr<z_extractor> _z_extr;
    
    float _pcb_top;
    float _pcb_bottom;
    float _pcb_left;
    float _pcb_right;
};

#endif