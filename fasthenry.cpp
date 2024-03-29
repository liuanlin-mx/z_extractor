/*****************************************************************************
*                                                                            *
*  Copyright (C) 2022-2023 Liu An Lin <liuanlin-mx@qq.com>                   *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/

#include <string.h>
#include <set>
#include <math.h>
#include "fasthenry.h"

#ifdef _WIN32
#define DEV_NULL " NUL "
#else
#define DEV_NULL " /dev/null "
#endif

fasthenry::fasthenry()
    : _conductivity(5.8e7)
    , _freq(1e0)
{
}


fasthenry::~fasthenry()
{
    remove("Zc.mat");
}

void fasthenry::clear()
{
    _inp.clear();
}


bool fasthenry::add_node(const std::string& node_name, point p)
{
    if (_added.count(node_name) == 0)
    {
        char buf[256] = {0};
        _added.insert(node_name);
        sprintf(buf, "N%s x=%.3f y=%.3f z=%.3f\n", node_name.c_str(), p.x, p.y, p.z);
        _inp += std::string(buf);
        return true;
    }
    return false;
}

bool fasthenry::add_wire(const char *name, point start, point end, float w, float h)
{
    char buf[256] = {0};
    sprintf(buf, "N%s_0 x=%f y=%f z=%f\n", name, start.x, start.y, start.z);
    _inp += std::string(buf);
    
    sprintf(buf, "N%s_1 x=%f y=%f z=%f\n", name, end.x, end.y, end.z);
    _inp += std::string(buf);
    
    sprintf(buf, "E%s N%s_0 N%s_1 w=%f h=%f\n", name, name, name, w, h);
    _inp += std::string(buf);
    
    return true;
}


bool fasthenry::add_wire(const std::string& node1_name, const std::string& node2_name, const std::string& wire_name,
                            point start, point end, float w, float h,
                            std::int32_t nwinc, std::int32_t nhinc)
{
    char buf[256] = {0};
    if (_added.count(node1_name) == 0)
    {
        _added.insert(node1_name);
        sprintf(buf, "N%s x=%.3f y=%.3f z=%.3f\n", node1_name.c_str(), start.x, start.y, start.z);
        _inp += std::string(buf);
    }
    
    if (_added.count(node2_name) == 0)
    {
        _added.insert(node2_name);
        sprintf(buf, "N%s x=%.3f y=%.3f z=%.3f\n", node2_name.c_str(), end.x, end.y, end.z);
        _inp += std::string(buf);
    }
    std::int32_t rw = 2;
    std::int32_t rh = 2;
    if (nwinc == 0)
    {
        nwinc = 1;
        if (_freq > 1)
        {
            nwinc = fasthenry::_get_ninc(w, _freq, _conductivity, rw);
        }
    }
    
    if (nhinc == 0)
    {
        nhinc = 1;
        if (_freq > 1)
        {
            nhinc = fasthenry::_get_ninc(h, _freq, _conductivity, rh);
        }
    }
    
    if (_added.count(wire_name) == 0)
    {
        _added.insert(wire_name);
        sprintf(buf, "E%s N%s N%s w=%f h=%f nwinc=%d rw=%d nhinc=%d rh=%d ",
                        wire_name.c_str(), node1_name.c_str(), node2_name.c_str(), w, h,
                        nwinc, rw, nhinc, rh);
        _inp += std::string(buf);
        _inp += "\n";
    }
    
    return true;
}

bool fasthenry::add_via(const std::string& node1_name, const std::string& node2_name, const std::string& wire_name, point start, point end, float drill, float size)
{
    char buf[256] = {0};
    
    
    if (_added.count(node1_name) == 0)
    {
        _added.insert(node1_name);
        sprintf(buf, "N%s x=%.4f y=%.4f z=%.4f\n", node1_name.c_str(), start.x, start.y, start.z);
        _inp += std::string(buf);
    }
    
    if (_added.count(node2_name) == 0)
    {
        _added.insert(node2_name);
        sprintf(buf, "N%s x=%.4f y=%.4f z=%.4f\n", node2_name.c_str(), end.x, end.y, end.z);
        _inp += std::string(buf);
    }
    
    //An的坐标为(p+r*cos(2π*(n-1)/n),q+r*sin(2π*(n-1)/n))
    
    float via_cu_thick = 0.0254;
    float inside_r = (drill +  via_cu_thick) * 0.5;
    float outer_r = size * 0.5;
    
    std::vector<point> inside_points;
    std::vector<point> outer_points;
    
    float cx = start.x;
    float cy = start.y;
    float div_n = 6;
    for (std::uint32_t i = 0; i < div_n; i++)
    {
        float x = cx + inside_r * cos(2 * M_PI * i / div_n);
        float y = cy + inside_r * sin(2 * M_PI * i / div_n);
        inside_points.push_back(point(x, y, 0));
        
        x = cx + outer_r * cos(2 * M_PI * i / div_n);
        y = cy + outer_r * sin(2 * M_PI * i / div_n);
        outer_points.push_back(point(x, y, 0));
    }
    
    std::string inp;
    float w = M_PI * (drill + 2 * via_cu_thick) / div_n;
    std::int32_t rw = 2;
    std::int32_t nwinc = 1;
    if (_freq > 1)
    {
       nwinc = _get_ninc(via_cu_thick, _freq, _conductivity, rw);
    }
    
    for (std::uint32_t i = 0; i < inside_points.size(); i++)
    {
        sprintf(buf, "N%s%d x=%.4f y=%.4f z=%.4f\n", node1_name.c_str(), i, inside_points[i].x, inside_points[i].y, start.z);
        if (_added.count(buf) == 0)
        {
            _added.insert(buf);
            inp += buf;
        }
        sprintf(buf, "N%s%d x=%.4f y=%.4f z=%.4f\n", node2_name.c_str(), i, inside_points[i].x, inside_points[i].y, end.z);
        if (_added.count(buf) == 0)
        {
            _added.insert(buf);
            inp += buf;
        }
            
        /* 连接过孔各层的面 */
        sprintf(buf, "Evid%s%d N%s%d N%s%d w=%f h=%f wx=%f wy=%f wz=0  nhinc=1  nwinc=%d rw=%d\n",
                    wire_name.c_str(), i, node1_name.c_str(), i, node2_name.c_str(), i,
                    via_cu_thick, w,
                    inside_points[i].x - cx, inside_points[i].y - cy,
                    nwinc, rw);
        inp += buf;
        
        sprintf(buf, ".equiv N%s%d N%s\n", node1_name.c_str(), i, node1_name.c_str());
        if (_added.count(buf) == 0)
        {
            _added.insert(buf);
            inp += buf;
        }
        sprintf(buf, ".equiv N%s%d N%s\n", node2_name.c_str(), i, node2_name.c_str());
        if (_added.count(buf) == 0)
        {
            _added.insert(buf);
            inp += buf;
        }
    }
    
    
    //sprintf(buf, "E%s N%s N%s w=%f h=%f nwinc=1 nhinc=1\n", wire_name.c_str(), node1_name.c_str(), node2_name.c_str(), drill, drill);
    //_inp += std::string(buf);
    _inp += inp;
    return true;
}

bool fasthenry::add_via(const char *name, point start, point end, float drill, float size)
{
    char buf[256] = {0};
    sprintf(buf, "N%s_0 x=%.3f y=%.3f z=%.3f\n", name, start.x, start.y, start.z);
    _inp += std::string(buf);
    
    sprintf(buf, "N%s_1 x=%.3f y=%.3f z=%.3f\n", name, end.x, end.y, end.z);
    _inp += std::string(buf);
    
    sprintf(buf, "E%s N%s_0 N%s_1 w=%.3f h=%.3f nhinc=5 nwinc=5\n", name, name, name, drill, drill);
    _inp += std::string(buf);
    
    return false;
}


bool fasthenry::add_equiv(const std::string& node1_name, const std::string& node2_name)
{
    char buf[256] = {0};
    sprintf(buf, ".equiv N%s N%s\n", node1_name.c_str(), node2_name.c_str());
    if (_equiv.count(buf) == 0)
    {
        _equiv.insert(buf);
        _inp += buf;
    }
    return true;
}


bool fasthenry::calc_impedance(const std::string& node1_name, const std::string& node2_name, double& r, double& l)
{
    _call_fasthenry(node1_name, node2_name);
    std::vector<impedance_matrix> ims = _read_impedance_matrix();
    if (ims.empty() || ims[0].values.empty())
    {
        return false;
    }
    r = ims[0].values[0].first;
    l = _calc_inductance(ims[0].freq, ims[0].values[0].second);
    
    return true;
}

std::string fasthenry::gen_ckt(const char *wire_name, const std::string& name)
{
    std::list<std::string> wire_names;
    wire_names.push_back(std::string(wire_name));
    _call_fasthenry(wire_names);
    return _make_cir(name, 2);
}

std::string fasthenry::gen_ckt2(std::list<std::string> wire_names, const std::string& name)
{
    _call_fasthenry(wire_names);
    return _make_cir(name, 4);
}


void fasthenry::dump()
{
    std::string tmp;
    char buf[256];
    std::int32_t nwinc = 1;
    std::int32_t nhinc = 1;
    sprintf(buf, "*****\n"
                    ".units mm\n"
                    ".default nwinc=%d nhinc=%d sigma=%g\n", nwinc, nhinc, _conductivity * 1e-3);
    tmp = buf;
    tmp += _inp;
    
    sprintf(buf, ".freq fmin=%g fmax=%g ndec=1\n.end\n", _freq, _freq);
    tmp += buf;
    printf("\n\n\n%s\n\n\n", tmp.c_str());
}


void fasthenry::calc_wire_lr(float w, float h, float len, float& l, float& r, float conductivity, float freq)
{
    std::string tmp;
    
    char buf[512] = {0};
    
    std::int32_t nwinc = 1;
    std::int32_t nhinc = 1;
    sprintf(buf, "*****\n"
                    ".units mm\n"
                    ".default nwinc=%d nhinc=%d sigma=%g\n", nwinc, nhinc, conductivity * 1e-3);
                    
    tmp += "N0 x=0 y=0 z=0\n";
    
    sprintf(buf, "N1 x=%.3f y=0 z=0\n", len);
    tmp += std::string(buf);
    
    sprintf(buf, "E0 N0 N1 w=%.3f h=%.3f\n", w, h);
    tmp += std::string(buf);
        
    tmp += ".external N0 N1\n";
    
    sprintf(buf, ".freq fmin=%g fmax=%g ndec=1\n.end\n", freq, freq);
    tmp += buf;
    
    FILE *fp = popen("fasthenry > " DEV_NULL, "w");
    if (fp)
    {
        fwrite(tmp.c_str(), 1, tmp.length(), fp);
        while (1)
        {
            if(fgets(buf, sizeof(buf), fp))
            {
                //printf("%s\n", buf);
            }
            else
            {
                break;
            }
        }
        
        pclose(fp);
    }
    fp = popen("ReadOutput Zc.mat", "r");
    if (fp)
    {
        buf[sizeof(buf) - 1] = 0;
        
        while(fgets(buf, sizeof(buf) - 1, fp));
        char *p = strstr(buf, "Row 0: ");
        if (p)
        {
            p += strlen("Row 0: ");
            r = atof(p);
            
            p = strstr(p, "+");
            if (p)
            {
                p += 1;
                l = atof(p);
            }
        }
        fclose(fp);
    }
}

void fasthenry::_call_fasthenry(std::list<std::string> wire_name)
{
    std::string tmp;
    char buf[512];
    
    std::int32_t nwinc = 1;
    std::int32_t nhinc = 1;
    sprintf(buf, "*****\n"
                    ".units mm\n"
                    ".default nwinc=%d nhinc=%d sigma=%g\n", nwinc, nhinc, _conductivity * 1e-3);
                   
    tmp += _inp;
    
    for (auto& name: wire_name)
    {
        sprintf(buf, ".external N%s_0 N%s_1\n", name.c_str(), name.c_str());
        tmp += buf;
    }
    
    sprintf(buf, ".freq fmin=%g fmax=%g ndec=1\n.end\n", _freq, _freq);
    tmp += buf;
    
        
    FILE *fp = popen("fasthenry > " DEV_NULL, "w");
    if (fp)
    {
        fwrite(tmp.c_str(), 1, tmp.length(), fp);
        while (1)
        {
            if(fgets(buf, sizeof(buf), fp))
            {
                //printf("%s\n", buf);
            }
            else
            {
                break;
            }
        }
        
        pclose(fp);
    }
}

void fasthenry::_call_fasthenry(const std::string& node1_name, const std::string& node2_name)
{
    std::string tmp;
    char buf[512];
    std::int32_t nwinc = 1;
    std::int32_t nhinc = 1;
    sprintf(buf, "*****\n"
                    ".units mm\n"
                    ".default nwinc=%d nhinc=%d sigma=%g\n", nwinc, nhinc, _conductivity * 1e-3);
                    
                    
    tmp = buf;
    tmp += _inp;
    
    sprintf(buf, ".external N%s N%s\n", node1_name.c_str(), node2_name.c_str());
    tmp += buf;
    
    sprintf(buf, ".freq fmin=%g fmax=%g ndec=1\n.end\n", _freq, _freq);
    tmp += buf;
    
    FILE *fp = popen("fasthenry > " DEV_NULL, "w");
    if (fp)
    {
        fwrite(tmp.c_str(), 1, tmp.length(), fp);
        //printf("\n\n\n%s\n\n", tmp.c_str());
        while (1)
        {
            if(fgets(buf, sizeof(buf), fp))
            {
                //printf("%s\n", buf);
            }
            else
            {
                break;
            }
        }
        
        pclose(fp);
    }
}


std::string fasthenry::_make_cir(const std::string& name, std::uint32_t pins)
{
    char buf[512];
    std::string cir;
    cir = ".subckt " + name;
    for (std::uint32_t i = 1; i <= pins; i++)
    {
        sprintf(buf, " %u", i);
        cir += buf;
    }
    cir += "\n";
    
    FILE *fp = popen("MakeLcircuit Zc.mat", "r");
    while (1)
    {
        if(fgets(buf, sizeof(buf), fp))
        {
            if ((buf[0] == 'L' && buf[1] =='Z')
                    || (buf[0] == 'R' && buf[1] =='Z')
                    || (buf[0] == 'K' && buf[1] =='Z')
                    || (buf[0] == 'H' && buf[1] =='Z')
                    || (buf[0] == 'V' && buf[1] =='a'))
            {
                cir += buf;
            }
        }
        else
        {
            break;
        }
    }
    pclose(fp);
    cir += ".ends\n";
    return cir;
}



std::vector<fasthenry::impedance_matrix> fasthenry::_read_impedance_matrix()
{
#define LINEMAX 4096
    
    char line[LINEMAX];
    FILE *fp = fopen("Zc.mat", "rb");
    if (fp == NULL)
    {
        return {};
    }
    std::vector<fasthenry::impedance_matrix> ims;
    while(fgets(line, LINEMAX, fp) != NULL)
    {
        impedance_matrix im;
        if (sscanf(line, "Impedance matrix for frequency = %lg %u x %u",
                    &im.freq, &im.rows, &im.cols) == 3)
        {
            for (std::uint32_t row = 0; row < im.rows; row++)
            {
                if (fgets(line, LINEMAX, fp) == NULL)
                {
                    printf("Unexpected end of file\n");
                    printf("err: freq:%lg ros:%u col:%u\n", im.freq, row , 0);
                    break;
                }
                
                const char *ptr = line;
                std::int32_t skip = 0;
                double real = 0;
                double imag = 0;
                std::pair<double, double> value;
                for(std::uint32_t col = 0; col < im.cols; col++)
                {
                    if (sscanf(ptr, "%lf%n", &real, &skip) != 1)
                    {
                        printf("err: freq:%lg ros:%u col:%u\n", im.freq, row , col);
                        continue;
                    }
                    else
                    {
                        value.first = real;
                    }
                    
                    ptr += skip;

                    if (sscanf(ptr, "%lf%n", &imag, &skip) != 1)
                    {
                        printf("err: freq:%lg ros:%u col:%u\n", im.freq, row , col);
                        continue;
                    }
                    else
                    {
                        value.second = imag;
                    }
                    ptr += skip;
                    

                    if (ptr[0] != 'j')
                    {
                        printf("Couldn't read j off of imaginary part\n");
                        printf("err: freq:%lg ros:%u col:%u\n", im.freq, row , col);
                        continue;
                    }
                    im.values.push_back(value);
                    ptr += 1;
                }
            }
            
            if (!im.values.empty())
            {
                ims.push_back(im);
            }
        }
    }
    fclose(fp);
    return ims;
}


double fasthenry::_calc_inductance(double freq, double imag)
{
    return imag / (2 * M_PI * freq);
}

std::int32_t fasthenry::_get_ninc(float w, float freq, float conductivity, std::int32_t& ratio)
{
    float d = sqrt(1 / (M_PI * freq * conductivity * 4 * M_PI * 1e-7)) * 1000.; // 趋肤深度单位mm
    std::int32_t n = 1;
    ratio = 2;
    float min = 1e9;
    std::int32_t a1 = 1;
    for (std::int32_t r = 1; r < 20; r++)
    {
        for (std::int32_t i = 1; i < 12; i++)
        {
            std::int32_t count = a1 * (1 - pow(r, i) / (1 - r)) * 2 + a1 * pow(r, i);
            float tmp = fabs(d - w / count);
            if (tmp < min)
            {
                min = tmp;
                n = i * 2 + 1;
                ratio = r;
            }
        }
    }
    //std::int32_t count = a1 * (1 - pow(ratio, n / 2) / (1 - ratio)) * 2 + a1 * pow(ratio, n / 2);
    //printf("ratio:%d n:%d d:%f v:%f\n", ratio, n, d, w / count);
    return n;
}