/*****************************************************************************
*                                                                            *
*  Copyright (C) 2023 Liu An Lin <liuanlin-mx@qq.com>                        *
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
#define FDM_USE_OPENCV 0

#if FDM_USE_OPENCV
#include <opencv2/opencv.hpp>
#endif
#include <math.h>
#include <set>
#include "fdm.h"

#define EPSILON_0 8.854e-12

fdm::fdm()
    : _h(0)
    , _w(1.9)
    , _bc_top(BC_NEUMANN)
    , _bc_bottom(BC_NEUMANN)
    , _bc_left(BC_NEUMANN)
    , _bc_right(BC_NEUMANN)
{
    
}

fdm::~fdm()
{
    
}
void fdm::set_box_size(std::int32_t rows, std::int32_t cols, float h)
{
    _h = h;
    _material_mat.create(rows, cols);
    _v_mat.create(rows + 1, cols + 1);
}


void fdm::add_material(std::uint8_t id, material& material)
{
    _material_map[id] = material;
}

void fdm::add_metal(std::uint8_t id, float v, std::uint8_t bc)
{
    _material_map[id].id = id;
    _material_map[id].type = MATERIAL_METAL;
    _material_map[id].er = 1;
    _material_map[id].v = v;
    _material_map[id].bc = bc;
}

void fdm::add_dielectric(std::uint8_t id, float er)
{
    _material_map[id].id = id;
    _material_map[id].type = MATERIAL_DIELECTRIC;
    _material_map[id].er = er;
    _material_map[id].v = 0;
    _material_map[id].bc = BC_NONE;
}

const fdm::material& fdm::get_material(std::uint8_t id)
{
    return _material_map[id];
}

void fdm::update_material(std::uint8_t id, material& material)
{
    _material_map[id] = material;
    _update_material(id, material);
}

void fdm::update_metal(std::uint8_t id, float v, std::uint8_t bc)
{
    if (_material_map[id].type == MATERIAL_METAL)
    {
        _material_map[id].v = v;
        _material_map[id].bc = bc;
        _update_material(id, _material_map[id]);
    }
}

void fdm::update_dielectric(std::uint8_t id, float er)
{
    if (_material_map[id].type == MATERIAL_DIELECTRIC)
    {
        _material_map[id].er = er;
        _update_material(id, _material_map[id]);
    }
}

void fdm::set_bc(std::uint8_t top, std::uint8_t bottom, std::uint8_t left, std::uint8_t right)
{
    _bc_top = top;
    _bc_bottom = bottom;
    _bc_left = left;
    _bc_right = right;
}

void fdm::add_point(std::int32_t row, std::int32_t col, std::int8_t id)
{
    _material_mat.at(row, col) = _material_map[id];
}


void fdm::solver(bool ignore_dielectric)
{
    _init_voltage();
    
    float t = cos(M_PI / _v_mat.rows()) + cos(M_PI / _v_mat.cols());
    _w = (8 - sqrt(64 - 16 * t *t)) / (t * t);
    //printf("t:%f w:%f\n", t, _w);
    
    float minR = 1.0 / (_v_mat.rows() * _v_mat.cols());
    if (ignore_dielectric)
    {
        while (1)
        {
            float R = _solver_no_er();
            if (R < minR)
            {
                return;
            }
        }
    }
    else
    {
        while (1)
        {
            float R = _solver_er();
            if (R < minR)
            {
                return;
            }
        }
    }
}

float fdm::calc_surface_electric_fields(std::uint8_t id, bool ignore_dielectric)
{
    if (ignore_dielectric)
    {
        return _calc_surface_electric_fields_vacuum(id);
    }
    else
    {
        return _calc_surface_electric_fields(id);
    }
}


float fdm::calc_Q(std::uint8_t id, bool ignore_dielectric)
{
    if (ignore_dielectric)
    {
        return _calc_surface_electric_fields_vacuum(id) * EPSILON_0;
    }
    else
    {
        return _calc_surface_electric_fields(id) * EPSILON_0;
    }
}

float fdm::calc_capacity(std::uint8_t id1, std::uint8_t id2, bool ignore_dielectric)
{
    if (ignore_dielectric)
    {
        float E1 = _calc_surface_electric_fields_vacuum(id1);
        float E2 = _calc_surface_electric_fields_vacuum(id2);
        printf("E1:%f E2:%f\n", E1, E2);
        float Q1 = E1 * EPSILON_0;
        float Q2 = E2 * EPSILON_0;
        printf("Q1:%g Q2:%g\n", Q1, Q2);
        return Q1 / (_material_map[id1].v - _material_map[id2].v);
    }
    else
    {
        float E1 = _calc_surface_electric_fields(id1);
        float E2 = _calc_surface_electric_fields(id2);
        printf("E1:%f E2:%f\n", E1, E2);
        float Q1 = E1 * EPSILON_0;
        float Q2 = E2 * EPSILON_0;
        printf("Q1:%g Q2:%g\n", Q1, Q2);
        return Q1 / (_material_map[id1].v - _material_map[id2].v);
    }
    
}

void fdm::gen_atlc()
{
#if FDM_USE_OPENCV
    std::set<std::uint32_t> erset;
    cv::Mat img(_material_mat.rows(), _material_mat.cols(), CV_8UC3, cv::Scalar(0, 0, 0));
    for (std::int32_t col = 1; col < _material_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 1; row < _material_mat.rows() - 1; row++)
        {
            if (_material_mat.at(row, col).type == MATERIAL_METAL)
            {
                if (_material_mat.at(row, col).v == 1)
                {
                    img.at<cv::Vec3b>(row, col)[2] = 255;
                }
                else if (_material_mat.at(row, col).v == 0)
                {
                    img.at<cv::Vec3b>(row, col)[1] = 255;
                }
                else if (_material_mat.at(row, col).v == -1)
                {
                    img.at<cv::Vec3b>(row, col)[0] = 255;
                }
            }
            else
            {
                if (_material_mat.at(row, col).er == 1.0)
                {
                    img.at<cv::Vec3b>(row, col)[2] = 255;
                    img.at<cv::Vec3b>(row, col)[1] = 255;
                    img.at<cv::Vec3b>(row, col)[0] = 255;
                }
                else
                {
                    std::uint32_t eps = (_material_mat.at(row, col).er * 1000.);
                    img.at<cv::Vec3b>(row, col)[0] = eps & 0xff;
                    img.at<cv::Vec3b>(row, col)[1] = (eps >> 8) & 0xff;
                    img.at<cv::Vec3b>(row, col)[2] = (eps >> 16) & 0xff;
                    if (erset.count(eps) == 0)
                    {
                        erset.insert(eps);
                        printf("%02x%02x%02x eps:%f\n", (eps >> 16) & 0xff, (eps >> 8) & 0xff, eps & 0xff, _material_mat.at(row, col).er);
                    }
                }
            }
        }
    }
    
    
    for (std::int32_t col = 0; col < _material_mat.cols(); col++)
    {
        img.at<cv::Vec3b>(0, col)[1] = 255;
        img.at<cv::Vec3b>(_material_mat.rows() - 1, col)[1] = 255;
        
        img.at<cv::Vec3b>(0, col)[0] = 0;
        img.at<cv::Vec3b>(0, col)[2] = 0;
        img.at<cv::Vec3b>(_material_mat.rows() - 1, col)[0] = 0;
        img.at<cv::Vec3b>(_material_mat.rows() - 1, col)[2] = 0;
    }
    
    for (std::int32_t row = 0; row < _material_mat.rows(); row++)
    {
        img.at<cv::Vec3b>(row, 0)[1] = 255;
        img.at<cv::Vec3b>(row, _material_mat.cols() - 1)[1] = 255;
        
        img.at<cv::Vec3b>(row, 0)[0] = 0;
        img.at<cv::Vec3b>(row, 0)[2] = 0;
        img.at<cv::Vec3b>(row, _material_mat.cols() - 1)[0] = 0;
        img.at<cv::Vec3b>(row, _material_mat.cols() - 1)[2] = 0;
    }
    
    cv::imwrite("atlc.bmp", img);
    //cv::imshow("atlc", img);
    //cv::waitKey(10);
#endif
}

void fdm::dump_V()
{
#if FDM_USE_OPENCV
    cv::Mat img(_v_mat.rows(), _v_mat.cols(), CV_8UC3, cv::Scalar(0, 0, 0));
    for (std::int32_t col = 0; col < _v_mat.cols(); col++)
    {
        for (std::int32_t row = 0; row < _v_mat.rows(); row++)
        {
            voltage& v = _v_mat.at(row, col);
            
            if (v.bc == BC_DIRICHLET)
            {
                if (v.v > 0)
                {
                    img.at<cv::Vec3b>(row, col)[2] = fabs(v.v) * 255;
                }
                else if (v.v == 0)
                {
                    img.at<cv::Vec3b>(row, col)[0] = 255;
                }
                else
                {
                    img.at<cv::Vec3b>(row, col)[1] = fabs(v.v) * 255;
                }
            }
            else
            {
                if (v.v > 0)
                {
                    img.at<cv::Vec3b>(row, col)[2] = fabs(v.v) * 255;
                }
                else
                {
                    img.at<cv::Vec3b>(row, col)[1] = fabs(v.v) * 255;
                }
            }
        }
    }
    cv::imwrite("dump_v.bmp", img);
    //cv::imshow("dump_v", img);
    //cv::waitKey(10);
#endif
}

void fdm::dump_E()
{
#if FDM_USE_OPENCV
    cv::Mat img(_v_mat.rows(), _v_mat.cols(), CV_8UC3, cv::Scalar(0, 0, 0));
    double max = 0;
    for (std::int32_t col = 0; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 0; row < _v_mat.rows() - 1; row++)
        {
            float Ex = (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
            float Ey = (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
            float E = sqrt(Ex * Ex + Ey * Ey);
            if (E > max)
            {
                max = E;
            }
        }
    }
    
    for (std::int32_t col = 0; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 0; row < _v_mat.rows() - 1; row++)
        {
            float Ex = (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
            float Ey = (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
            float E = sqrt(Ex * Ex + Ey * Ey);
            img.at<cv::Vec3b>(row, col)[2] = 255 * pow(E / max, 0.5);
        }
    }
    
    cv::imwrite("img_E.bmp", img);
    //cv::imshow("img_E", img);
    //cv::waitKey(10);
#endif
}


void fdm::dump_E2()
{
#if FDM_USE_OPENCV
    cv::Mat img(_v_mat.rows(), _v_mat.cols(), CV_8UC3, cv::Scalar(0, 0, 0));
    double max = 0;
    for (std::int32_t col = 0; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 0; row < _v_mat.rows() - 1; row++)
        {
            float Ex1 = (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
            float Ex2 = (_v_mat.at(row + 1, col).v - _v_mat.at(row + 1, col + 1).v) / _h;
            float Ey1 = (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
            float Ey2 = (_v_mat.at(row, col + 1).v - _v_mat.at(row + 1, col + 1).v) / _h;
            float Ex = (Ex1 + Ex2) * 0.5;
            float Ey = (Ey1 + Ey2) * 0.5;
            float E = sqrt(Ex * Ex + Ey * Ey);
            if (E > max)
            {
                max = E;
            }
        }
    }
    
    for (std::int32_t col = 1; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 1; row < _v_mat.rows() - 1; row++)
        {
            float Ex1 = (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
            float Ex2 = (_v_mat.at(row + 1, col).v - _v_mat.at(row + 1, col + 1).v) / _h;
            float Ey1 = (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
            float Ey2 = (_v_mat.at(row, col + 1).v - _v_mat.at(row + 1, col + 1).v) / _h;
            float Ex = (Ex1 + Ex2) * 0.5;
            float Ey = (Ey1 + Ey2) * 0.5;
            float E = sqrt(Ex * Ex + Ey * Ey);
            img.at<cv::Vec3b>(row, col)[2] = 255 * pow(E / max, 0.5);
        }
    }
    
    cv::imwrite("img_E2.bmp", img);
    //cv::imshow("img_E2", img);
    //cv::waitKey(10);
#endif
}


void fdm::_init_voltage()
{
    for (std::int32_t col = 0; col < _material_mat.cols(); col++)
    {
        for (std::int32_t row = 0; row < _material_mat.rows(); row++)
        {
            material& m = _material_mat.at(row, col);
            if (m.type == MATERIAL_DIELECTRIC)
            {
                _v_mat.at(row, col).id = m.id;
                _v_mat.at(row, col).v = m.v;
                _v_mat.at(row, col).bc = m.bc;
                
                _v_mat.at(row, col + 1).id = m.id;
                _v_mat.at(row, col + 1).v = m.v;
                _v_mat.at(row, col + 1).bc = m.bc;
                
                _v_mat.at(row + 1, col).id = m.id;
                _v_mat.at(row + 1, col).v = m.v;
                _v_mat.at(row + 1, col).bc = m.bc;
                
                _v_mat.at(row + 1, col + 1).id = m.id;
                _v_mat.at(row + 1, col + 1).v = m.v;
                _v_mat.at(row + 1, col + 1).bc = m.bc;
            }
        }
    }
    
    for (std::int32_t col = 0; col < _material_mat.cols(); col++)
    {
        for (std::int32_t row = 0; row < _material_mat.rows(); row++)
        {
            material& m = _material_mat.at(row, col);
            
            if (m.type == MATERIAL_METAL)
            {
                _v_mat.at(row, col).id = m.id;
                _v_mat.at(row, col).v = m.v;
                _v_mat.at(row, col).bc = m.bc;
                
                _v_mat.at(row, col + 1).id = m.id;
                _v_mat.at(row, col + 1).v = m.v;
                _v_mat.at(row, col + 1).bc = m.bc;
                
                _v_mat.at(row + 1, col).id = m.id;
                _v_mat.at(row + 1, col).v = m.v;
                _v_mat.at(row + 1, col).bc = m.bc;
                
                _v_mat.at(row + 1, col + 1).id = m.id;
                _v_mat.at(row + 1, col + 1).v = m.v;
                _v_mat.at(row + 1, col + 1).bc = m.bc;
            }
        }
    }
    
    for (std::int32_t col = 0; col < _material_mat.cols(); col++)
    {
        for (std::int32_t row = 0; row < _material_mat.rows(); row++)
        {
            _v_mat.at(row, col).er = _material_mat.at(row, col).er;
        }
    }
    
    for (std::int32_t col = 0; col < _v_mat.cols(); col++)
    {
        _v_mat.at(0, col).bc = _bc_top;
        _v_mat.at(_v_mat.rows() - 1, col).bc = _bc_bottom;
    }
    
    for (std::int32_t row = 0; row < _v_mat.rows(); row++)
    {
        _v_mat.at(row, 0).bc = _bc_left;
        _v_mat.at(row, _v_mat.cols() - 1).bc = _bc_right;
    }
}

float fdm::_solver_no_er()
{
    float max_R = 0;
#if 0
    float xx = 0.25;
    std::int32_t rows = _v_mat.rows();
    std::int32_t cols = _v_mat.cols();
    for (std::int32_t row = 1; row < rows - 1; row++)
    {
        voltage *mid = _v_mat.data() + row * cols;
        voltage *up = mid - cols;
        voltage *down = mid + cols;
        
        for (std::int32_t col = 1; col < cols - 1; col++)
        {
            if (mid[col].bc == BC_NONE)
            {
                float tmp = (up[col].v + down[col].v + mid[col - 1].v + mid[col + 1].v);
                float R = tmp / 4 - mid[col].v;
                mid[col].v = mid[col].v + _w * R;
                if (R > max_R)
                {
                    max_R = R;
                }
            }
            
        }
    }
#else
    for (std::int32_t row = 1; row < _v_mat.rows() - 1; row++)
    {
        for (std::int32_t col = 1; col < _v_mat.cols() - 1; col++)
        {
            if (_v_mat.at(row, col).bc == BC_NONE)
            {
                float w = _w;
                float R = (_v_mat.at(row - 1, col).v + _v_mat.at(row + 1, col).v + _v_mat.at(row, col - 1).v + _v_mat.at(row, col + 1).v) / 4 - _v_mat.at(row, col).v;
                _v_mat.at(row, col).v = _v_mat.at(row, col).v + w * R;
                if (R > max_R)
                {
                    max_R = R;
                }
            }
        }
    }
#endif
    for (std::int32_t col = 0; col < _v_mat.cols(); col++)
    {
        if (_v_mat.at(0, col).bc == BC_NEUMANN)
        {
            _v_mat.at(0, col).v = _v_mat.at(1, col).v;
        }
        
        if (_v_mat.at(_v_mat.rows() - 1, col).bc == BC_NEUMANN)
        {
            _v_mat.at(_v_mat.rows() - 1, col).v = _v_mat.at(_v_mat.rows() - 2, col).v;
        }
    }
    
    for (std::int32_t row = 0; row < _v_mat.rows(); row++)
    {
        if (_v_mat.at(row, 0).bc == BC_NEUMANN)
        {
            _v_mat.at(row, 1).v = _v_mat.at(row, 1).v;
        }
        if (_v_mat.at(row, _v_mat.cols() - 1).bc == BC_NEUMANN)
        {
            _v_mat.at(row, _v_mat.cols() - 1).v = _v_mat.at(row, _v_mat.cols() - 2).v;
        }
    }
    
    return max_R;
}

float fdm::_solver_er()
{
    float max_R = 0;
    for (std::int32_t row = 1; row < _v_mat.rows() - 1; row++)
    {
        for (std::int32_t col = 1; col < _v_mat.cols() - 1; col++)
        {
            if (_v_mat.at(row, col).bc == BC_NONE)
            {
                float a0 = _v_mat.at(row, col).er + _v_mat.at(row - 1, col).er + _v_mat.at(row - 1, col - 1).er + _v_mat.at(row, col - 1).er;
                float a1 = (_v_mat.at(row, col).er + _v_mat.at(row - 1, col).er) * 0.5;
                float a2 = (_v_mat.at(row - 1, col).er + _v_mat.at(row - 1, col - 1).er) * 0.5;
                float a3 = (_v_mat.at(row - 1, col - 1).er + _v_mat.at(row, col - 1).er) * 0.5;
                float a4 = (_v_mat.at(row, col).er + _v_mat.at(row, col - 1).er) * 0.5;
                
                float w = _w;
                float R = (a1 * _v_mat.at(row, col + 1).v + a2 * _v_mat.at(row - 1, col).v + a3 * _v_mat.at(row, col - 1).v + a4 * _v_mat.at(row + 1, col).v) / a0 - _v_mat.at(row, col).v;
                _v_mat.at(row, col).v = _v_mat.at(row, col).v + w * R;
                if (R > max_R)
                {
                    max_R = R;
                }
            }
        }
    }
    
    for (std::int32_t col = 0; col < _v_mat.cols(); col++)
    {
        if (_v_mat.at(0, col).bc == BC_NEUMANN)
        {
            _v_mat.at(0, col).v = _v_mat.at(1, col).v;
        }
        
        if (_v_mat.at(_v_mat.rows() - 1, col).bc == BC_NEUMANN)
        {
            _v_mat.at(_v_mat.rows() - 1, col).v = _v_mat.at(_v_mat.rows() - 2, col).v;
        }
    }
    
    for (std::int32_t row = 0; row < _v_mat.rows(); row++)
    {
        if (_v_mat.at(row, 0).bc == BC_NEUMANN)
        {
            _v_mat.at(row, 0).v = _v_mat.at(row, 1).v;
        }
        if (_v_mat.at(row, _v_mat.cols() - 1).bc == BC_NEUMANN)
        {
            _v_mat.at(row, _v_mat.cols() - 1).v = _v_mat.at(row, _v_mat.cols() - 2).v;
        }
    }
    
    return max_R;
}


float fdm::_calc_surface_electric_fields(std::uint8_t id)
{
    float E = 0;
    for (std::int32_t col = 1; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 1; row < _v_mat.rows() - 1; row++)
        {
            if (_v_mat.at(row, col).id == id && _v_mat.at(row, col + 1).id != id)
            {
                float er = (_v_mat.at(row, col).er + _v_mat.at(row - 1, col).er) * 0.5;
                E += (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) * er / _h;
            }
            
            if (_v_mat.at(row, col).id == id && _v_mat.at(row, col - 1).id != id)
            {
                float er = (_v_mat.at(row, col - 1).er + _v_mat.at(row - 1, col - 1).er) * 0.5;
                E += (_v_mat.at(row, col).v - _v_mat.at(row, col - 1).v) * er / _h;
            }
            
            if (_v_mat.at(row, col).id == id && _v_mat.at(row + 1, col).id != id)
            {
                float er = (_v_mat.at(row, col).er + _v_mat.at(row, col - 1).er) * 0.5;
                E += (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) * er / _h;
            }
            if (_v_mat.at(row, col).id == id && _v_mat.at(row - 1, col).id != id)
            {
                float er = (_v_mat.at(row - 1, col).er + _v_mat.at(row - 1, col - 1).er) * 0.5;
                E += (_v_mat.at(row, col).v - _v_mat.at(row - 1, col).v) * er / _h;
            }
        }
    }
    /* *h 是因为 累积的只是离散网格点上的电场 网格点之间的没有计算在内所以要*h 才是全部的电场积分*/
    return E * _h;
}

float fdm::_calc_surface_electric_fields_vacuum(std::uint8_t id)
{
    float E = 0;
    for (std::int32_t col = 1; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 1; row < _v_mat.rows() - 1; row++)
        {
        #if 0
            if (_v_mat.at(row, col).id == id)
            {
                float Ux1 = _v_mat.at(row, col).v - _v_mat.at(row, col + 1).v;
                float Ux2 = _v_mat.at(row, col).v - _v_mat.at(row, col - 1).v;
                float Uy1 = _v_mat.at(row, col).v - _v_mat.at(row + 1, col).v;
                float Uy2 = _v_mat.at(row, col).v - _v_mat.at(row - 1, col).v;
                
                E += (Ux1 / _h + Ux2 / _h + Uy1 / _h + Uy2 / _h);
                //printf("Ux1:%g Ux2:%g Uy1:%g Uy2:%g\n", Ux1 / _h, Ux2 / _h, Uy1 / _h, Uy2 / _h);
            }
        #else
            if (_v_mat.at(row, col).id == id && _v_mat.at(row, col + 1).id != id)
            {
                E += (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
            }
            
            if (_v_mat.at(row, col).id == id && _v_mat.at(row, col - 1).id != id)
            {
                E += (_v_mat.at(row, col).v - _v_mat.at(row, col - 1).v) / _h;
            }
            
            if (_v_mat.at(row, col).id == id && _v_mat.at(row + 1, col).id != id)
            {
                E += (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
            }
            if (_v_mat.at(row, col).id == id && _v_mat.at(row - 1, col).id != id)
            {
                E += (_v_mat.at(row, col).v - _v_mat.at(row - 1, col).v) / _h;
            }
        #endif
        
        }
    }
    return E * _h;
#if 0
    E = 0;
    for (std::int32_t col = 0; col < _v_mat.cols() - 1; col++)
    {
        for (std::int32_t row = 0; row < _v_mat.rows() - 1; row++)
        {
            if (_v_mat.at(row, col).id == id)
            {
                float Ex1 = (_v_mat.at(row, col).v - _v_mat.at(row, col + 1).v) / _h;
                float Ex2 = (_v_mat.at(row + 1, col).v - _v_mat.at(row + 1, col + 1).v) / _h;
                float Ey1 = (_v_mat.at(row, col).v - _v_mat.at(row + 1, col).v) / _h;
                float Ey2 = (_v_mat.at(row, col + 1).v - _v_mat.at(row + 1, col + 1).v) / _h;
                float Ex = (Ex1 + Ex2) * 0.5;
                float Ey = (Ey1 + Ey2) * 0.5;
                E += sqrt(Ex * Ex + Ey * Ey);
            }
        }
    }
    
    for (std::int32_t col = 1; col < _v_mat.cols(); col++)
    {
        for (std::int32_t row = 1; row < _v_mat.rows(); row++)
        {
            if (_v_mat.at(row, col).id == id)
            {
                float Ex1 = (_v_mat.at(row, col).v - _v_mat.at(row, col - 1).v) / _h;
                float Ex2 = (_v_mat.at(row - 1, col).v - _v_mat.at(row - 1, col - 1).v) / _h;
                float Ey1 = (_v_mat.at(row, col).v - _v_mat.at(row - 1, col).v) / _h;
                float Ey2 = (_v_mat.at(row, col - 1).v - _v_mat.at(row - 1, col - 1).v) / _h;
                float Ex = (Ex1 + Ex2) * 0.5;
                float Ey = (Ey1 + Ey2) * 0.5;
                E += sqrt(Ex * Ex + Ey * Ey);
            }
        }
    }
    printf("E2:%f\n", E);
    return E * _h;
#endif
}

void fdm::_update_material(std::uint8_t id, material& material)
{
    for (std::int32_t col = 0; col < _material_mat.cols(); col++)
    {
        for (std::int32_t row = 0; row < _material_mat.rows(); row++)
        {
            if (_material_mat.at(row, col).id == id)
            {
                _material_mat.at(row, col) = material;
            }
        }
    }
}