#ifndef __ATLC_H__
#define __ATLC_H__

#include <cstdint>
#include <opencv2/opencv.hpp>
#include <set>

class atlc
{
public:
    atlc();
    ~atlc();
    
public:
    void set_bmp_name(const std::string& bmp_name) { _bmp_name = bmp_name; }
    
    /* 1个像素代表的长度 */
    void set_pix_unit(float unit = 0.035);
    
    void set_box_size(float w, float h);
    
    void clean();
    void clean_all();
    /* 坐标是盒子的中心点 */
    void draw_ground(float x, float y, float w, float thickness);
    void draw_ring_ground(float x, float y, float r, float thickness);
    void draw_wire(float x, float y, float w, float thickness);
    void draw_ring_wire(float x, float y, float r, float thickness);
    void draw_coupler(float x, float y, float w, float thickness);
    void draw_elec(float x, float y, float w, float thickness, float er = 4.6);
    void draw_ring_elec(float x, float y, float r, float thickness, float er = 4.6);
    bool calc_zo(float& Zo, float& v, float& c, float& l);
    bool calc_coupled_zo(float& Zodd, float& Zeven, float& Zdiff, float& Zcomm,
                        float& Lodd, float& Leven, float& Codd, float& Ceven);
private:
    std::int32_t _unit2pix(float v) { return round(v * _pix_unit_r);}
    void _draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void _draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b);
    float _read_value(const char *str, const char *key);
    bool _is_some(cv::Mat& img1, cv::Mat& img2);
    std::string _get_bmp_name();
    
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
    float _Zdiff;
    float _Zcomm;
    
    float _Lodd;
    float _Leven;
    float _Codd;
    float _Ceven;
};

#endif
