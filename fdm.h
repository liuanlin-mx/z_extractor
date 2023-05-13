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

#ifndef __FDM_H__
#define __FDM_H__

#include <map>
#include "matrix.h"

class fdm
{
public:
    enum
    {
        BC_NONE,
        BC_NEUMANN,
        BC_DIRICHLET
    };
    
    enum
    {
        MATERIAL_DIELECTRIC = 0,
        MATERIAL_METAL
    };
    
    struct material
    {
        material()
            : v(0)
            , er(1.0)
            , type(MATERIAL_DIELECTRIC)
            , bc(BC_NONE)
        {
        }
        float v;
        float er;
        std::uint8_t type;
        std::uint8_t bc;
        std::uint8_t id;
    };
    
    struct voltage
    {
        voltage()
            : v(0)
            , er(1)
            , bc(BC_NONE)
        {
        }
        float v;
        float er;
        std::uint8_t bc;
        std::uint8_t id;
        std::uint8_t er_id;
    };
    
public:
    fdm();
    ~fdm();
public:
    void set_box_size(std::int32_t rows, std::int32_t cols, float h);
    void add_material(std::uint8_t id, material& material);
    void add_metal(std::uint8_t id, float v = 0, std::uint8_t bc = BC_DIRICHLET);
    void add_dielectric(std::uint8_t id, float er);
    const material& get_material(std::uint8_t id);
    void update_material(std::uint8_t id, material& material);
    void update_metal(std::uint8_t id, float v = 0, std::uint8_t bc = BC_DIRICHLET);
    void update_dielectric(std::uint8_t id, float er);
    
    void set_bc(std::uint8_t top = BC_NEUMANN, std::uint8_t bottom = BC_NEUMANN, std::uint8_t left = BC_NEUMANN, std::uint8_t right = BC_NEUMANN);
    void add_point(std::int32_t row, std::int32_t col, std::int8_t id);
    
    void solver(bool ignore_dielectric = false);
    
    float calc_surface_electric_fields(std::uint8_t id, bool ignore_dielectric = false);
    float calc_Q(std::uint8_t id, bool ignore_dielectric = false);
    
    float calc_capacity(std::uint8_t id1, std::uint8_t id2, bool ignore_dielectric);
    void gen_atlc();
    void dump_V();
    void dump_E();
    void dump_E2();
    
private:
    void _init_voltage();
    float _solver_no_er();
    float _solver_er();
    float _calc_surface_electric_fields(std::uint8_t id);
    float _calc_surface_electric_fields_vacuum(std::uint8_t id);
    void _update_material(std::uint8_t id, material& material);
    
private:
    material _material_map[256];
    matrix<material> _material_mat;
    matrix<voltage> _v_mat;
    float _h;
    float _w;
    std::uint8_t _bc_top;
    std::uint8_t _bc_bottom;
    std::uint8_t _bc_left;
    std::uint8_t _bc_right;
};

#endif