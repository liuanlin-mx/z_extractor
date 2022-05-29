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

void atlc::set_pix_unit(float unit)
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
    _Zdiff = 0;
    _Zcomm = 0;
    
    _Lodd = 0;
    _Leven = 0;
    _Codd = 0;
    _Ceven = 0;
}

void atlc::draw_ground(float x, float y, float w, float thickness)
{
    _draw(x, y, w, thickness, 0, 255, 0);
}

void atlc::draw_wire(float x, float y, float w, float thickness)
{
    _draw(x, y, w, thickness, 255, 0, 0);
}

void atlc::draw_coupler(float x, float y, float w, float thickness)
{
    _draw(x, y, w, thickness, 0, 0, 255);
}

void atlc::draw_elec(float x, float y, float w, float thickness, float er)
{
    _ers.insert(er);
    std::uint16_t uer =  er * 1000;
    _draw(x, y, w, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


bool atlc::calc_zo(float& Zo, float& v, float& c, float& l)
{
    //cv::namedWindow("img", cv::WINDOW_NORMAL);
    //cv::rectangle(_img, cv::Point(0, 0), cv::Point(_unit2pix(_box_w) - 1, _unit2pix(_box_h) - 1), cv::Scalar(0, 255, 0), 1);

    cv::rectangle(_img, cv::Point(0, _img.rows - 1), cv::Point(_img.cols - 1, _img.rows - 1), cv::Scalar(0, 255, 0), 1);
    cv::rectangle(_img, cv::Point(0, 0), cv::Point(_img.cols - 1, 0), cv::Scalar(0, 255, 0), 1);

    cv::imwrite(_get_bmp_name(), _img);
    //cv::imshow(_get_bmp_name(), _img);
    //cv::waitKey(10);
    
    if (_is_some(_last_img, _img))
    {
        Zo = _Zo;
        c = _c;
        l = _l;
        v = _v;
        return true;
    }
    _last_img = _img;
    char cmd[512] = {0};
    std::string er_str;
    for (auto er: _ers)
    {
        char buf[32] = {0};
        std::uint16_t uer = er * 1000;
        sprintf(buf, "-d 0f%02x%02x=%f ", (uer >> 8) & 0xff, uer & 0xff, er);
        er_str += buf;
    }
    sprintf(cmd, "atlc %s -r 1.6 -S -s %s", er_str.c_str(), _get_bmp_name().c_str());
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
    
    }
    pclose(fp);
    return false;
}

bool atlc::calc_coupled_zo(float& Zodd, float& Zeven, float& Zdiff, float& Zcomm,
                        float& Lodd, float& Leven, float& Codd, float& Ceven)
{
    //cv::rectangle(_img, cv::Point(0, _img.rows - 1), cv::Point(_img.cols - 1, _img.rows - 1), cv::Scalar(0, 255, 0), 1);
    //cv::rectangle(_img, cv::Point(0, 0), cv::Point(_img.cols - 1, 0), cv::Scalar(0, 255, 0), 1);

    cv::rectangle(_img, cv::Point(0, 0), cv::Point(_img.cols - 1, _img.rows - 1), cv::Scalar(0, 255, 0), 1);
    
    cv::imwrite(_get_bmp_name(), _img);
    cv::imshow(_get_bmp_name(), _img);
    cv::waitKey(10);
    
    if (_is_some(_last_img, _img))
    {
        Zodd = _Zodd;
        Zeven = _Zeven;
        Zdiff = _Zdiff;
        Zcomm = _Zcomm;
        
        Lodd = _Lodd;
        Leven = _Leven;
        Codd = _Codd;
        Ceven = _Ceven;
        return true;
    }
    _last_img = _img;
    char cmd[512] = {0};
    std::string er_str;
    for (auto er: _ers)
    {
        char buf[32] = {0};
        std::uint16_t uer = er * 1000;
        sprintf(buf, "-d 0f%02x%02x=%f ", (uer >> 8) & 0xff, uer & 0xff, er);
        er_str += buf;
    }
    sprintf(cmd, "atlc %s -r 1.6 -S -s %s", er_str.c_str(), _get_bmp_name().c_str());
    //printf("%s\n", cmd);
    
    char buf[1024] = {0};
    FILE *fp = popen(cmd, "r");
    if (fgets(buf, sizeof(buf), fp))
    {
        //printf("%s\n", buf);
        Zodd = _read_value(buf, "Zodd");
        Zeven = _read_value(buf, "Zeven");
        Zdiff = _read_value(buf, "Zdiff");
        Zcomm = _read_value(buf, "Zcomm");
        
        Lodd = _read_value(buf, "Lodd");
        Leven = _read_value(buf, "Leven");
        Codd = _read_value(buf, "Codd");
        Ceven = _read_value(buf, "Ceven");
    
        _Zodd = Zodd;
        _Zeven = Zeven;
        _Zdiff = Zdiff;
        _Zcomm = Zcomm;
        
        _Lodd = Lodd;
        _Leven = Leven;
        _Codd = Codd;
        _Ceven = Ceven;
    
    }
    pclose(fp);
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