#ifndef __FASTHENRY_H__
#define __FASTHENRY_H__
#include <list>
#include <string>
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
    
public:
    fasthenry();
    ~fasthenry();
    
public:
    void clear();
    bool add_wire(const char *name, point start, point end, float w, float h);
    bool add_via(const char *name, point start, point end, float drill, float size);
    
    std::string gen_ckt(const char *wire_name, const std::string& name);
    
    std::string gen_ckt2(std::list<std::string> wire_names, const std::string& name);
    void dump();
    
public:
    static void calc_wire_lr(float w, float h, float len, float& l, float& r);
    
private:
    void _call_fasthenry(std::list<std::string> wire_name);
    std::string _make_cir(const std::string& name, std::uint32_t pins);
    
private:
    std::string _inp;
};

#endif