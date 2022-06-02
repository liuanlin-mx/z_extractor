#include "mmtl.h"

static const char *base_xsctn = "package require csdl\n\n"
                "set _title \"Example Coplanar Waveguide\"\n"
                "set ::Stackup::couplingLength \"0.0254\"\n"
                "set ::Stackup::riseTime \"25\"\n"
                "set ::Stackup::frequency \"1000MHz\"\n"
                "set ::Stackup::defaultLengthUnits \"meters\"\n"
                "set CSEG 10\n"
                "set DSEG 10\n"
                
                "GroundPlane ground  \\\n"
                "   -thickness 0.035 \\\n"
                "   -yOffset 0.0 \\\n"
                "   -xOffset 0.0\n"
                "DielectricLayer dielAir  \\\n"
                "   -thickness 10 \\\n"
                "   -lossTangent 0.0002 \\\n"
                "   -permittivity 1.0 \\\n"
                "   -permeability 1.0 \\\n"
                "   -yOffset 0.0 \\\n"
                "   -xOffset 0.0\n";
                
mmtl::mmtl()
{
    _xsctn = base_xsctn;
}


mmtl::~mmtl()
{
}

void mmtl::set_precision(float unit)
{
}


void mmtl::set_box_size(float w, float h)
{
}


void mmtl::clean()
{
    _xsctn = base_xsctn;
}


void mmtl::clean_all()
{
    _xsctn = base_xsctn;
}


void mmtl::add_ground(float x, float y, float w, float thickness)
{
}


void mmtl::add_ring_ground(float x, float y, float r, float thickness)
{
}


void mmtl::add_wire(float x, float y, float w, float thickness)
{
}


void mmtl::add_ring_wire(float x, float y, float r, float thickness)
{
}


void mmtl::add_coupler(float x, float y, float w, float thickness)
{
}


void mmtl::add_elec(float x, float y, float w, float thickness, float er)
{
}


void mmtl::add_ring_elec(float x, float y, float r, float thickness, float er)
{
}


bool mmtl::calc_zo(float & Zo, float & v, float & c, float & l)
{
    char cmd[512] = {0};
    
    FILE *fp = fopen("tmp.xsctn", "wb");
    if (fp)
    {
        fwrite(_xsctn.c_str(), 1, _xsctn.length(), fp);
        fclose(fp);
    }
    
    sprintf(cmd, "mmtl_bem tmp");
    printf("%s\n", cmd);
    
    char buf[1024] = {0};
    FILE *pfp = popen(cmd, "r");
    if (fgets(buf, sizeof(buf), pfp))
    {
    }
    pclose(pfp);
    return true;
    
}
bool mmtl::calc_coupled_zo(float & Zodd, float & Zeven, float & Zdiff, float & Zcomm, float & Lodd, float & Leven, float & Codd, float & Ceven)
{
    return false;
}
