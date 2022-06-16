#ifndef __MMTL_H__
#define __MMTL_H__

#include <cstdint>
#include <map>
#include <opencv2/opencv.hpp>
#include "Z0_calc.h"

class mmtl: public Z0_calc
{
private:

    enum
    {
        ITEM_TYPE_ELEC = 0,
        ITEM_TYPE_COND,
        ITEM_TYPE_GND,
    };
    
    struct item
    {
        float type;
        float x;
        float y;
        float w;
        float h;
        
        float er;
        float conductivity;
    };
    
public:
    mmtl();
    virtual ~mmtl();
    
public:
    virtual void set_tmp_name(const std::string& tmp_name) { _tmp_name = tmp_name; }
    
    virtual void set_precision(float unit = 0.035);
    
    virtual void set_box_size(float w, float h);
    
    virtual std::uint32_t get_type() { return Z0_calc::Z0_CALC_MMTL; }
    
    virtual void clean();
    virtual void clean_all();
    
    virtual void add_ground(float x, float y, float w, float thickness);
    virtual void add_wire(float x, float y, float w, float thickness, float conductivity);
    virtual void add_coupler(float x, float y, float w, float thickness, float conductivity);
    virtual void add_elec(float x, float y, float w, float thickness, float er = 4.6);
    virtual bool calc_Z0(float& Zo, float& v, float& c, float& l, float& r, float& g);
    virtual bool calc_coupled_Z0(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2]);
private:
    
    void _add_ground(float x, float y, float w, float thickness);
    void _add_wire(float x, float y, float w, float thickness, float conductivity);
    void _add_elec(float x, float y, float w, float thickness, float er = 4.6);
    
    void _build();
    void _read_value(float & Z0, float & v, float & c, float & l, float& r, float& g);
    void _read_value(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2]);
    
    std::int32_t _unit2pix(float v) { return round(v * _pix_unit_r);}
    void _draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void _draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    bool _is_some();
    
private:

    std::string _tmp_name;
    std::string _xsctn;
    std::uint32_t _cond_id;
    std::uint32_t _gnd_id;
    std::uint32_t _elec_id;
    std::multimap<float, item> _map;
    
    cv::Mat _img;
    cv::Mat _last_img;
    float _pix_unit;
    float _pix_unit_r;
    float _box_w;
    float _box_h;
    float _c_x;
    float _c_y;
    
    float _Z0;
    float _c;
    float _l;
    float _v;
    float _r;
    float _g;
    
    float _Zodd;
    float _Zeven;
    
    float _c_matrix[2][2];
    float _l_matrix[2][2];
    float _r_matrix[2][2];
    
    
    float _wire_w;
    float _wire_h;
    float _coupler_w;
    float _coupler_h;
};

#endif
