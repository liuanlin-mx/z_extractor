#ifndef __FASTHENRY_H__
#define __FASTHENRY_H__
#include <list>
#include <string>
#include <vector>

class fasthenry
{
public:
    struct point
    {
        point(float x_, float y_, float z_)
            : x(x_), y(y_), z(z_)
        {
        }
        
        float x;
        float y;
        float z;
    };
    
    struct impedance_matrix
    {
        double freq;
        std::uint32_t rows;
        std::uint32_t cols;
        std::vector<std::pair<double, double> > values;
    };
    
public:
    fasthenry();
    ~fasthenry();
    
public:
    void clear();
    bool add_wire(const char *name, point start, point end, float w, float h);
    bool add_wire(const std::string& node1_name, const std::string& node2_name, const std::string& wire_name,
                    point start, point end, float w, float h, std::int32_t nwinc = 8, std::int32_t nhinc = 3);
                    
    bool add_via(const std::string& node1_name, const std::string& node2_name, const std::string& wire_name, point start, point end, float drill, float size);
    
    bool add_via(const char *name, point start, point end, float drill, float size);
    bool add_equiv(const std::string& node1_name, const std::string& node2_name);
    bool calc_impedance(const std::string& node1_name, const std::string& node2_name, double& r, double& l);
    std::string gen_ckt(const char *wire_name, const std::string& name);
    
    std::string gen_ckt2(std::list<std::string> wire_names, const std::string& name);
    void dump();
    
public:
    static void calc_wire_lr(float w, float h, float len, float& l, float& r);
    
private:
    void _call_fasthenry(std::list<std::string> wire_name);
    void _call_fasthenry(const std::string& node1_name, const std::string& node2_name);
    std::string _make_cir(const std::string& name, std::uint32_t pins);
    std::vector<impedance_matrix> _read_impedance_matrix();
    double _calc_inductance(double freq, double imag);
private:
    std::string _inp;
    std::set<std::string> _added_nodes;
    std::set<std::string> _equiv;
};

#endif