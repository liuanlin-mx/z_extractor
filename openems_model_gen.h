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

#ifndef __OPENEMS_MODEL_GEN_H__
#define __OPENEMS_MODEL_GEN_H__

#include <set>
#include <vector>
#include "pcb.h"

class openems_model_gen
{
public:
    enum
    {
        BC_MUR,
        BC_PML,
    };
    
    struct point
    {
        point() : x(0), y(0), z(0) {}
        point(float x, float y, float z) : x(x), y(y), z(z) {}
        float x;
        float y;
        float z;
    };
    
    struct excitation
    {
        enum
        {
            DIR_X,
            DIR_Y,
            DIR_Z,
        };
        
        excitation()
            : dir(DIR_X)
            , R(50)
        {
        }
        
        point start;
        point end;
        
        std::uint32_t dir;
        float R;
    };
    
    struct mesh
    {
        enum
        {
            DIR_X,
            DIR_Y,
            DIR_Z,
        };
        struct line
        {
            line() : v(0), prio(0) {}
            line(float v_, std::uint32_t prio_) : v(v_), prio(prio_) {}
            float v;
            std::uint32_t prio;
            
            bool operator <(const line& b) const
            {
                return this->v < b.v;
            }
        };
        
        struct line_range
        {
            line_range() : start(0), end(0), gap(0), prio(0) {}
            line_range(float start_, float end_, float gap_, float prio_)
                : start(start_), end(end_), gap(gap_), prio(prio_) {}
            
            float start;
            float end;
            float gap;
            std::uint32_t prio;
            bool operator <(const line_range& other) const
            {
                return this->gap < other.gap;
            }
        };
        
        std::set<line> x;
        std::set<line> y;
        std::set<line> z;
        
        std::multiset<line_range> x_range;
        std::multiset<line_range> y_range;
        std::multiset<line_range> z_range;
    };
    
public:
    openems_model_gen(const std::shared_ptr<pcb>& pcb);
    ~openems_model_gen();
    
public:
    void add_net(std::uint32_t net_id);
    void add_footprint(const std::string& fp_ref);
    void add_excitation(const std::string& fp1, const std::string& fp1_pad_number, const std::string& fp1_layer_name,
                        const std::string& fp2, const std::string& fp2_pad_number, const std::string& fp2_layer_name, std::uint32_t dir, float R = 50);
                        
    void add_excitation(pcb::point start, const std::string& start_layer, pcb::point end, const std::string& end_layer, std::uint32_t dir, float R = 50);
    
    void add_mesh_range(float start, float end, float gap, std::uint32_t dir = mesh::DIR_X, std::uint32_t prio = 0);
    
    void set_boundary_cond(std::uint32_t bc);
    void set_nf2ff_footprint(const std::string& fp);
    void set_excitation_freq(float f0, float fc);
    void set_far_field_freq(float freq);
    
    void set_mesh_min_gap(float x_min_gap = 0.1, float y_min_gap = 0.1, float z_min_gap = 0.01);
    
    void gen_model(const std::string& func_name,
                    std::uint32_t segment_prio, std::uint32_t via_prio,
                    std::uint32_t zone_prio, std::uint32_t fp_prio);
    void gen_mesh(const std::string& func_name);
    void gen_antenna_simulation_scripts();
private:
    void _gen_mesh_z(FILE *fp);
    void _gen_mesh_xy(FILE *fp);
    void _add_dielectric(FILE *fp);
    void _add_metal(FILE *fp);
    void _add_segment(FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_via(FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_zone(FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_footprint(FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_gr(const pcb::gr& gr, pcb::point at, float angle, const std::string& name, FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_pad(const pcb::footprint& footprint, const pcb::pad& p, const std::string& name, FILE *fp, std::uint32_t mesh_prio = 0);
    
    void _add_excitation(FILE *fp, std::uint32_t mesh_prio = 0);
    void _add_nf2ff_box(FILE *fp, std::uint32_t mesh_prio = 0);
    
    void _apply_mesh_line_range(mesh& mesh);
    void _apply_mesh_line_range(std::set<mesh::line>& mesh_line, const std::multiset<mesh::line_range>& mesh_line_range);
    void _clean_mesh_line(std::set<mesh::line>& mesh_line, float min_gap = 0.01);
private:
    std::shared_ptr<pcb> _pcb;
    std::set<std::uint32_t> _nets;
    std::set<std::string> _footprints;
    
    
    mesh _mesh;
    float _mesh_x_min_gap;
    float _mesh_y_min_gap;
    float _mesh_z_min_gap;
    float _lambda_mesh_ratio;
    
    bool _ignore_cu_thickness;
    
    std::uint32_t _bc;
    float _f0;
    float _fc;
    float _far_field_freq;
    std::string _nf2ff_fp;
    
    std::vector<excitation> _excitations;
};

#endif