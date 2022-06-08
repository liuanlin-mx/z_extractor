#ifndef __ATLC_H__
#define __ATLC_H__

#include <cstdint>
#include <opencv2/opencv.hpp>
#include <set>

#include "Z0_calc.h"
class atlc: public Z0_calc
{
public:
    atlc();
    virtual ~atlc();
    
public:
    virtual void set_tmp_name(const std::string& tmp_name) { _bmp_name = tmp_name; }
    
    /* 1个像素代表的长度 */
    virtual void set_precision(float unit = 0.035);
    
    virtual void set_box_size(float w, float h);
    
    virtual std::uint32_t get_type() { return Z0_calc::Z0_CALC_ATLC; }
    
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
    virtual bool calc_zo(float& Zo, float& v, float& c, float& l, float& r, float& g);
    
    virtual bool calc_coupled_zo(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2]);
private:
    std::int32_t _unit2pix(float v) { return round(v * _pix_unit_r);}
    void _draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void _draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    float _read_value(const char *str, const char *key);
    bool _is_some(cv::Mat& img1, cv::Mat& img2);
    std::string _get_bmp_name();
    
    
    void _calc_zo(cv::Mat img, float& Zo, float& v, float& c, float& l, float& r, float& g);
private:
    std::string _bmp_name;
    float _pix_unit;
    float _pix_unit_r;
    float _box_w;
    float _box_h;
    float _c_x;
    float _c_y;
    
    cv::Mat _img;
    cv::Mat _last_img;
    std::set<float> _ers;
    
    float _Zo;
    float _c;
    float _l;
    float _v;
    
    float _Zodd;
    float _Zeven;
    
    float _c_matrix[2][2];
    float _l_matrix[2][2];
};

#endif
