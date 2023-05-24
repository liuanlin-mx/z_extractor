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
    struct point
    {
        float x;
        float y;
        float z;
    };
    
public:
    openems_model_gen(const std::shared_ptr<pcb>& pcb);
    ~openems_model_gen();
    
public:
    void add_net(std::uint32_t net_id);
    void add_footprint(const std::string& fp_ref);
    void add_excitation(const std::string& fp1, const std::string& fp1_pad_number, const std::string& fp1_layer_name,
                        const std::string& fp2, const std::string& fp2_pad_number, const std::string& fp2_layer_name, std::uint32_t dir);
    void set_nf2ff(const std::string& fp);
    void set_excitation_freq(float f0, float fc);
    void set_far_field_freq(float freq);
    void gen_model(const std::string& func_name);
    void gen_mesh(const std::string& func_name);
    void gen_antenna_simulation_scripts();
private:
    void _gen_mesh_z(FILE *fp);
    void _gen_mesh_xy(FILE *fp);
    void _add_dielectric(FILE *fp);
    void _add_metal(FILE *fp);
    void _add_segment(FILE *fp);
    void _add_via(FILE *fp);
    void _add_zone(FILE *fp);
    void _add_footprint(FILE *fp);
    void _add_gr(const pcb::gr& gr, pcb::point at, float angle, const std::string& name, FILE *fp);
    void _add_pad(const pcb::footprint& footprint, const pcb::pad& p, const std::string& name, FILE *fp);
    
    void _add_excitation(FILE *fp);
    void _add_nf2ff_box(FILE *fp);
    
private:
    std::shared_ptr<pcb> _pcb;
    std::set<std::uint32_t> _nets;
    std::set<std::string> _footprints;
    
    std::set<float> _mesh_x;
    std::set<float> _mesh_y;
    std::set<float> _mesh_z;
    
    float _f0;
    float _fc;
    float _far_field_freq;
    std::string _nf2ff_fp;
    
    std::string _exc_fp1;
    std::string _exc_fp1_pad_number;
    std::string _exc_fp1_layer_name;
    std::string _exc_fp2;
    std::string _exc_fp2_pad_number;
    std::string _exc_fp2_layer_name;
    std::uint32_t _exc_dir;
};

#endif