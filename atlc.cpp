#include <string.h>
#include "atlc.h"

atlc::atlc()
    : _bmp_name("atlc.bmp")
    , _pix_unit(0.035)
    , _pix_unit_r(1.0 / _pix_unit)
    , _box_w(10)
    , _box_h(10)
    , _c_x(_box_w / 2)
    , _c_y(_box_h / 3)
{
    _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
    clean();
}

atlc::~atlc()
{
}

void atlc::set_precision(float unit)
{
    _pix_unit = unit;
    _pix_unit_r = 1.0 / _pix_unit;
}

void atlc::set_box_size(float w, float h)
{
    _box_w = w;
    _box_h = h;
    _c_x = _box_w / 2;
    _c_y = _box_h / 2;
    clean();
}


void atlc::clean()
{
    _img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(255, 255, 255));
    
    _wire_w = 0;
    _wire_h = 0;
    _coupler_w = 0;
    _coupler_h = 0;
    _wire_conductivity = 5.0e7;
    _coupler_conductivity = 5.0e7;
}

void atlc::clean_all()
{
    clean();
    _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
    _Zo = 0;
    _c = 0;
    _l = 0;
    _v = 0;
    
    _Zodd = 0;
    _Zeven = 0;
    
}

void atlc::add_ring_ground(float x, float y, float r, float thickness)
{
    _draw_ring(x, y, r, thickness, 0, 255, 0);
}

void atlc::add_ground(float x, float y, float w, float thickness)
{
    _draw(x, y, w, thickness, 0, 255, 0);
}

void atlc::add_wire(float x, float y, float w, float thickness, float conductivity)
{
    _wire_w = w;
    _wire_h = thickness;
    _draw(x, y, w, thickness, 255, 0, 0);
}

void atlc::add_ring_wire(float x, float y, float r, float thickness)
{
    _draw_ring(x, y, r, thickness, 255, 0, 0);
}

void atlc::add_coupler(float x, float y, float w, float thickness, float conductivity)
{
    _coupler_w = w;
    _coupler_h = thickness;
    _draw(x, y, w, thickness, 0, 0, 255);
}

void atlc::add_elec(float x, float y, float w, float thickness, float er)
{
    _ers.insert(er);
    std::uint16_t uer =  er * 1000;
    _draw(x, y, w, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


void atlc::add_ring_elec(float x, float y, float r, float thickness, float er)
{
    _ers.insert(er);
    std::uint16_t uer =  er * 1000;
    _draw_ring(x, y, r, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


bool atlc::calc_Z0(float& Zo, float& v, float& c, float& l, float& r, float& g)
{
    r = g = 0;
    //cv::namedWindow("img", cv::WINDOW_NORMAL);
    //cv::rectangle(_img, cv::Point(0, 0), cv::Point(_unit2pix(_box_w) - 1, _unit2pix(_box_h) - 1), cv::Scalar(0, 255, 0), 1);

    cv::rectangle(_img, cv::Point(0, _img.rows - 1), cv::Point(_img.cols - 1, _img.rows - 1), cv::Scalar(0, 255, 0), 1);
    cv::rectangle(_img, cv::Point(0, 0), cv::Point(_img.cols - 1, 0), cv::Scalar(0, 255, 0), 1);

    //cv::imshow(_get_bmp_name(), _img);
    //cv::waitKey(10);
    
    r = 1.0 / (_wire_w * _wire_h * _wire_conductivity);
    if (_is_some(_last_img, _img))
    {
        Zo = _Zo;
        c = _c;
        l = _l;
        v = _v;
        return true;
    }
    cv::imwrite(_get_bmp_name(), _img);
    _last_img = _img;
    
    _calc_Z0(_img, Zo, v, c, l, r, g);
    return false;
}

bool atlc::calc_coupled_Z0(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2])
{
    cv::rectangle(_img, cv::Point(0, _img.rows - 1), cv::Point(_img.cols - 1, _img.rows - 1), cv::Scalar(0, 255, 0), 1);
    cv::rectangle(_img, cv::Point(0, 0), cv::Point(_img.cols - 1, 0), cv::Scalar(0, 255, 0), 1);
    
    
    float z1;
    float v1;
    float c1;
    float l1;
    float r1;
    float g1;
    
    float z2;
    float v2;
    float c2;
    float l2;
    float r2;
    float g2;
    
    float Lodd;
    float Leven;
    float Codd;
    float Ceven;
    
    r_matrix[0][1] = r_matrix[1][0] = 0;
    r_matrix[0][0] = 1.0 / (_wire_w * _wire_h * _wire_conductivity);
    r_matrix[1][1] = 1.0 / (_coupler_w * _coupler_h * _coupler_conductivity);
    if (_is_some(_last_img, _img))
    {
        Zodd = _Zodd;
        Zeven = _Zeven;
        
        memcpy(c_matrix, _c_matrix, sizeof(_c_matrix));
        memcpy(l_matrix, _l_matrix, sizeof(_l_matrix));
        return true;
    }
    
    _last_img = _img;
    cv::Mat img = _img.clone();
    for (std::int32_t i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            cv::Vec3b& pix = img.at<cv::Vec3b>(i, j);
            if (pix[0] == 255 && pix[1] == 0 && pix[2] == 0)
            {
                pix[0] = pix[1] = pix[2] = 255;
            }
        }
    }
    
    
    cv::imwrite(_get_bmp_name(), img);
    //cv::imshow(_get_bmp_name(), img);
    _calc_Z0(img, z1, v1, c1, l1, r1, g1);
    
    //printf("z1:%f, v1:%f, c1:%f, l1:%f, r1:%f, g1:%f\n", z1, v1, c1, l1, r1, g1);
    //cv::waitKey();
    
    
    img = _img.clone();
    for (std::int32_t i = 0; i < img.rows; i++)
    {
        for (int j = 0; j < img.cols; j++)
        {
            cv::Vec3b& pix = img.at<cv::Vec3b>(i, j);
            if (pix[0] == 0 && pix[1] == 0 && pix[2] == 255)
            {
                pix[0] = pix[1] = pix[2] = 255;
            }
            else if (pix[0] == 255 && pix[1] == 0 && pix[2] == 0)
            {
                pix[0] = 0;
                pix[1] = 0;
                pix[2] = 255;
            }
        }
    }
    
    
    cv::imwrite(_get_bmp_name(), img);
    //cv::imshow(_get_bmp_name(), img);
    _calc_Z0(img, z2, v2, c2, l2, r2, g2);
    
    //printf("z2:%f, v2:%f, c2:%f, l2:%f, r2:%f, g2:%f\n", z2, v2, c2, l2, r2, g2);
    
    
    
    //cv::waitKey();
    
    
    
    cv::imwrite(_get_bmp_name(), _img);
    //cv::imshow(_get_bmp_name(), _img);
    
    char cmd[512] = {0};
    std::string er_str;
    for (auto er: _ers)
    {
        char buf[32] = {0};
        std::uint16_t uer = er * 1000;
        sprintf(buf, "-d 0f%02x%02x=%f ", (uer >> 8) & 0xff, uer & 0xff, er);
        er_str += buf;
    }
    sprintf(cmd, "atlc %s -c 0.001 -S -s %s", er_str.c_str(), _get_bmp_name().c_str());
    //printf("%s\n", cmd);
    
    char buf[1024] = {0};
    FILE *fp = popen(cmd, "r");
    if (fgets(buf, sizeof(buf), fp))
    {
        Zodd = _read_value(buf, "Zodd");
        Zeven = _read_value(buf, "Zeven");
        
        Lodd = _read_value(buf, "Lodd");
        Leven = _read_value(buf, "Leven");
        Codd = _read_value(buf, "Codd");
        Ceven = _read_value(buf, "Ceven");
    
        _Zodd = Zodd;
        _Zeven = Zeven;
        
    }
    pclose(fp);
    
    c_matrix[0][0] = c1;
    c_matrix[0][1] = (Ceven - Codd) * 0.5;
    c_matrix[1][0] = (Ceven - Codd) * 0.5;
    c_matrix[1][1] = c2;
    
    l_matrix[0][0] = l1;
    l_matrix[0][1] = (Leven - Lodd) * 0.5;
    l_matrix[1][0] = (Leven - Lodd) * 0.5;
    l_matrix[1][1] = l2;
    
    memcpy(_c_matrix, c_matrix, sizeof(_c_matrix));
    memcpy(_l_matrix, l_matrix, sizeof(_l_matrix));
    
    //printf("Zodd:%f Zeven:%f %f \n", Zodd, Zeven, sqrt(l1/c1));
    
    //cv::waitKey();
    
    return false;
}

void atlc::_draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y1 = _unit2pix(y + _c_y);
    std::int32_t pix_x1 = _unit2pix(x + _c_x - w / 2);
    std::int32_t pix_x2 = _unit2pix(x + _c_x - w / 2 + w);
    std::int32_t pix_y2 = _unit2pix(y + _c_y + thick);;
    cv::rectangle(_img, cv::Point(pix_x1, pix_y1), cv::Point(pix_x2 - 1, pix_y2 - 1), cv::Scalar(b, g, r), -1);
}

void atlc::_draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y = _unit2pix(y + _c_y);
    std::int32_t pix_x = _unit2pix(x + _c_x);
    cv::circle(_img, cv::Point(pix_x, pix_y), _unit2pix(radius + thick * 0.5), cv::Scalar(b, g, r), _unit2pix(thick));
}

float atlc::_read_value(const char *str, const char *key)
{
    char *s = strstr((char *)str, key);
    if (s == NULL)
    {
        return 0.;
    }
    s += strlen(key);
    while (*s == '=' || *s == ' ' || *s == '\t')s++;
    return atof(s);
}

bool atlc::_is_some(cv::Mat& img1, cv::Mat& img2)
{
    if (img1.cols != img2.cols || img1.rows != img2.rows)
    {
        return false;
    }
    
    std::int32_t count = 0;
    for (std::int32_t row = 0; row < img1.rows; row++)
    {
        for (std::int32_t col = 0; col < img1.cols; col++)
        {
            if (img1.at<std::uint8_t>(row, col) == img2.at<std::uint8_t>(row, col))
            {
                count++;
            }
        }
    }
    
    return count > img1.cols * img1.rows  - _unit2pix(4 * 0.0254);
}


std::string atlc::_get_bmp_name()
{
    return _bmp_name;
}



void atlc::_calc_Z0(cv::Mat img, float& Zo, float& v, float& c, float& l, float& r, float& g)
{
    
    cv::imwrite(_get_bmp_name(), img);
    
    char cmd[512] = {0};
    std::string er_str;
    for (auto er: _ers)
    {
        char buf[32] = {0};
        std::uint16_t uer = er * 1000;
        sprintf(buf, "-d 0f%02x%02x=%f ", (uer >> 8) & 0xff, uer & 0xff, er);
        er_str += buf;
    }
    sprintf(cmd, "atlc %s -c 0.001 -S -s %s", er_str.c_str(), _get_bmp_name().c_str());
    //printf("%s\n", cmd);
    
    char buf[1024] = {0};
    FILE *fp = popen(cmd, "r");
    if (fgets(buf, sizeof(buf), fp))
    {
        //printf("%s\n", buf);
        Zo = _read_value(buf, "Zo");
        c = _read_value(buf, "C");
        l = _read_value(buf, "L");
        v = _read_value(buf, "v");
    
        _Zo = Zo;
        _c = c;
        _l = l;
        _v = v;
    
        //printf("Zo:%f v:%fmm/ns c:%f l:%f\n", Zo, v / 1000000, c, l);
    }
    pclose(fp);
}