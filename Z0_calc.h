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

#ifndef __Z0_CALC_H__
#define __Z0_CALC_H__

#include <cstdint>
#include <memory>

class Z0_calc
{
public:
    enum
    {
        Z0_CALC_ATLC = 0,
        Z0_CALC_MMTL,
        Z0_CALC_FDM,
    };
    
public:
    Z0_calc() {};
    virtual ~Z0_calc() {}
    
public:
    virtual void set_tmp_name(const std::string& tmp_name) {}
    
    /* 设置物体最小尺寸 */
    virtual void set_precision(float unit = 0.035) {}
    
    virtual void set_box_size(float w, float h) {}
    virtual std::uint32_t get_type() = 0;
    
    virtual void clean() = 0;
    virtual void clean_all() = 0;
    
    /* 坐标是盒子的中心点 */
    virtual void add_ground(float x, float y, float w, float thickness) = 0;
    virtual void add_wire(float x, float y, float w, float thickness, float conductivity) = 0;
    virtual void add_coupler(float x, float y, float w, float thickness, float conductivity) = 0;
    virtual void add_elec(float x, float y, float w, float thickness, float er = 4.6) = 0;
    /* 返回true表示返回的结果为上一次计算结果的缓存值  */
    virtual bool calc_Z0(float& Zo, float& v, float& c, float& l, float& r, float& g) = 0;
    virtual bool calc_coupled_Z0(float& Zodd, float& Zeven, float c_matrix[2][2], float l_matrix[2][2], float r_matrix[2][2], float g_matrix[2][2]) = 0;
                        
public:
    static std::shared_ptr<Z0_calc> create(std::uint32_t type);
};

#endif