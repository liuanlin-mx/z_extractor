#include "Z0_calc.h"
#include <atlc.h>

std::shared_ptr<Z0_calc> Z0_calc::create(std::uint32_t type)
{
    if (type == Z0_CALC_ATLC)
    {
        return std::make_shared<atlc>();
    }
    return std::make_shared<atlc>();
}