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

#include <string.h>
#include "fdm_Z0_calc.h"

static void row_vector_mul_add(float *dst_vector, const float *a_vector, float b, const float *c_vector, std::int32_t n)
{
    for (std::int32_t i = 0; i < n; i++)
    {
        dst_vector[i] = a_vector[i] * b + c_vector[i];
    }
}

static void row_vector_div(float *dst_vector, const float *a_vector, float b, std::int32_t n)
{
    for (std::int32_t i = 0; i < n; i++)
    {
        dst_vector[i] = a_vector[i] / b;
    }
}


static void matrix_swap_row(float *matrix, std::int32_t rows, std::int32_t cols, std::int32_t row1, std::int32_t row2)
{
    for (std::int32_t col = 0; col < cols; col++)
    {
        float tmp = matrix[row1 * cols + col];
        matrix[row1 * cols + col] = matrix[row2 * cols + col];
        matrix[row2 * cols + col] = tmp;
    }
}

static void matrix_mul(float *dst_matrix, float *a_matrix, float b, std::int32_t rows, std::int32_t cols)
{
    for (std::int32_t row = 0; row < rows; row++)
    {
        for (std::int32_t col = 0; col < cols; col++)
        {
            dst_matrix[row * cols + col] = a_matrix[row * cols + col] * b;
        }
    }
}

static bool matrix_invert(float *matrix, float *invert, std::int32_t rows, std::int32_t cols)
{
    if (rows != cols)
    {
        return false;
    }
    
    /* 初始化逆矩阵为单位矩阵 */
    for (std::int32_t row = 0; row < rows; row++)
    {
        for (std::int32_t col = 0; col < cols; col++)
        {
            invert[cols * row + col] = 0;
        }
    }
    for (std::int32_t col = 0; col < cols; col++)
    {
        invert[cols * col + col] = 1;
    }
    
    
    /* 从上往下消元 */
    /* 一列一列的进行消元 */
    for (std::int32_t col = 0; col < cols; col++)
    {
        /* 1. 找到主元行 */
        std::int32_t pivot_row = col;
        std::int32_t non_zero_row = pivot_row;
        for (; non_zero_row < rows; non_zero_row++)
        {
            if (matrix[cols * non_zero_row + col] != 0)
            {
                break;
            }
        }
        /* 没有找到非零行 消元无法进行 */
        if (non_zero_row > rows)
        {
            return false;
        }
        
        if (non_zero_row != pivot_row)
        {
            /* 交换行 */
            matrix_swap_row(matrix, rows, cols, pivot_row, non_zero_row);
            matrix_swap_row(invert, rows, cols, pivot_row, non_zero_row);
        }
        
        for (std::int32_t row = pivot_row + 1; row < rows; row++)
        {
            float a = matrix[row * cols + col] / matrix[pivot_row * cols + col];
            row_vector_mul_add(matrix + row * cols, matrix + pivot_row * cols, -a , matrix + row * cols, cols);
            row_vector_mul_add(invert + row * cols, invert + pivot_row * cols, -a , invert + row * cols, cols);
        }
    }
    
    /* 从下往上消元 */
    /* 一列一列的进行消元 */
    for (std::int32_t col = cols - 1; col >= 0; col--)
    {
        /* 1. 找到主元行 */
        std::int32_t pivot_row = col;
        std::int32_t non_zero_row = pivot_row;
        for (; non_zero_row >= 0; non_zero_row--)
        {
            if (matrix[cols * non_zero_row + col] != 0)
            {
                break;
            }
        }
        /* 没有找到非零行 消元无法进行 */
        if (non_zero_row < 0)
        {
            return false;
        }
        
        if (non_zero_row != pivot_row)
        {
            /* 交换行 */
            matrix_swap_row(matrix, rows, cols, pivot_row, non_zero_row);
            matrix_swap_row(invert, rows, cols, pivot_row, non_zero_row);
        }
        
        for (std::int32_t row = pivot_row - 1; row >= 0; row--)
        {
            float a = matrix[row * cols + col] / matrix[pivot_row * cols + col];
            row_vector_mul_add(matrix + row * cols, matrix + pivot_row * cols, -a , matrix + row * cols, cols);
            row_vector_mul_add(invert + row * cols, invert + pivot_row * cols, -a , invert + row * cols, cols);
        }
    }
    
    
    /* 把主元变换为1 */
    for (std::int32_t row = 0; row < rows; row++)
    {
        if (matrix[row * cols + row] == 0)
        {
            continue;
        }
        float a = matrix[row * cols + row];
        row_vector_div(matrix + row * cols, matrix + row * cols, a, cols);
        row_vector_div(invert + row * cols, invert + row * cols, a, cols);
    }
    
    return true;
}

fdm_Z0_calc::fdm_Z0_calc()
    : _pix_unit(0.035)
    , _pix_unit_r(1.0 / _pix_unit)
    , _box_w(10)
    , _box_h(10)
    , _c_x(_box_w / 2)
    , _c_y(_box_h / 3)
    , _fdm_er_id(FDM_ID_ER)
{
    _last_img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(0, 0, 0));
    clean();
}

fdm_Z0_calc::~fdm_Z0_calc()
{
}

void fdm_Z0_calc::set_precision(float unit)
{
    _pix_unit = unit;
    _pix_unit_r = 1.0 / _pix_unit;
}

void fdm_Z0_calc::set_box_size(float w, float h)
{
    _box_w = w;
    _box_h = h;
    _c_x = _box_w / 2;
    _c_y = _box_h / 2;
    clean();
}


void fdm_Z0_calc::clean()
{
    _img = cv::Mat(_unit2pix(_box_h), _unit2pix(_box_w), CV_8UC3, cv::Scalar(255, 255, 255));
    _er_map.clear();
    _fdm_er_id = FDM_ID_ER;
    
    _wire_w = 0;
    _wire_h = 0;
    _coupler_w = 0;
    _coupler_h = 0;
    _wire_conductivity = 5.0e7;
    _coupler_conductivity = 5.0e7;
}

void fdm_Z0_calc::clean_all()
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

void fdm_Z0_calc::add_ring_ground(float x, float y, float r, float thickness)
{
    _draw_ring(x, y, r, thickness, 0, 255, 0);
}

void fdm_Z0_calc::add_ground(float x, float y, float w, float thickness)
{
    _draw(x, y, w, thickness, 0, 255, 0);
}

void fdm_Z0_calc::add_wire(float x, float y, float w, float thickness, float conductivity)
{
    _wire_w = w;
    _wire_h = thickness;
    _draw(x, y, w, thickness, 255, 0, 0);
}

void fdm_Z0_calc::add_ring_wire(float x, float y, float r, float thickness)
{
    _draw_ring(x, y, r, thickness, 255, 0, 0);
}

void fdm_Z0_calc::add_coupler(float x, float y, float w, float thickness, float conductivity)
{
    _coupler_w = w;
    _coupler_h = thickness;
    _draw(x, y, w, thickness, 0, 0, 255);
}

void fdm_Z0_calc::add_elec(float x, float y, float w, float thickness, float er)
{
    std::uint16_t uer =  er * 1000;
    if (_er_map.count(uer) == 0)
    {
        _er_map[uer] = _fdm_er_id++;
    }
    _draw(x, y, w, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


void fdm_Z0_calc::add_ring_elec(float x, float y, float r, float thickness, float er)
{
    std::uint16_t uer =  er * 1000;
    if (_er_map.count(uer) == 0)
    {
        _er_map[uer] = _fdm_er_id++;
    }
    _draw_ring(x, y, r, thickness, 0x0f, (uer >> 8) & 0xff, uer & 0xff);
}


bool fdm_Z0_calc::calc_Z0(float& Zo, float& v, float& c, float& l, float& r, float& g)
{
    r = g = 0;
    
    r = 1.0 / (_wire_w * _wire_h * _wire_conductivity);
    if (_is_some(_last_img, _img))
    {
        Zo = _Zo;
        c = _c;
        l = _l;
        v = _v;
        return true;
    }
    _last_img = _img;
    
    _calc_Z0(_img, Zo, v, c, l, r, g);
    return false;
}

bool fdm_Z0_calc::calc_coupled_Z0(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2])
{
    
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
    
    
    const float EPS0 = 8.85419e-12;
    const float MUE0 = 4 * M_PI * 1e-7;
    fdm fdm;
    _init_fdm(fdm, _img);
    float C_vacuum[4] = {0, 0, 0, 0};
    float L[4] = {0, 0, 0, 0};
    float C[4] = {0, 0, 0, 0};
    
    /* 计算真空中导体电容矩阵 (忽略介电常数) */
    {
        float C11 = 0;
        float C12 = 0;
        float C21 = 0;
        float C22 = 0;
        
        /* 计算C11 C21*/
        fdm.update_metal(FDM_ID_METAL_COND1, 1);
        fdm.update_metal(FDM_ID_METAL_COND2, 0);
        
        fdm.solver(true);
        
        float Q1 = fdm.calc_Q(FDM_ID_METAL_COND1, true);
        float Q2 = fdm.calc_Q(FDM_ID_METAL_COND2, true);
        
        C11 = Q1;
        C21 = Q2;
        
        
        /* 计算C22 C12*/
        fdm.update_metal(FDM_ID_METAL_COND1, 0);
        fdm.update_metal(FDM_ID_METAL_COND2, 1);
        
        fdm.solver(true);
        
        Q1 = fdm.calc_Q(FDM_ID_METAL_COND1, true);
        Q2 = fdm.calc_Q(FDM_ID_METAL_COND2, true);
        C12 = Q1;
        C22 = Q2;
        
        C_vacuum[0] = C11;
        C_vacuum[1] = C12;
        C_vacuum[2] = C21;
        C_vacuum[3] = C22;
    }
    
    //printf("C_vacuum matrix\n" "%g\t%g\n%g\t%g\n", C_vacuum[0], C_vacuum[1], C_vacuum[2], C_vacuum[3]);
            
    /* 根据真空中导体电容矩阵求解电感矩阵 */
    {
        float matrix_c[4] = {C_vacuum[0], C_vacuum[1], C_vacuum[2], C_vacuum[3]};
        
        /* 对电容矩阵求逆 */
        matrix_invert((float *)matrix_c, L, 2, 2);
        
        /* 再乘以真空介电常数和真空磁导率 就得到了电感矩阵 */
        matrix_mul(L, L, EPS0 * MUE0, 2, 2);
            
        l_matrix[0][0] = L[0] * 1e9;
        l_matrix[0][1] = L[1] * 1e9;
        l_matrix[1][0] = L[2] * 1e9;
        l_matrix[1][1] = L[3] * 1e9;
    }
    
    /* 计算 电介质中导体的电容矩阵 */
    {
        float C11 = 0;
        float C12 = 0;
        float C21 = 0;
        float C22 = 0;
        
        /* 计算C11 C21*/
        fdm.update_metal(FDM_ID_METAL_COND1, 1);
        fdm.update_metal(FDM_ID_METAL_COND2, 0);
        
        fdm.solver();
        
        float Q1 = fdm.calc_Q(FDM_ID_METAL_COND1);
        float Q2 = fdm.calc_Q(FDM_ID_METAL_COND2);
        
        C11 = Q1;
        C21 = Q2;
        
        
        /* 计算C22 C12*/
        fdm.update_metal(FDM_ID_METAL_COND1, 0);
        fdm.update_metal(FDM_ID_METAL_COND2, 1);
        
        fdm.solver();
        
        Q1 = fdm.calc_Q(FDM_ID_METAL_COND1);
        Q2 = fdm.calc_Q(FDM_ID_METAL_COND2);
        C12 = Q1;
        C22 = Q2;
        
        C[0] = C11;
        C[1] = C12;
        C[2] = C21;
        C[3] = C22;
        
        c_matrix[0][0] = C11 * 1e12;
        c_matrix[0][1] = C12 * 1e12;
        c_matrix[1][0] = C21 * 1e12;
        c_matrix[1][1] = C22 * 1e12;
        
    }
    
    Zodd = (sqrt((L[0] - L[1]) / (C[0] - C[1])) + sqrt((L[3] - L[2]) / (C[3] - C[2]))) / 2;
    Zeven = (sqrt((L[0] + L[1]) / (C[0] + C[1])) + sqrt((L[3] + L[2]) / (C[3] + C[2]))) / 2;

    _Zodd = Zodd;
    _Zeven = Zeven;
    
    memcpy(_c_matrix, c_matrix, sizeof(_c_matrix));
    memcpy(_l_matrix, l_matrix, sizeof(_l_matrix));
    
    return false;
}

void fdm_Z0_calc::_draw(float x, float y, float w, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y1 = _unit2pix(y + _c_y);
    std::int32_t pix_x1 = _unit2pix(x + _c_x - w / 2);
    std::int32_t pix_x2 = _unit2pix(x + _c_x - w / 2 + w);
    std::int32_t pix_y2 = _unit2pix(y + _c_y + thick);;
    cv::rectangle(_img, cv::Point(pix_x1, pix_y1), cv::Point(pix_x2 - 1, pix_y2 - 1), cv::Scalar(b, g, r), -1);
}

void fdm_Z0_calc::_draw_ring(float x, float y, float radius, float thick, std::uint8_t r, std::uint8_t g, std::uint8_t b)
{
    std::int32_t pix_y = _unit2pix(y + _c_y);
    std::int32_t pix_x = _unit2pix(x + _c_x);
    cv::circle(_img, cv::Point(pix_x, pix_y), _unit2pix(radius + thick * 0.5), cv::Scalar(b, g, r), _unit2pix(thick));
}

float fdm_Z0_calc::_read_value(const char *str, const char *key)
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

bool fdm_Z0_calc::_is_some(cv::Mat& img1, cv::Mat& img2)
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
            if (img1.at<cv::Vec3b>(row, col) == img2.at<cv::Vec3b>(row, col))
            {
                count++;
            }
        }
    }
    
    return count > img1.cols * img1.rows  - _unit2pix(4 * 0.0254);
}





void fdm_Z0_calc::_init_fdm(fdm& fdm, cv::Mat& img)
{
    fdm.set_box_size(img.rows, img.cols, _pix_unit);
    //fdm.set_bc(fdm::BC_DIRICHLET, fdm::BC_DIRICHLET, fdm::BC_DIRICHLET, fdm::BC_DIRICHLET);
    fdm.set_bc(fdm::BC_DIRICHLET, fdm::BC_DIRICHLET, fdm::BC_NEUMANN, fdm::BC_NEUMANN);
    //fdm.set_bc(fdm::BC_NEUMANN, fdm::BC_NEUMANN, fdm::BC_DIRICHLET, fdm::BC_DIRICHLET);
    //fdm.set_bc(fdm::BC_NEUMANN, fdm::BC_NEUMANN, fdm::BC_NEUMANN, fdm::BC_DIRICHLET);
    fdm.add_dielectric(FDM_ID_AIR, 1);
    fdm.add_metal(FDM_ID_METAL_GND, 0);
    fdm.add_metal(FDM_ID_METAL_COND1, 1);
    fdm.add_metal(FDM_ID_METAL_COND2, -1);
    
    for (auto& it: _er_map)
    {
        float er = it.first / 1000.;
        std::uint8_t fdm_er_id = it.second;
        fdm.add_dielectric(fdm_er_id, er);
    }
    
    for (std::int32_t col = 0; col < img.cols; col++)
    {
        for (std::int32_t row = 0; row < img.rows; row++)
        {
            std::uint8_t b = img.at<cv::Vec3b>(row, col)[0];
            std::uint8_t g = img.at<cv::Vec3b>(row, col)[1];
            std::uint8_t r = img.at<cv::Vec3b>(row, col)[2];
            
            if (r == 255 && g == 0 && b == 0)
            {
                fdm.add_point(row, col, FDM_ID_METAL_COND1);
            }
            else if (r == 0 && g == 0 && b == 255)
            {
                fdm.add_point(row, col, FDM_ID_METAL_COND2);
            }
            else if (r == 0 && g == 255 && b == 0)
            {
                fdm.add_point(row, col, FDM_ID_METAL_GND);
            }
            else if (r == 0x0f)
            {
                std::uint16_t uer = b | (g << 8);
                if (_er_map.count(uer))
                {
                    fdm.add_point(row, col, _er_map[uer]);
                }
            }
        }
    }
    //fdm.gen_atlc();
}

void fdm_Z0_calc::_calc_Z0(cv::Mat img, float& Z0, float& v, float& c, float& l, float& r, float& g)
{
    const float EPS0 = 8.85419e-12;
    const float MUE0 = 4 * M_PI * 1e-7;
    fdm fdm;
    _init_fdm(fdm, img);
    
    /* 计算真空下的电容 */
    fdm.solver(true);
    float C0 = fdm.calc_Q(FDM_ID_METAL_COND1, true);
    
    /* 计算电感 */
    l = EPS0 * MUE0 / C0;
    
    /* 计算电介质下的电容 */
    fdm.solver(false);
    c = fdm.calc_Q(FDM_ID_METAL_COND1, false);
    
    Z0 = sqrt(l / c);
    float velocity = 1.0 / sqrt(l * c);
    l = l * 1e9; //nH
    c = c * 1e12; //pF
    v = velocity;
    
    _Zo = Z0;
    _c = c;
    _l = l;
    _v = v;
    //printf("Z0:%g velocity:%g C0:%g c:%g l:%g\n", Z0, velocity, C0, c, l);
}