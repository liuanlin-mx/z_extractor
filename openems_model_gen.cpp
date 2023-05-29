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
#include "calc.h"
#include "openems_model_gen.h"
openems_model_gen::openems_model_gen(const std::shared_ptr<pcb>& pcb)
    : _pcb(pcb)
    , _mesh_x_min_gap(0.1)
    , _mesh_y_min_gap(0.1)
    , _mesh_z_min_gap(0.01)
    , _ignore_cu_thickness(true)
    , _lambda_mesh_ratio(20)
    , _bc(BC_PML)
    , _f0(1.8e9)
    , _fc(100)
    , _far_field_freq(2.4e9)
{
    _pcb->ignore_cu_thickness(_ignore_cu_thickness);
}

openems_model_gen::~openems_model_gen()
{
}

void openems_model_gen::add_net(std::uint32_t net_id)
{
    _nets.insert(net_id);
}

void openems_model_gen::add_footprint(const std::string & footprint)
{
    _footprints.insert(footprint);
}

void openems_model_gen::add_excitation(const std::string& fp1, const std::string& fp1_pad_number, const std::string& fp1_layer_name,
                        const std::string& fp2, const std::string& fp2_pad_number, const std::string& fp2_layer_name, std::uint32_t dir, float R)
{
    pcb::pad pad1;
    pcb::pad pad2;
    pcb::footprint footprint1;
    pcb::footprint footprint2;
    if (_pcb->get_footprint(fp1, footprint1) && _pcb->get_footprint(fp2, footprint2)
            && _pcb->get_pad(fp1, fp1_pad_number, pad1)
            && _pcb->get_pad(fp2, fp2_pad_number, pad2))
    {
        pcb::point p1 = pad1.at;
        pcb::point p2 = pad2.at;
        _pcb->get_rotation_pos(footprint1.at, footprint1.at_angle, p1);
        _pcb->get_rotation_pos(footprint2.at, footprint2.at_angle, p2);
        
    
        excitation ex;
        ex.R = R;
        ex.dir = dir;
        
        ex.start.z = _pcb->get_layer_z_axis(fp1_layer_name);
        ex.end.z = _pcb->get_layer_z_axis(fp2_layer_name);
        
        if (dir == excitation::DIR_X)
        {
            ex.start.x = p1.x;
            ex.start.y = p1.y - std::min(pad1.size_w, pad1.size_h) / 2;
            //ex.start.y = p1.y;
            ex.end.x = p2.x;
            ex.end.y = p2.y + std::min(pad2.size_w, pad2.size_h) / 2;
            //ex.end.y = p2.y;
        }
        else if (dir == excitation::DIR_Y)
        {
            ex.start.x = p1.x - std::min(pad1.size_w, pad1.size_h) / 2;
            ex.start.y = p1.y;
            
            ex.end.x = p2.x + std::min(pad2.size_w, pad2.size_h) / 2;
            ex.end.y = p2.y;
        }
        else if (dir == excitation::DIR_Z)
        {
            ex.start.x = p1.x - std::min(pad1.size_w, pad1.size_h) / 2;
            ex.start.y = p1.y - std::min(pad1.size_w, pad1.size_h) / 2;
            
            ex.end.x = p2.x + std::min(pad2.size_w, pad2.size_h) / 2;
            ex.end.y = p2.y + std::min(pad2.size_w, pad2.size_h) / 2;
        }
        _excitations.push_back(ex);
    }
    else
    {
        printf("add_excitation err\n");
    }
}

void openems_model_gen::add_excitation(pcb::point start, const std::string& start_layer, pcb::point end, const std::string& end_layer, std::uint32_t dir, float R)
{
    excitation ex;
    ex.R = R;
    ex.dir = dir;
    ex.start.x = start.x;
    ex.start.y = start.y;
    ex.start.z = _pcb->get_layer_z_axis(start_layer);
    ex.end.x = end.x;
    ex.end.y = end.y;
    ex.end.z = _pcb->get_layer_z_axis(end_layer);
    _excitations.push_back(ex);
}


void openems_model_gen::add_mesh_range(float start, float end, float gap, std::uint32_t dir, std::uint32_t prio)
{
    mesh::line_range range(start, end, gap, prio);
    if (dir == mesh::DIR_X)
    {
        _mesh.x_range.insert(range);
    }
    else if (dir == mesh::DIR_Y)
    {
        _mesh.y_range.insert(range);
    }
    else if (dir == mesh::DIR_Z)
    {
        _mesh.z_range.insert(range);
    }
}


void openems_model_gen::set_boundary_cond(std::uint32_t bc)
{
    _bc = bc;
}

void openems_model_gen::set_nf2ff_footprint(const std::string& fp)
{
    _nf2ff_fp = fp;
}

void openems_model_gen::set_excitation_freq(float f0, float fc)
{
    _f0 = f0;
    _fc = fc;
}

void openems_model_gen::set_far_field_freq(float freq)
{
    _far_field_freq = freq;
}

void openems_model_gen::set_mesh_min_gap(float x_min_gap, float y_min_gap, float z_min_gap)
{
    _mesh_x_min_gap = x_min_gap;
    _mesh_y_min_gap = y_min_gap;
    _mesh_z_min_gap = z_min_gap;
}

void openems_model_gen::gen_model(const std::string& func_name,
                    std::uint32_t segment_prio, std::uint32_t via_prio,
                    std::uint32_t zone_prio, std::uint32_t fp_prio)
{
    FILE *fp = fopen((func_name + ".m").c_str(), "wb");
    if (fp)
    {
        fprintf(fp, "function [CSX] = %s(CSX, max_freq)\n", func_name.c_str());
        fprintf(fp, "physical_constants;\n");
        fprintf(fp, "unit = 1e-3;\n");
        
        //_gen_mesh_z(fp);
        _add_dielectric(fp);
        _add_metal(fp);
        _add_segment(fp, segment_prio);
        _add_via(fp, via_prio);
        _add_zone(fp, zone_prio);
        _add_footprint(fp, fp_prio);
        //_gen_mesh_xy(fp);
        fprintf(fp, "end\n");
        fclose(fp);
    }
}


void openems_model_gen::gen_mesh(const std::string& func_name)
{
    FILE *fp = fopen((func_name + ".m").c_str(), "wb");
    if (fp)
    {
        fprintf(fp, "function [CSX, mesh] = %s(CSX, max_freq)\n", func_name.c_str());
        fprintf(fp, "physical_constants;\n");
        fprintf(fp, "unit = 1e-3;\n");
        
        _apply_mesh_line_range(_mesh);
        
        _gen_mesh_z(fp);
        _gen_mesh_xy(fp);
        fprintf(fp, "CSX = DefineRectGrid(CSX, unit, mesh);\n\n\n");
        fprintf(fp, "end\n");
        fclose(fp);
    }
}


void openems_model_gen::gen_antenna_simulation_scripts()
{
    const char *plot_s = R"(
%% postprocessing & do the plots
freq = linspace(max([1e6,f0 - fc]), f0 + fc, 501);
U = ReadUI({'port_ut1', 'et'}, [Sim_Path '/'], freq); % time domain/freq domain voltage
I = ReadUI('port_it1', [Sim_Path '/'], freq); % time domain/freq domain current (half time step is corrected)

% plot time domain voltage
figure
[ax,h1,h2] = plotyy( U.TD{1}.t/1e-9, U.TD{1}.val, U.TD{2}.t/1e-9, U.TD{2}.val );
set( h1, 'Linewidth', 2 );
set( h1, 'Color', [1 0 0] );
set( h2, 'Linewidth', 2 );
set( h2, 'Color', [0 0 0] );
grid on
title( 'time domain voltage' );
xlabel( 'time t / ns' );
ylabel( ax(1), 'voltage ut1 / V' );
ylabel( ax(2), 'voltage et / V' );
% now make the y-axis symmetric to y=0 (align zeros of y1 and y2)
y1 = ylim(ax(1));
y2 = ylim(ax(2));
ylim( ax(1), [-max(abs(y1)) max(abs(y1))] );
ylim( ax(2), [-max(abs(y2)) max(abs(y2))] );

% plot feed point impedance
figure
Zin = U.FD{1}.val ./ I.FD{1}.val;
plot( freq/1e6, real(Zin), 'k-', 'Linewidth', 2 );
hold on
grid on
plot( freq/1e6, imag(Zin), 'r--', 'Linewidth', 2 );
title( 'feed point impedance' );
xlabel( 'frequency f / MHz' );
ylabel( 'impedance Z_{in} / Ohm' );
legend( 'real', 'imag' );

% plot reflection coefficient S11
figure
uf_inc = 0.5*(U.FD{1}.val + I.FD{1}.val * 50);
if_inc = 0.5*(I.FD{1}.val - U.FD{1}.val / 50);
uf_ref = U.FD{1}.val - uf_inc;
if_ref = I.FD{1}.val - if_inc;
s11 = uf_ref ./ uf_inc;
plot( freq/1e6, 20*log10(abs(s11)), 'k-', 'Linewidth', 2 );
grid on
title( 'reflection coefficient S_{11}' );
xlabel( 'frequency f / MHz' );
ylabel( 'reflection coefficient |S_{11}|' );

P_in = 0.5*U.FD{1}.val .* conj( I.FD{1}.val );
)";
    const char *plot_nf = R"(
%% NFFF contour plots %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
#f_res_ind = find(s11==min(s11));
#f_res = freq(f_res_ind);
f_res = far_field_freq;
[~, f_res_ind] = min(abs(freq - f_res));
% calculate the far field at phi=0 degrees and at phi=90 degrees
thetaRange = (0:2:359) - 180;
phiRange = [0 90];
disp( 'calculating far field at phi=[0 90] deg...' );
nf2ff = CalcNF2FF(nf2ff, Sim_Path, f_res, thetaRange*pi/180, phiRange*pi/180, 'Center', (nf2ff_start + nf2ff_stop) * 0.5 * unit);

Dlog=10*log10(nf2ff.Dmax);

#G_a = 4*pi*A/(c0/f0)^2;
#e_a = nf2ff.Dmax/G_a;

% display power and directivity
disp( ['radiated power: Prad = ' num2str(nf2ff.Prad) ' Watt']);
disp( ['directivity: Dmax = ' num2str(Dlog) ' dBi'] );
disp( ['efficiency: nu_rad = ' num2str(100*nf2ff.Prad./real(P_in(f_res_ind))) ' %']);
#disp( ['aperture efficiency: e_a = ' num2str(e_a*100) '%'] );

% display phi
figure
plotFFdB(nf2ff,'xaxis','theta','param',[1 2]);
drawnow

% polar plot
figure
polarFF(nf2ff,'xaxis','theta','param',[1 2],'logscale',[-40 20], 'xtics', 12);
drawnow

%   polar( nf2ff.theta, nf2ff.E_norm{1}(:,1) )


%% calculate 3D pattern
phiRange = 0:2:360;
thetaRange = 0:2:180;
disp( 'calculating 3D far field...' );
nf2ff = CalcNF2FF(nf2ff, Sim_Path, f_res, thetaRange*pi/180, phiRange*pi/180, 'Verbose',2,'Outfile','nf2ff_3D.h5', 'Center', (nf2ff_start + nf2ff_stop) * 0.5 * unit);
figure
#plotFF3D(nf2ff);
plotFF3D(nf2ff, 'logscale', -20)

E_far_normalized = nf2ff.E_norm{1} / max(nf2ff.E_norm{1}(:)) * nf2ff.Dmax; DumpFF2VTK([Sim_Path '/3D_Pattern.vtk'], E_far_normalized, thetaRange, phiRange, 'scale', 1e-3);


)";
    FILE *fp = fopen("antenna_simulation_scripts.m", "wb");
    if (fp)
    {
        fprintf(fp, "close all; clear; clc;\n");
        fprintf(fp, "physical_constants;\n");
        fprintf(fp, "unit = 1e-3;\n");
        fprintf(fp, "max_timesteps = 1e9; min_decrement = 1e-5;\n");
        fprintf(fp, "FDTD = InitFDTD('NrTS', max_timesteps, 'EndCriteria', min_decrement);\n");
        fprintf(fp, "f0 = %e; fc = %e;\n", _f0, _fc);
        //fprintf(fp, "lambda = c0 / (f0 + fc) / unit;\n");
        fprintf(fp, "FDTD = SetGaussExcite(FDTD, f0, fc);\n");
        if (_bc == BC_PML)
        {
            fprintf(fp, "BC = {'PML_8' 'PML_8' 'PML_8' 'PML_8' 'PML_8' 'PML_8'};\n");
        }
        else
        {
            fprintf(fp, "BC = {'MUR' 'MUR' 'MUR' 'MUR' 'MUR' 'MUR'};\n");
        }
        fprintf(fp, "FDTD = SetBoundaryCond(FDTD, BC);\n");
        fprintf(fp, "\n");
        fprintf(fp, "CSX = InitCSX();\n");
        fprintf(fp, "CSX = load_pcb_model(CSX, f0 + fc);\n");
        fprintf(fp, "[CSX, mesh] = load_pcb_mesh(CSX, f0 + fc);\n");
        fprintf(fp, "\n");
        
        _add_excitation(fp, 3);
        _add_nf2ff_box(fp);
        
        fprintf(fp, "Sim_Path = 'ant_sim'; Sim_CSX = 'ant.xml';\n");
        fprintf(fp, "rmdir(Sim_Path, 's');\n");
        fprintf(fp, "mkdir(Sim_Path);\n");
        fprintf(fp, "WriteOpenEMS( [Sim_Path '/' Sim_CSX], FDTD, CSX);\n");
        fprintf(fp, "CSXGeomPlot( [Sim_Path '/' Sim_CSX], ['--export-STL=' Sim_Path]);\n");
        fprintf(fp, "RunOpenEMS(Sim_Path, Sim_CSX, '--debug-PEC');\n");
        
        

        fprintf(fp, "%s\n", plot_s);
        fprintf(fp, "\n");
        fprintf(fp, "\n");
        fprintf(fp, "%s\n", plot_nf);
        
        fclose(fp);
    
    }
    gen_model("load_pcb_model", 1, 1, 0, 2);
    gen_mesh("load_pcb_mesh");
}

void openems_model_gen::_gen_mesh_z(FILE *fp)
{
    std::string str;
    
    std::set<mesh::line> mesh_z = _mesh.z;
    _mesh.z.clear();
    
    float min_z = _pcb->get_cu_min_thickness();
    if (_ignore_cu_thickness)
    {
        min_z = _pcb->get_min_thickness(pcb::layer::DIELECTRIC);
    }
    
    std::vector<pcb::layer> layers = _pcb->get_layers();
    std::string last_layer;
    for (const auto& layer: layers)
    {
        if (layer.type == pcb::layer::TOP_SOLDER_MASK || layer.type == pcb::layer::BOTTOM_SOLDER_MASK)
        {
            continue;
        }
        if (_ignore_cu_thickness && layer.type == pcb::layer::COPPER)
        {
            continue;
        }
        float z = _pcb->get_layer_z_axis(layer.name);
        _mesh.z.insert(mesh::line(z, 0));
        last_layer = layer.name;
    }
    _mesh.z.insert(mesh::line(_pcb->get_layer_z_axis(last_layer) + _pcb->get_layer_thickness(last_layer), 0));
    
    _clean_mesh_line(_mesh.z, _mesh_z_min_gap);
    
    fprintf(fp, "mesh.z = [");
    for (auto z: _mesh.z)
    {
        fprintf(fp, "%f ", z.v);
    }
    fprintf(fp, "];\n");
    
    fprintf(fp, "max_res = %f;\n", min_z);
    fprintf(fp, "mesh.z = SmoothMeshLines(mesh.z, max_res, 1.5);\n");
    
    
    
    float margin = _pcb->get_board_thickness() * 20;
    if (mesh_z.size() > 1)
    {
        fprintf(fp, "mesh.z = unique([mesh.z, %f, %f]);\n", std::min(-margin, mesh_z.begin()->v), std::max(margin, mesh_z.rbegin()->v));
    }
    else
    {
        fprintf(fp, "margin = %g;\n", margin);
        fprintf(fp, "mesh.z = unique([mesh.z, -margin, margin]);\n");
    }
    
    fprintf(fp, "max_res = c0 / (max_freq) / unit / %f;\n", _lambda_mesh_ratio);
    fprintf(fp, "mesh.z = SmoothMeshLines(mesh.z, max_res, 1.5);\n");
    
    fprintf(fp, "\n\n");
}


void openems_model_gen::_gen_mesh_xy(FILE *fp)
{
    float x1 = _pcb->get_edge_left();
    float x2 = _pcb->get_edge_right();
    float y1 = _pcb->get_edge_top();
    float y2 = _pcb->get_edge_bottom();
    
    float lambda = 299792458. / (_f0 + _fc) * 1e3;
    
    float ratio = (_bc == BC_PML)? _lambda_mesh_ratio / 10: _lambda_mesh_ratio;
    
    float left = x1 - lambda / ratio;
    float right = x2 + lambda / ratio;
    float top = y1 - lambda / ratio;
    float bottom = y2 + lambda / ratio;
    
    if (!_mesh.x.empty() && _mesh.x.begin()->v > left)
    {
        _mesh.x.insert(mesh::line(left, _mesh.x.begin()->prio));
    }
    
    if (!_mesh.x.empty() && _mesh.x.rbegin()->v < right)
    {
        _mesh.x.insert(mesh::line(right, _mesh.x.rbegin()->prio));
    }
    
    if (!_mesh.y.empty() && _mesh.y.begin()->v > top)
    {
        _mesh.y.insert(mesh::line(top, _mesh.y.begin()->prio));
    }
    
    if (!_mesh.y.empty() && _mesh.y.rbegin()->v < bottom)
    {
        _mesh.y.insert(mesh::line(bottom, _mesh.y.rbegin()->prio));
    }
    
    
    
    
    _clean_mesh_line(_mesh.x, _mesh_x_min_gap);
    _clean_mesh_line(_mesh.y, _mesh_y_min_gap);
    
    fprintf(fp, "mesh.x = [");
    for (auto x: _mesh.x)
    {
        fprintf(fp, "%f ", x.v);
    }
    fprintf(fp, "];\n");
    
    fprintf(fp, "mesh.y = [");
    for (auto y: _mesh.y)
    {
        fprintf(fp, "%f ", y.v);
    }
    fprintf(fp, "];\n");
    
    
    fprintf(fp, "max_res = c0 / (max_freq) / unit / %f;\n", _lambda_mesh_ratio);
    fprintf(fp, "mesh.x = SmoothMeshLines(mesh.x, max_res, 1.5);\n");
    fprintf(fp, "mesh.y = SmoothMeshLines(mesh.y, max_res, 1.5);\n");
}


void openems_model_gen::_add_dielectric(FILE *fp)
{
    float x1 = _pcb->get_edge_left();
    float x2 = _pcb->get_edge_right();
    float y1 = _pcb->get_edge_top();
    float y2 = _pcb->get_edge_bottom();
    
    std::vector<pcb::layer> layers = _pcb->get_layers();
    for (const auto& layer: layers)
    {
        if (layer.type == pcb::layer::TOP_SOLDER_MASK
            || layer.type == pcb::layer::BOTTOM_SOLDER_MASK
            || layer.type == pcb::layer::COPPER)
        {
            continue;
        }
        
        float z1 = _pcb->get_layer_z_axis(layer.name);
        float z2 = z1 + _pcb->get_layer_thickness(layer.name);
        
        fprintf(fp, "start = [%f %f %f];\n", x1, y1, z1);
        fprintf(fp, "stop = [%f %f %f];\n", x2, y2, z2);
        fprintf(fp, "CSX = AddMaterial(CSX, '%s');\n", layer.name.c_str());
        fprintf(fp, "CSX = SetMaterialProperty( CSX, '%s', 'Epsilon', %f);\n", layer.name.c_str(), _pcb->get_layer_epsilon_r(layer.name));
        fprintf(fp, "CSX = AddBox(CSX, '%s', 1, start, stop);\n", layer.name.c_str());
    }

    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_metal(FILE *fp)
{
    for (auto net_id: _nets)
    {
        std::string net_name = _pcb->get_net_name(net_id);
        fprintf(fp, "CSX = AddMetal(CSX, '%s');\n", net_name.c_str());
    }
    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_segment(FILE *fp, std::uint32_t mesh_prio)
{
    for (auto net_id: _nets)
    {
        std::list<pcb::segment> segments = _pcb->get_segments(net_id);
        for (const auto& s: segments)
        {
            //const std::string& layer = s.layer_name;
            //float z1 = _pcb->get_layer_z_axis(layer);
            //float z2 = z1 + _pcb->get_layer_thickness(layer);
            //fprintf(fp, "start = [%f %f %f];\n", x1, y1, z1);
            //fprintf(fp, "stop = [%f %f %f];\n", x2, y2, z2);
            //fprintf(fp, "CSX = AddBox(CSX, 'copper', 2, start, stop);\n");
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_via(FILE *fp, std::uint32_t mesh_prio)
{
    for (auto net_id: _nets)
    {
        std::string net_name = _pcb->get_net_name(net_id);
        std::list<pcb::via> vias = _pcb->get_vias(net_id);
        for (const auto& v: vias)
        {
            std::vector<std::string> layers = _pcb->get_via_layers(v);
    
            float min_z = 10000;
            float max_z = -10000;
            for (const auto& layer: layers)
            {
                pcb::point c(v.at);
                float z1 = _pcb->get_layer_z_axis(layer);
                float thickness = _pcb->get_layer_thickness(layer);
                float z2 = z1 + thickness;
                
                if (z1 < min_z)
                {
                    min_z = z1;
                }
                
                if (z2 < min_z)
                {
                    min_z = z2;
                }
                
                if (z1 > max_z)
                {
                    max_z = z1;
                }
                
                if (z2 > max_z)
                {
                    max_z = z2;
                }
                
        #if 0
                float radius = v.size / 2;
                
                fprintf(fp, "CSX = AddCylinder(CSX, '%s', 2, [%f %f %f], [%f %f %f], %f);\n",
                        net_name.c_str(),
                        c.x, c.y, z1,
                        c.x, c.y, _ignore_cu_thickness? z2 + 0.001: z2,
                        radius);
        #endif
                        
            }
            if (min_z < 10000 && max_z > -10000)
            {
                pcb::point c(v.at);
                
                float radius = v.drill / 2;
                fprintf(fp, "CSX = AddCylinder(CSX, '%s', 2, [%f %f %f], [%f %f %f], %f);\n",
                            net_name.c_str(),
                            c.x, c.y, min_z,
                            c.x, c.y, max_z,
                            radius);
                _mesh.x.insert(mesh::line(c.x, mesh_prio));
                //_mesh_x.insert(c.x + radius);
                //_mesh_x.insert(c.x - radius);
                _mesh.y.insert(mesh::line(c.y, mesh_prio));
                //_mesh_y.insert(c.y + radius);
                //_mesh_y.insert(c.y - radius);
            }
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_zone(FILE *fp, std::uint32_t mesh_prio)
{
    for (auto net_id: _nets)
    {
        std::list<pcb::zone> zones = _pcb->get_zones(net_id);
        for (const auto& z: zones)
        {
            const std::string& layer = z.layer_name;
            float z1 = _pcb->get_layer_z_axis(layer);
            float thickness = _pcb->get_layer_thickness(layer);
            
            std::uint32_t idx = 1;
            for (const auto& p: z.pts)
            {
                fprintf(fp, "p(1, %d) = %f; p(2, %d) = %f;\n", idx, p.x, idx, p.y);
                idx++;
                _mesh.x.insert(mesh::line(p.x, mesh_prio));
                _mesh.y.insert(mesh::line(p.y, mesh_prio));
            }
            
            fprintf(fp, "CSX = AddLinPoly(CSX, '%s', 2, 2, %f, p, %f, 'CoordSystem', 0);\n", _pcb->get_net_name(z.net).c_str(), z1, thickness);
            fprintf(fp, "clear p;\n");
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}

void openems_model_gen::_add_footprint(FILE *fp, std::uint32_t mesh_prio)
{
    const std::vector<pcb::footprint>& footprints = _pcb->get_footprints();
    for (const auto& footprint: footprints)
    {
        if (_footprints.count(footprint.reference))
        {
            fprintf(fp, "CSX = AddMetal(CSX, '%s');\n", footprint.reference.c_str());
            for (const auto& gr: footprint.grs)
            {
                if (_pcb->is_cu_layer(gr.layer_name))
                {
                    _add_gr(gr, footprint.at, footprint.at_angle, footprint.reference, fp);
                }
            }
            
            for (const auto& pad: footprint.pads)
            {
                _add_pad(footprint, pad, footprint.reference, fp);
            }
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_gr(const pcb::gr& gr, pcb::point at, float angle, const std::string& name, FILE *fp, std::uint32_t mesh_prio)
{
    const std::string& layer = gr.layer_name;
    float z1 = _pcb->get_layer_z_axis(layer);
    float thickness = _pcb->get_layer_thickness(layer);
    float z2 = z1 + thickness;
    
    if (gr.gr_type == pcb::gr::GR_POLY)
    {
        std::uint32_t idx = 1;
        for (auto xy : gr.pts)
        {
            _pcb->get_rotation_pos(at, angle, xy);
            fprintf(fp, "p(1, %d) = %f; p(2, %d) = %f;\n", idx, xy.x, idx, xy.y);
            idx++;
            _mesh.x.insert(mesh::line(xy.x, mesh_prio));
            _mesh.y.insert(mesh::line(xy.y, mesh_prio));
        }
        
        fprintf(fp, "CSX = AddLinPoly(CSX, '%s', 2, 2, %f, p, %f, 'CoordSystem', 0);\n", name.c_str(), z1, thickness);
        fprintf(fp, "clear p;\n");
    }
    else if (gr.gr_type == pcb::gr::GR_RECT)
    {
        pcb::point p1(gr.start.x, gr.start.y);
        pcb::point p2(gr.end.x, gr.start.y);
        pcb::point p3(gr.end.x, gr.end.y);
        pcb::point p4(gr.start.x, gr.end.y);
        
        _pcb->get_rotation_pos(at, angle, p1);
        _pcb->get_rotation_pos(at, angle, p2);
        _pcb->get_rotation_pos(at, angle, p3);
        _pcb->get_rotation_pos(at, angle, p4);
        
        if (gr.fill_type == pcb::gr::FILL_SOLID)
        {
            fprintf(fp, "p(1, 1) = %f; p(2, 1) = %f;\n", p1.x, p1.y);
            fprintf(fp, "p(1, 2) = %f; p(2, 2) = %f;\n", p2.x, p2.y);
            fprintf(fp, "p(1, 3) = %f; p(2, 3) = %f;\n", p3.x, p3.y);
            fprintf(fp, "p(1, 4) = %f; p(2, 4) = %f;\n", p4.x, p4.y);
        
            //fprintf(fp, "CSX = AddBox(CSX, '%s', 2, [%f %f %f], [%f %f %f]);\n",
            //    name.c_str(), p1.x, p1.y, z1, p3.x, p3.y, z2);
            fprintf(fp, "CSX = AddLinPoly(CSX, '%s', 2, 2, %f, p, %f, 'CoordSystem', 0);\n", name.c_str(), z1, thickness);
            fprintf(fp, "clear p;\n");
            
            _mesh.x.insert(mesh::line(p1.x, mesh_prio)); _mesh.y.insert(mesh::line(p1.y, mesh_prio));
            _mesh.x.insert(mesh::line(p2.x, mesh_prio)); _mesh.y.insert(mesh::line(p2.y, mesh_prio));
            _mesh.x.insert(mesh::line(p3.x, mesh_prio)); _mesh.y.insert(mesh::line(p3.y, mesh_prio));
            _mesh.x.insert(mesh::line(p4.x, mesh_prio)); _mesh.y.insert(mesh::line(p4.y, mesh_prio));
        }
    }
    else if (gr.gr_type == pcb::gr::GR_LINE)
    {
        pcb::point start = gr.start;
        pcb::point end = gr.end;
        _pcb->get_rotation_pos(at, angle, start);
        _pcb->get_rotation_pos(at, angle, end);
        
        //cv::Point p1(_cvt_img_x(start.x, pix_unit), _cvt_img_y(start.y, pix_unit));
        //cv::Point p2(_cvt_img_x(end.x, pix_unit), _cvt_img_y(end.y, pix_unit));
        //float thickness = _cvt_img_len(gr.stroke_width, pix_unit);
        //cv::line(img, p1, p2, cv::Scalar(b, g, r), thickness);
    }
    else if (gr.gr_type == pcb::gr::GR_CIRCLE)
    {
        pcb::point start = gr.start;
        pcb::point end = gr.end;
        _pcb->get_rotation_pos(at, angle, start);
        _pcb->get_rotation_pos(at, angle, end);
        float radius = calc_dist(start.x, start.y, end.x, end.y);
        if (gr.fill_type == pcb::gr::FILL_SOLID)
        {
            fprintf(fp, "CSX = AddCylinder(CSX, '%s', 2, [%f %f %f], [%f %f %f], %f);\n",
                        name.c_str(),
                        start.x, start.y, z1,
                        start.x, start.y, z2,
                        radius);
                        
            _mesh.x.insert(mesh::line(start.x, mesh_prio));
            _mesh.y.insert(mesh::line(start.y, mesh_prio));
        }
        else
        {
            //float thickness = _cvt_img_len(gr.stroke_width, pix_unit);
        }
    }
    fprintf(fp, "\n");
}


void openems_model_gen::_add_pad(const pcb::footprint& footprint, const pcb::pad& p, const std::string& name, FILE *fp, std::uint32_t mesh_prio)
{
    std::vector<std::string> layers = _pcb->get_pad_layers(p);
    
    if (p.type == pcb::pad::TYPE_THRU_HOLE)
    {
        float min_z = 10000;
        float max_z = -10000;
        
        for (const auto& layer: layers)
        {
            float z1 = _pcb->get_layer_z_axis(layer);
            float thickness = _pcb->get_layer_thickness(layer);
            float z2 = z1 + thickness;
            if (z1 < min_z)
            {
                min_z = z1;
            }
            
            if (z2 < min_z)
            {
                min_z = z2;
            }
            
            if (z1 > max_z)
            {
                max_z = z1;
            }
            
            if (z2 > max_z)
            {
                max_z = z2;
            }
        }
        
        if (min_z < 10000 && max_z > -10000)
        {
            pcb::point c(p.at);
            
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, c);
            float radius = p.drill / 2;
            fprintf(fp, "CSX = AddCylinder(CSX, '%s', 3, [%f %f %f], [%f %f %f], %f);\n",
                        name.c_str(),
                        c.x, c.y, min_z,
                        c.x, c.y, max_z,
                        radius);
            //_mesh_x.insert(c.x);
            _mesh.x.insert(mesh::line(c.x + radius, mesh_prio));
            _mesh.x.insert(mesh::line(c.x - radius, mesh_prio));
            //_mesh_y.insert(c.y);
            _mesh.y.insert(mesh::line(c.y + radius, mesh_prio));
            _mesh.y.insert(mesh::line(c.y - radius, mesh_prio));
        }
    }
    
    for (const auto& layer: layers)
    {
        float z1 = _pcb->get_layer_z_axis(layer);
        float thickness = _pcb->get_layer_thickness(layer);
        float z2 = z1 + thickness;
        
        if (p.shape == pcb::pad::SHAPE_RECT || p.shape == pcb::pad::SHAPE_ROUNDRECT)
        {
            pcb::point p1(p.at.x - p.size_w / 2, p.at.y + p.size_h / 2);
            pcb::point p2(p.at.x + p.size_w / 2, p.at.y + p.size_h / 2);
            pcb::point p3(p.at.x + p.size_w / 2, p.at.y - p.size_h / 2);
            pcb::point p4(p.at.x - p.size_w / 2, p.at.y - p.size_h / 2);
            
            
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, p1);
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, p2);
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, p3);
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, p4);
            
            
            fprintf(fp, "p(1, 1) = %f; p(2, 1) = %f;\n", p1.x, p1.y);
            fprintf(fp, "p(1, 2) = %f; p(2, 2) = %f;\n", p2.x, p2.y);
            fprintf(fp, "p(1, 3) = %f; p(2, 3) = %f;\n", p3.x, p3.y);
            fprintf(fp, "p(1, 4) = %f; p(2, 4) = %f;\n", p4.x, p4.y);
        
            fprintf(fp, "CSX = AddLinPoly(CSX, '%s', 3, 2, %f, p, %f, 'CoordSystem', 0);\n", name.c_str(), z1, thickness);
            fprintf(fp, "clear p;\n");
            
            _mesh.x.insert(mesh::line(p1.x, mesh_prio)); _mesh.y.insert(mesh::line(p1.y, mesh_prio));
            _mesh.x.insert(mesh::line(p2.x, mesh_prio)); _mesh.y.insert(mesh::line(p2.y, mesh_prio));
            _mesh.x.insert(mesh::line(p3.x, mesh_prio)); _mesh.y.insert(mesh::line(p3.y, mesh_prio));
            _mesh.x.insert(mesh::line(p4.x, mesh_prio)); _mesh.y.insert(mesh::line(p4.y, mesh_prio));
        }
        else if (p.shape == pcb::pad::SHAPE_CIRCLE)
        {
            pcb::point c(p.at);
            
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, c);
            float radius = p.size_w / 2;
            
            fprintf(fp, "CSX = AddCylinder(CSX, '%s', 3, [%f %f %f], [%f %f %f], %f);\n",
                        name.c_str(),
                        c.x, c.y, z1,
                        c.x, c.y, _ignore_cu_thickness? z2 + 0.001: z2,
                        radius);
                        
            _mesh.x.insert(mesh::line(c.x, mesh_prio));
            _mesh.y.insert(mesh::line(c.y, mesh_prio));
        }
        else if (p.shape == pcb::pad::SHAPE_OVAL)
        {
            
        }
    }
    
    fprintf(fp, "\n\n");
    
}


void openems_model_gen::_add_excitation(FILE *fp, std::uint32_t mesh_prio)
{
    std::uint32_t portnr = 1;
    for (const auto& ex: _excitations)
    {
        fprintf(fp, "[CSX] = AddLumpedPort(CSX, 1, %u, %f, [%f %f %f], [%f %f %f], [%d %d %d], true);\n",
                portnr,
                ex.R,
                ex.start.x, ex.start.y, ex.start.z,
                ex.end.x, ex.end.y, ex.end.z,
                (ex.dir == excitation::DIR_X)? 1: 0,
                (ex.dir == excitation::DIR_Y)? 1: 0,
                (ex.dir == excitation::DIR_Z)? 1: 0);
        portnr++;
        _mesh.x.insert(mesh::line(ex.start.x, mesh_prio));
        _mesh.x.insert(mesh::line(ex.end.x, mesh_prio));
        _mesh.y.insert(mesh::line(ex.start.y, mesh_prio));
        _mesh.y.insert(mesh::line(ex.end.y, mesh_prio));
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
    
}

void openems_model_gen::_add_nf2ff_box(FILE *fp, std::uint32_t mesh_prio)
{
    float nf2ff_cx = 0;
    float nf2ff_cy = 0;
    float nf2ff_cz = 0;
    
    if (_nf2ff_fp.empty())
    {
        if (!_excitations.empty())
        {
            auto ex = _excitations.front();
            nf2ff_cx = (ex.start.x + ex.end.x) / 2;
            nf2ff_cy = (ex.start.y + ex.end.y) / 2;
            nf2ff_cz = (ex.start.z + ex.end.z) / 2;
        }
    }
    else
    {
        pcb::footprint fp;
        if (_pcb->get_footprint(_nf2ff_fp, fp))
        {
            nf2ff_cz = _pcb->get_layer_z_axis(fp.layer);
            nf2ff_cx = fp.at.x;
            nf2ff_cy = fp.at.y;
        }
    }
    
    float ratio = (_bc == BC_PML)? _lambda_mesh_ratio / 10: _lambda_mesh_ratio;
    //float lambda = 299792458. / (_f0 + _fc) * 1e3;
    float lambda = 299792458. / (_far_field_freq) * 1e3;
    float x_margin = std::max(fabs(nf2ff_cx - _pcb->get_edge_left()) + lambda / ratio, fabs(nf2ff_cx - _pcb->get_edge_right())  + lambda / ratio);
    x_margin = std::max(x_margin, lambda / 2);
    float y_margin = std::max(fabs(nf2ff_cy - _pcb->get_edge_top()) + lambda / ratio, fabs(nf2ff_cy - _pcb->get_edge_bottom()) + lambda / ratio);
    y_margin = std::max(y_margin, lambda / 2);
    float z_margin = std::max(_pcb->get_board_thickness() * ratio, lambda / 2);
    
    _mesh.x.insert(mesh::line(nf2ff_cx - x_margin - lambda / ratio, mesh_prio));
    _mesh.x.insert(mesh::line(nf2ff_cx + x_margin + lambda / ratio, mesh_prio));
    _mesh.y.insert(mesh::line(nf2ff_cy - y_margin - lambda / ratio, mesh_prio));
    _mesh.y.insert(mesh::line(nf2ff_cy + y_margin + lambda / ratio, mesh_prio));
    _mesh.z.insert(mesh::line(nf2ff_cz - z_margin - lambda / ratio, mesh_prio));
    _mesh.z.insert(mesh::line(nf2ff_cz + z_margin + lambda / ratio, mesh_prio));
    
    fprintf(fp, "far_field_freq = %g;\n", _far_field_freq);
    fprintf(fp, "nf2ff_cx = %e; nf2ff_cy = %e; nf2ff_cz = %e;\n", nf2ff_cx, nf2ff_cy, nf2ff_cz);
    fprintf(fp, "x_margin = %e; y_margin = %e; z_margin = %e;\n", x_margin, y_margin, z_margin);
    fprintf(fp, "nf2ff_start = [nf2ff_cx - x_margin, nf2ff_cy - y_margin, nf2ff_cz - z_margin];\n");
    fprintf(fp, "nf2ff_stop = [nf2ff_cx + x_margin, nf2ff_cy + y_margin, nf2ff_cz + z_margin];\n");
    fprintf(fp, "[CSX nf2ff] = CreateNF2FFBox(CSX, 'nf2ff', nf2ff_start, nf2ff_stop);\n");


    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}



void openems_model_gen::_apply_mesh_line_range(mesh& mesh)
{
    _apply_mesh_line_range(mesh.x, mesh.x_range);
    _apply_mesh_line_range(mesh.y, mesh.y_range);
    _apply_mesh_line_range(mesh.z, mesh.z_range);
}

void openems_model_gen::_apply_mesh_line_range(std::set<mesh::line>& mesh_line, const std::multiset<mesh::line_range>& mesh_line_range)
{
    for (auto it = mesh_line_range.begin(); it != mesh_line_range.end(); it++)
    {
        float start = it->start;
        float end = it->end;
        float gap = it->gap;
        
        for (float v = start; v < end; v += gap)
        {
            bool continue_ = false;
            for (auto it2 = mesh_line_range.begin(); it2 != it; it2++)
            {
                if (v >= it2->start && v < it2->end)
                {
                    continue_ = true;
                    break;
                }
            }
            if (continue_)
            {
                continue;
            }
            mesh_line.insert(mesh::line(v, it->prio));
        }
    }
}

void openems_model_gen::_clean_mesh_line(std::set<mesh::line>& mesh_line, float min_gap)
{
    if (mesh_line.size() < 2)
    {
        return;
    }
    
    bool brk = true;
    do
    {
        brk = true;
        auto it1 = mesh_line.begin();
        auto it2 = it1;
        it2++;
        for (; it2 != mesh_line.end(); )
        {
            if (fabs(it2->v - it1->v) < min_gap)
            {
                brk = false;
                if (it1->prio == it2->prio)
                {
                    mesh::line tmp((it1->v + it2->v) * 0.5, it1->prio);
                    
                    it1 = mesh_line.erase(it1);
                    it1 = mesh_line.erase(it1);
                    auto r = mesh_line.insert(tmp);
                    if (r.second)
                    {
                        it1 = r.first;
                    }
                }
                else if (it1->prio > it2->prio)
                {
                    it1 = mesh_line.erase(it2);
                }
                else
                {
                    it1 = mesh_line.erase(it1);
                }
                it2 = it1;
                it2++;
                continue;
            }
            it1++;
            it2++;
        }
    } while (brk == false);
}