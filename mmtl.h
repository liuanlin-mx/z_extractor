#ifndef __ATLC_H__
#define __ATLC_H__

#include <cstdint>
#include "Z0_calc.h"

class mmtl: public Z0_calc
{
public:
    mmtl();
    ~mmtl();
    
public:
    virtual void set_tmp_name(const std::string& tmp_name) {}
    
    /* 1个像素代表的长度 */
    virtual void set_precision(float unit = 0.035);
    
    virtual void set_box_size(float w, float h);
    
    virtual void clean();
    virtual void clean_all();
    /* 坐标是盒子的中心点 */
    virtual void add_ground(float x, float y, float w, float thickness);
    virtual void add_ring_ground(float x, float y, float r, float thickness);
    virtual void add_wire(float x, float y, float w, float thickness);
    virtual void add_ring_wire(float x, float y, float r, float thickness);
    virtual void add_coupler(float x, float y, float w, float thickness);
    virtual void add_elec(float x, float y, float w, float thickness, float er = 4.6);
    virtual void add_ring_elec(float x, float y, float r, float thickness, float er = 4.6);
    virtual bool calc_zo(float& Zo, float& v, float& c, float& l);
    virtual bool calc_coupled_zo(float& Zodd, float& Zeven, float& Zdiff, float& Zcomm,
                        float& Lodd, float& Leven, float& Codd, float& Ceven);
                        
private:
    std::string _xsctn;
};

#endif
