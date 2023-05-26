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
    , _ignore_cu_thickness(true)
    , _f0(0e9)
    , _fc(3.5e9)
    , _far_field_freq(2.4e9)
    , _exc_dir(0)
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
                        const std::string& fp2, const std::string& fp2_pad_number, const std::string& fp2_layer_name, std::uint32_t dir)
{
    _exc_fp1 = fp1;
    _exc_fp1_pad_number = fp1_pad_number;
    _exc_fp1_layer_name = fp1_layer_name;
    _exc_fp2 = fp2;
    _exc_fp2_pad_number = fp2_pad_number;
    _exc_fp2_layer_name = fp2_layer_name;
    _exc_dir = dir;
    
}

void openems_model_gen::set_nf2ff(const std::string& fp)
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

void openems_model_gen::gen_model(const std::string& func_name)
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
        _add_segment(fp);
        _add_via(fp);
        _add_zone(fp);
        _add_footprint(fp);
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
        fprintf(fp, "max_timesteps = 1e9; min_decrement = 1e-1;\n");
        fprintf(fp, "FDTD = InitFDTD('NrTS', max_timesteps, 'EndCriteria', min_decrement);\n");
        fprintf(fp, "f0 = %e; fc = %e;\n", _f0, _fc);
        //fprintf(fp, "lambda = c0 / (f0 + fc) / unit;\n");
        fprintf(fp, "FDTD = SetGaussExcite(FDTD, f0, fc);\n");
        fprintf(fp, "BC = {'MUR' 'MUR' 'MUR' 'MUR' 'MUR' 'MUR'};\n");
        fprintf(fp, "FDTD = SetBoundaryCond(FDTD, BC);\n");
        fprintf(fp, "\n");
        fprintf(fp, "CSX = InitCSX();\n");
        fprintf(fp, "CSX = load_pcb_model(CSX, f0 + fc);\n");
        fprintf(fp, "[CSX, mesh] = load_pcb_mesh(CSX, f0 + fc);\n");
        fprintf(fp, "\n");
        
        _add_excitation(fp);
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
    gen_model("load_pcb_model");
    gen_mesh("load_pcb_mesh");
}

void openems_model_gen::_gen_mesh_z(FILE *fp)
{
    std::string str;
    
    std::set<float> mesh_z = _mesh_z;
    _mesh_z.clear();
    
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
        _mesh_z.insert(z);
        last_layer = layer.name;
    }
    _mesh_z.insert(_pcb->get_layer_z_axis(last_layer) + _pcb->get_layer_thickness(last_layer));
    
    fprintf(fp, "mesh.z = [");
    for (auto z: _mesh_z)
    {
        fprintf(fp, "%f ", z);
    }
    fprintf(fp, "];\n");
    
    fprintf(fp, "max_res = %f;\n", min_z);
    fprintf(fp, "mesh.z = SmoothMeshLines(mesh.z, max_res, 1.3);\n");
    
    
    
    float margin = _pcb->get_board_thickness() * 20;
    if (mesh_z.size() > 1)
    {
        fprintf(fp, "mesh.z = unique([mesh.z, %f, %f]);\n", std::min(-margin, *mesh_z.begin()), std::max(margin, *mesh_z.rbegin()));
    }
    else
    {
        fprintf(fp, "margin = %g;\n", margin);
        fprintf(fp, "mesh.z = unique([mesh.z, -margin, margin]);\n");
    }
    
    fprintf(fp, "max_res = c0 / (max_freq) / unit / 20;\n");
    fprintf(fp, "mesh.z = SmoothMeshLines(mesh.z, max_res, 1.3);\n");
    
    fprintf(fp, "\n\n");
}


void openems_model_gen::_gen_mesh_xy(FILE *fp)
{
    float x1 = _pcb->get_edge_left();
    float x2 = _pcb->get_edge_right();
    float y1 = _pcb->get_edge_top();
    float y2 = _pcb->get_edge_bottom();
    
    
    _mesh_x.insert(x1);
    _mesh_x.insert(x2);
    
    _mesh_y.insert(y1);
    _mesh_y.insert(y2);
    
    fprintf(fp, "mesh.x = [");
    for (auto x: _mesh_x)
    {
        fprintf(fp, "%f ", x);
    }
    fprintf(fp, "];\n");
    
    fprintf(fp, "mesh.y = [");
    for (auto y: _mesh_y)
    {
        fprintf(fp, "%f ", y);
    }
    fprintf(fp, "];\n");
    
    //fprintf(fp, "max_res = c0 / (max_freq) / unit / 30;\n");
    //fprintf(fp, "max_res = %f;\n", 1);
    //fprintf(fp, "mesh.x = SmoothMeshLines(mesh.x, max_res, 1.4);\n");
    //fprintf(fp, "mesh.y = SmoothMeshLines(mesh.y, max_res, 1.4);\n");
    
    float lambda = 299792458. / (_f0 + _fc) * 1e3;
    
    fprintf(fp, "max_res = c0 / (max_freq) / unit / 20;\n");
    fprintf(fp, "mesh.x = unique([mesh.x %f %f]);\n", x1 - lambda / 10, x2 + lambda / 10);
    fprintf(fp, "mesh.y = unique([mesh.y %f %f]);\n", y1 - lambda / 10, y2 + lambda / 10);
    
    fprintf(fp, "mesh.x = SmoothMeshLines(mesh.x, max_res, 1.3);\n");
    fprintf(fp, "mesh.y = SmoothMeshLines(mesh.y, max_res, 1.3);\n");
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


void openems_model_gen::_add_segment(FILE *fp)
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


void openems_model_gen::_add_via(FILE *fp)
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
                
                float radius = v.size / 2;
                
                fprintf(fp, "CSX = AddCylinder(CSX, '%s', 2, [%f %f %f], [%f %f %f], %f);\n",
                        net_name.c_str(),
                        c.x, c.y, z1,
                        c.x, c.y, z2,
                        radius);
                        
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
            }
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}


void openems_model_gen::_add_zone(FILE *fp)
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
                //_mesh_x.insert(p.x);
                //_mesh_y.insert(p.y);
            }
            
            fprintf(fp, "CSX = AddLinPoly(CSX, '%s', 2, 2, %f, p, %f, 'CoordSystem', 0);\n", _pcb->get_net_name(z.net).c_str(), z1, thickness);
            fprintf(fp, "clear p;\n");
        }
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}

void openems_model_gen::_add_footprint(FILE *fp)
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


void openems_model_gen::_add_gr(const pcb::gr& gr, pcb::point at, float angle, const std::string& name, FILE *fp)
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
            
            _mesh_x.insert(xy.x);
            _mesh_y.insert(xy.y);
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
            
            _mesh_x.insert(p1.x); _mesh_y.insert(p1.y);
            _mesh_x.insert(p2.x); _mesh_y.insert(p2.y);
            _mesh_x.insert(p3.x); _mesh_y.insert(p3.y);
            _mesh_x.insert(p4.x); _mesh_y.insert(p4.y);
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
                        
            _mesh_x.insert(start.x);
            _mesh_y.insert(start.y);
        }
        else
        {
            //float thickness = _cvt_img_len(gr.stroke_width, pix_unit);
        }
    }
    fprintf(fp, "\n");
}


void openems_model_gen::_add_pad(const pcb::footprint& footprint, const pcb::pad& p, const std::string& name, FILE *fp)
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
            _mesh_x.insert(c.x);
            _mesh_y.insert(c.y);
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
            
            _mesh_x.insert(p1.x); _mesh_y.insert(p1.y);
            _mesh_x.insert(p2.x); _mesh_y.insert(p2.y);
            _mesh_x.insert(p3.x); _mesh_y.insert(p3.y);
            _mesh_x.insert(p4.x); _mesh_y.insert(p4.y);
        }
        else if (p.shape == pcb::pad::SHAPE_CIRCLE)
        {
            pcb::point c(p.at);
            
            _pcb->get_rotation_pos(footprint.at, footprint.at_angle, c);
            float radius = p.size_w / 2;
            
            fprintf(fp, "CSX = AddCylinder(CSX, '%s', 3, [%f %f %f], [%f %f %f], %f);\n",
                        name.c_str(),
                        c.x, c.y, z1,
                        c.x, c.y, z2,
                        radius);
                        
            _mesh_x.insert(c.x);
            _mesh_y.insert(c.y);
        }
        else if (p.shape == pcb::pad::SHAPE_OVAL)
        {
            
        }
    }
    
    fprintf(fp, "\n\n");
    
}


void openems_model_gen::_add_excitation(FILE *fp)
{
    if (_exc_fp1.empty())
    {
        return;
    }
    
    if (_exc_dir == 0)
    {
        float z1 = _pcb->get_layer_z_axis(_exc_fp1_layer_name);
        float thickness = _pcb->get_layer_thickness(_exc_fp1_layer_name);
        float z2 = z1 + thickness;
        pcb::pad pad1;
        pcb::pad pad2;
        pcb::footprint footprint1;
        pcb::footprint footprint2;
        if (_pcb->get_footprint(_exc_fp1, footprint1) && _pcb->get_footprint(_exc_fp2, footprint2)
                && _pcb->get_pad(_exc_fp1, _exc_fp1_pad_number, pad1)
                && _pcb->get_pad(_exc_fp2, _exc_fp2_pad_number, pad2))
        {
            pcb::point p1 = pad1.at;
            pcb::point p2 = pad2.at;
            _pcb->get_rotation_pos(footprint1.at, footprint1.at_angle, p1);
            _pcb->get_rotation_pos(footprint2.at, footprint2.at_angle, p2);
            float y1 = p1.y - std::min(pad1.size_w, pad1.size_h) / 2;
            float y2 = p2.y + std::min(pad2.size_w, pad2.size_h) / 2;
            fprintf(fp, "[CSX] = AddLumpedPort(CSX, 1, 1, 50, [%f %f %f], [%f %f %f], [1 0 0], true);\n",
                    p1.x, y1, z1,
                    p2.x, y2, z2);
                    
            _mesh_x.insert(p1.x);
            _mesh_y.insert(y1);
            _mesh_x.insert(p2.x);
            _mesh_y.insert(y2);
        }
    }
    else if (_exc_dir == 1)
    {
        float z1 = _pcb->get_layer_z_axis(_exc_fp1_layer_name);
        float thickness = _pcb->get_layer_thickness(_exc_fp1_layer_name);
        float z2 = z1 + thickness;
    }
    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
    
}

void openems_model_gen::_add_nf2ff_box(FILE *fp)
{
    float nf2ff_cx = 0;
    float nf2ff_cy = 0;
    float nf2ff_cz = 0;
    
    if (_nf2ff_fp.empty())
    {
        if (_exc_fp1.empty())
        {
            return;
        }
        
        if (_exc_dir == 0 || _exc_dir == 1)
        {
            float z1 = _pcb->get_layer_z_axis(_exc_fp1_layer_name);
            float thickness = _pcb->get_layer_thickness(_exc_fp1_layer_name);
            float z2 = z1 + thickness;
            pcb::pad pad1;
            pcb::pad pad2;
            pcb::footprint footprint1;
            pcb::footprint footprint2;
            if (_pcb->get_footprint(_exc_fp1, footprint1) && _pcb->get_footprint(_exc_fp2, footprint2)
                    && _pcb->get_pad(_exc_fp1, _exc_fp1_pad_number, pad1)
                    && _pcb->get_pad(_exc_fp2, _exc_fp2_pad_number, pad2))
            {
                pcb::point p1 = pad1.at;
                pcb::point p2 = pad2.at;
                _pcb->get_rotation_pos(footprint1.at, footprint1.at_angle, p1);
                _pcb->get_rotation_pos(footprint2.at, footprint2.at_angle, p2);
                
                nf2ff_cx = (p1.x + p2.x) / 2;
                nf2ff_cy = (p1.y + p2.y) / 2;
                nf2ff_cz = (z1 + z2) / 2;
            }
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
    //float lambda = 299792458. / (_f0 + _fc) * 1e3;
    float lambda = 299792458. / (_far_field_freq) * 1e3;
    float x_margin = std::max(fabs(nf2ff_cx - _pcb->get_edge_left()) + lambda / 10, fabs(nf2ff_cx - _pcb->get_edge_right())  + lambda / 10);
    x_margin = std::max(x_margin, lambda / 2);
    float y_margin = std::max(fabs(nf2ff_cy - _pcb->get_edge_top()) + lambda / 10, fabs(nf2ff_cy - _pcb->get_edge_bottom()) + lambda / 10);
    y_margin = std::max(y_margin, lambda / 2);
    float z_margin = std::max(_pcb->get_board_thickness() * 10, lambda / 2);
    
    _mesh_x.insert(nf2ff_cx - x_margin - lambda / 10);
    _mesh_x.insert(nf2ff_cx + x_margin + lambda / 10);
    _mesh_y.insert(nf2ff_cy - y_margin - lambda / 10);
    _mesh_y.insert(nf2ff_cy + y_margin + lambda / 10);
    _mesh_z.insert(nf2ff_cz - z_margin - lambda / 10);
    _mesh_z.insert(nf2ff_cz + z_margin + lambda / 10);
    
    fprintf(fp, "far_field_freq = %g;\n", _far_field_freq);
    fprintf(fp, "nf2ff_cx = %e; nf2ff_cy = %e; nf2ff_cz = %e;\n", nf2ff_cx, nf2ff_cy, nf2ff_cz);
    fprintf(fp, "x_margin = %e; y_margin = %e; z_margin = %e;\n", x_margin, y_margin, z_margin);
    fprintf(fp, "nf2ff_start = [nf2ff_cx - x_margin, nf2ff_cy - y_margin, nf2ff_cz - z_margin];\n");
    fprintf(fp, "nf2ff_stop = [nf2ff_cx + x_margin, nf2ff_cy + y_margin, nf2ff_cz + z_margin];\n");
    fprintf(fp, "[CSX nf2ff] = CreateNF2FFBox(CSX, 'nf2ff', nf2ff_start, nf2ff_stop);\n");


    fprintf(fp, "\n\n");
    fprintf(fp, "\n\n");
}