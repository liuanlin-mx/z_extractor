#include <string.h>
#include "mmtl.h"

static const char *base_xsctn = "package require csdl\n\n"
                "set _title \"Example Coplanar Waveguide\"\n"
                "set ::Stackup::couplingLength \"0.0254\"\n"
                "set ::Stackup::riseTime \"5\"\n"
                "set ::Stackup::frequency \"500MHz\"\n"
                "set ::Stackup::defaultLengthUnits \"meters\"\n"
                "set CSEG 10\n"
                "set DSEG 10\n"
                
                "GroundPlane ground  \\\n"
                "   -thickness 0.035 \\\n"
                "   -yOffset 0.0 \\\n"
                "   -xOffset 0.0\n"
                "DielectricLayer dielAir  \\\n"
                "   -thickness 3 \\\n"
                "   -lossTangent 0.0002 \\\n"
                "   -permittivity 1.0 \\\n"
                "   -permeability 1.0 \\\n"
                "   -yOffset 0.0 \\\n"
                "   -xOffset 0.0\n";
                
mmtl::mmtl()
    : _tmp_name("tmp")
    , _xsctn(base_xsctn)
    , _cond_id(0)
    , _gnd_id(0)
    , _elec_id(0)
    , _pix_unit(0.035)
    , _pix_unit_r(1.0 / _pix_unit)
    , _box_w(10)
    , _box_h(10)
    , _c_x(_box_w / 2)
    , _c_y(_box_h / 3)
    , _Z0(0)
    , _c(0)
    , _l(0)
    , _v(0)
    , _r(0)
    , _g(0)
    , _Zodd(0)
    , _Zeven(0)
    , _wire_w(0)
    , _wire_h(0)
    , _coupler_w(0)
    , _coupler_h(0)
{
    memset(_c_matrix, 0, sizeof(_c_matrix));
    memset(_l_matrix, 0, sizeof(_l_matrix));
}


mmtl::~mmtl()
{
    remove((_tmp_name + ".xsctn").c_str());
    remove((_tmp_name + ".result").c_str());
    remove((_tmp_name + ".dump").c_str());
    remove((_tmp_name + ".result_field_plot_data").c_str());
    remove("nmmtl.dump");
    
}


void mmtl::set_precision(float unit)
{
    _pix_unit = unit;
    _pix_unit_r = 1.0 / _pix_unit;
}

void mmtl::set_box_size(float w, float h)
{
    _box_w = w;
    _box_h = h;
    _c_x = _box_w / 2;
    _c_y = _box_h / 2;
    clean();
}

void mmtl::clean()
{
    _map.clear();
    _xsctn = base_xsctn;
    _cond_id = 0;
    _gnd_id = 0;
    _elec_id = 0;
    
    _img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(255, 255, 255));
}


void mmtl::clean_all()
{
    _map.clear();
    _xsctn = base_xsctn;
    _cond_id = 0;
    _gnd_id = 0;
    _elec_id = 0;
    
    _img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(255, 255, 255));
    _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
}


void mmtl::add_ground(float x, float y, float w, float thickness)
{
    item item_;
    item_.type = ITEM_TYPE_GND;
    item_.x = x;
    item_.y = y;
    item_.w = w;
    item_.h = thickness;
    
    _map.emplace(y, item_);
    _draw(x, y, w, thickness, 0, 255, 0);
}


void mmtl::add_wire(float x, float y, float w, float thickness, float conductivity)
{
    item item_;
    item_.type = ITEM_TYPE_COND;
    item_.x = x;
    item_.y = y;
    item_.w = w;
    item_.h = thickness;
    item_.conductivity = conductivity;
    _map.emplace(y, item_);
    _draw(x, y, w, thickness, 255, 0, 0);
    
    if (fabs(w - _wire_w) > 0.0001 || fabs(thickness - _wire_h) > 0.0001)
    {
        _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
    }
    _wire_w = w;
    _wire_h = thickness;
}


void mmtl::add_coupler(float x, float y, float w, float thickness, float conductivity)
{
    item item_;
    item_.type = ITEM_TYPE_COND;
    item_.x = x;
    item_.y = y;
    item_.w = w;
    item_.h = thickness;
    item_.conductivity = conductivity;
    _map.emplace(y, item_);
    _draw(x, y, w, thickness, 0, 0, 255);
    
    if (fabs(w - _coupler_w) > 0.0001 || fabs(thickness - _coupler_h) > 0.0001)
    {
        _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
    }
    _coupler_w = w;
    _coupler_h = thickness;
    
}


void mmtl::add_elec(float x, float y, float w, float thickness, float er)
{
    item item_;
    item_.type = ITEM_TYPE_ELEC;
    item_.x = x;
    item_.y = y;
    item_.w = w;
    item_.h = thickness;
    item_.er = er;
    _map.emplace(y, item_);
    
    std::uint16_t uer =  er * 1000;
    _draw(x, y, w, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


bool mmtl::calc_Z0(float& Z0, float& v, float& c, float& l, float& r, float& g)
{
    char buf[1024] = {0};
    if (_is_some())
    {
        Z0 = _Z0;
        v = _v;
        c = _c;
        l = _l;
        r = _r;
        g = _g;
        return true;
    }
    
    _last_img = _img;
    
    _build();
    FILE *fp = fopen((_tmp_name + ".xsctn").c_str(), "wb");
    if (fp)
    {
        fwrite(_xsctn.c_str(), 1, _xsctn.length(), fp);
        fclose(fp);
    }
    
    
    FILE *pfp = popen(("mmtl_bem " + _tmp_name).c_str(), "r");
    while (fgets(buf, sizeof(buf), pfp))
    {
    }
    pclose(pfp);
    _read_value(Z0, v, c, l, r, g);
    
    
    _Z0 = Z0;
    _v = v;
    _c = c;
    _l = l;
    _r = r;
    _g = g;
        
    //printf("Z0:%f v:%fmm/ns c:%f l:%f\n", Z0, v / 1000000, c, l);
    return false;
}


bool mmtl::calc_coupled_Z0(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2])
{
    char cmd[512] = {0};
    char buf[1024] = {0};
    g_matrix[0][0] = g_matrix[0][1] = g_matrix[1][0] = g_matrix[1][1];
    
    if (_is_some())
    {
        Zodd = _Zodd;
        Zeven = _Zeven;
        
        memcpy(c_matrix, _c_matrix, sizeof(_c_matrix));
        memcpy(l_matrix, _l_matrix, sizeof(_l_matrix));
        memcpy(r_matrix, _r_matrix, sizeof(_r_matrix));
        return true;
    }
    
    _last_img = _img;
    _build();
    sprintf(buf, "%s.xsctn", _tmp_name.c_str());
    FILE *fp = fopen(buf, "wb");
    if (fp)
    {
        fwrite(_xsctn.c_str(), 1, _xsctn.length(), fp);
        fflush(fp);
        fclose(fp);
    }
    
    
    sprintf(cmd, "mmtl_bem %s", _tmp_name.c_str());
    
    FILE *pfp = popen(cmd, "r");
    while (fgets(buf, sizeof(buf), pfp))
    {
    }
    pclose(pfp);
    _read_value(Zodd, Zeven, c_matrix, l_matrix, r_matrix, g_matrix);
    
    memcpy(_c_matrix, c_matrix, sizeof(_c_matrix));
    memcpy(_l_matrix, l_matrix, sizeof(_l_matrix));
    memcpy(_r_matrix, r_matrix, sizeof(_r_matrix));
    
#if 0
    printf("Zodd:%f Zeven:%f\n", Zodd, Zeven);
    printf("C: ");
    for (int i = 0; i < 2;i++)
    for (std::int32_t j = 0; j < 2; j++)
    {
        printf("%f ", c_matrix[i][j]);
    }
    printf("\nL: ");
    for (int i = 0; i < 2;i++)
    for (std::int32_t j = 0; j < 2; j++)
    {
        printf("%f ", l_matrix[i][j]);
    }
    printf("\n");
#endif
    return false;
}


void mmtl::_add_ground(float x, float y, float w, float thickness)
{
    char buf[128];
    sprintf(buf, "RectangleConductors groundWires%d  \\\n", _gnd_id++);
    _xsctn += buf;
    
    sprintf(buf, "-width %f \\\n", w);
    _xsctn += buf;
    _xsctn += "-pitch 1 \\\n";
    _xsctn += "-conductivity 5.0e7S/m \\\n";
    
    sprintf(buf, "-height %f \\\n", thickness);
    _xsctn += buf;
    
    _xsctn += "-number 1 \\\n";
    
    sprintf(buf, "-yOffset %f \\\n", y);
    _xsctn += buf;
    
    sprintf(buf, "-xOffset %f \n", x - w * 0.5);
    _xsctn += buf;
}

void mmtl::_add_wire(float x, float y, float w, float thickness, float conductivity)
{
    char buf[128];
    
    sprintf(buf, "RectangleConductors cond%d  \\\n", _cond_id++);
    _xsctn += buf;
    
    sprintf(buf, "-width %f \\\n", w);
    _xsctn += buf;
    _xsctn += "-pitch 1 \\\n";
    
    sprintf(buf, "-conductivity %gS/m \\\n", conductivity);
    _xsctn += buf;
    
    sprintf(buf, "-height %f \\\n", thickness);
    _xsctn += buf;
    
    _xsctn += "-number 1 \\\n";
    
    sprintf(buf, "-yOffset %f \\\n", y);
    _xsctn += buf;
    
    sprintf(buf, "-xOffset %f \n", x - w * 0.5);
    _xsctn += buf;
}


void mmtl::_add_elec(float x, float y, float w, float thickness, float er)
{
    char buf[128];
    
    sprintf(buf, "DielectricLayer DielLaye%d  \\\n", _elec_id++);
    _xsctn += buf;
    
    sprintf(buf, "-thickness %f \\\n", thickness);
    _xsctn += buf;
    
    _xsctn += "-lossTangent 0.0002 \\\n";
    
    sprintf(buf, "-permittivity %f \\\n", er);
    _xsctn += buf;
    
    _xsctn += "-permeability 1.0 \\\n";
    
     
    sprintf(buf, "-yOffset %f \\\n", y);
    _xsctn += buf;
    
    sprintf(buf, "-xOffset %f \n", x - w * 0.5);
    _xsctn += buf;
}


void mmtl::_build()
{
    float offset = 0;
    for (const auto& it: _map)
    {
        if (it.second.type == ITEM_TYPE_GND || it.second.type == ITEM_TYPE_COND)
        {
            float x = it.second.x - it.second.w * 0.5;
            if (x < offset)
            {
                offset = x;
            }
        }
    }
    
    offset = -offset;
    
    _xsctn = base_xsctn;
    while (!_map.empty())
    {
        auto it = _map.begin();
        std::size_t cnt = _map.count(it->first);
        auto it2 = it;
        for (std::size_t i = 0; i < cnt; i++)
        {
            if (it->second.type == ITEM_TYPE_GND)
            {
                _add_ground(it->second.x + offset, 0, it->second.w, it->second.h);
            }
            else if (it->second.type == ITEM_TYPE_COND)
            {
                _add_wire(it->second.x + offset, 0, it->second.w, it->second.h, it->second.conductivity);
            }
            it++;
        }
        
        for (std::size_t i = 0; i < cnt; i++)
        {
            if (it2->second.type == ITEM_TYPE_ELEC)
            {
                _add_elec(it2->second.x, 0, it2->second.w, it2->second.h, it2->second.er);
            }
            it2 = _map.erase(it2);
        }
    }
}


void mmtl::_read_value(float & Z0, float & v, float & c, float & l, float& r, float& g)
{
    char buf[4096];
    
    sprintf(buf, "%s.result", _tmp_name.c_str());
    FILE *fp = fopen(buf, "rb");
    if (fp == NULL)
    {
        return;
    }
    
    
    int rlen = fread(buf, 1, sizeof(buf) - 1, fp);
    buf[rlen] = 0;
    fclose(fp);
    
    char *s = strstr(buf, "B( ::cond0R0 , ::cond0R0 )=   ");
    if (s)
    {
        s += strlen("B( ::cond0R0 , ::cond0R0 )=   ");
        c = atof(s) * 1e12;
    }
    
    s = strstr(buf, "L( ::cond0R0 , ::cond0R0 )=   ");
    if (s)
    {
        s += strlen("L( ::cond0R0 , ::cond0R0 )=   ");
        l = atof(s) * 1e9;
    }
    
    s = strstr(buf, "Characteristic Impedance (Ohms):");
    if (s)
    {
        s = strstr(s, "::cond0R0= ");
        if (s)
        {
            s += strlen("::cond0R0= ");
            Z0 = atof(s);
        }
    }
    
    s = strstr(buf, "Propagation Velocity (meters/second):");
    if (s)
    {
        s = strstr(s, "::cond0R0=   ");
        if (s)
        {
            s += strlen("::cond0R0=   ");
            v = atof(s);
        }
    }
    
    s = strstr(buf, "Rdc( ::cond0R0 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("Rdc( ::cond0R0 , ::cond0R0 )=  ");
        r = atof(s);
    }
    g = 0;
}


void mmtl::_read_value(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2])
{
    
    char buf[4096];
    
    sprintf(buf, "%s.result", _tmp_name.c_str());
    FILE *fp = fopen(buf, "rb");
    if (fp == NULL)
    {
        return;
    }
    
    
    int rlen = fread(buf, 1, sizeof(buf) - 1, fp);
    buf[rlen] = 0;
    fclose(fp);
    
    char *s = strstr(buf, "B( ::cond1R1 , ::cond1R1 )=  ");
    if (s)
    {
        s += strlen("B( ::cond1R1 , ::cond1R1 )=  ");
        c_matrix[1][1] = atof(s) * 1e12;
    }
    
    s = strstr(buf, "B( ::cond1R1 , ::cond0R0 )= ");
    if (s)
    {
        s += strlen("B( ::cond1R1 , ::cond0R0 )= ");
        c_matrix[1][0] = atof(s) * 1e12;
    }
    
    s = strstr(buf, "B( ::cond0R0 , ::cond1R1 )= ");
    if (s)
    {
        s += strlen("B( ::cond0R0 , ::cond1R1 )= ");
        c_matrix[0][1] = atof(s) * 1e12;
    }
    
    s = strstr(buf, "B( ::cond0R0 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("B( ::cond0R0 , ::cond0R0 )=  ");
        c_matrix[0][0] = atof(s) * 1e12;
    }
    
    
    
    
    s = strstr(buf, "L( ::cond1R1 , ::cond1R1 )=  ");
    if (s)
    {
        s += strlen("L( ::cond1R1 , ::cond1R1 )=  ");
        l_matrix[1][1] = atof(s) * 1e9;
    }
    
    s = strstr(buf, "L( ::cond1R1 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("L( ::cond1R1 , ::cond0R0 )=  ");
        l_matrix[1][0] = atof(s) * 1e9;
    }
    
    s = strstr(buf, "L( ::cond0R0 , ::cond1R1 )=  ");
    if (s)
    {
        s += strlen("L( ::cond0R0 , ::cond1R1 )=  ");
        l_matrix[0][1] = atof(s) * 1e9;
    }
    
    s = strstr(buf, "L( ::cond0R0 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("L( ::cond0R0 , ::cond0R0 )=  ");
        l_matrix[0][0] = atof(s) * 1e9;
    }
    
    
    s = strstr(buf, "Rdc( ::cond1R1 , ::cond1R1 )=  ");
    if (s)
    {
        s += strlen("Rdc( ::cond1R1 , ::cond1R1 )=  ");
        r_matrix[1][1] = atof(s);
    }
    
    s = strstr(buf, "Rdc( ::cond1R1 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("Rdc( ::cond1R1 , ::cond0R0 )=  ");
        r_matrix[1][0] = atof(s);
    }
    
    s = strstr(buf, "Rdc( ::cond0R0 , ::cond1R1 )=  ");
    if (s)
    {
        s += strlen("Rdc( ::cond0R0 , ::cond1R1 )=  ");
        r_matrix[0][1] = atof(s);
    }
    
    s = strstr(buf, "Rdc( ::cond0R0 , ::cond0R0 )=  ");
    if (s)
    {
        s += strlen("Rdc( ::cond0R0 , ::cond0R0 )=  ");
        r_matrix[0][0] = atof(s);
    }
    
    
    s = strstr(buf, "odd= ");
    if (s)
    {
        s += strlen("odd= ");
        Zodd = atof(s);
    }
    
    s = strstr(buf, "even= ");
    if (s)
    {
        s += strlen("even= ");
        Zeven = atof(s);
    }
}



void mmtl::_draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y1 = _unit2pix(y + _c_y);
    std::int32_t pix_x1 = _unit2pix(x + _c_x - w / 2);
    std::int32_t pix_x2 = _unit2pix(x + _c_x - w / 2 + w);
    std::int32_t pix_y2 = _unit2pix(y + _c_y + thick);;
    cv::rectangle(_img, cv::Point(pix_x1, pix_y1), cv::Point(pix_x2 - 1, pix_y2 - 1), cv::Scalar(b, g, r), -1);
}

void mmtl::_draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y = _unit2pix(y + _c_y);
    std::int32_t pix_x = _unit2pix(x + _c_x);
    cv::circle(_img, cv::Point(pix_x, pix_y), _unit2pix(radius + thick * 0.5), cv::Scalar(b, g, r), _unit2pix(thick));
}


bool mmtl::_is_some()
{
    if (_img.cols != _last_img.cols || _img.rows != _last_img.rows)
    {
        return false;
    }
    
    std::int32_t count = 0;
    for (std::int32_t row = 0; row < _img.rows; row++)
    {
        for (std::int32_t col = 0; col < _img.cols; col++)
        {
            if (_img.at<cv::Vec3b>(row, col) == _last_img.at<cv::Vec3b>(row, col))
            {
                count++;
            }
        }
    }
    return count > _img.cols * _img.rows  - _unit2pix(4 * 0.0254);
}
