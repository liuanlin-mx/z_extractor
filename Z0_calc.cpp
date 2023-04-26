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

#include "Z0_calc.h"
#include "atlc.h"
#include "mmtl.h"

std::shared_ptr<Z0_calc> Z0_calc::create(std::uint32_t type)
{
    if (type == Z0_CALC_ATLC)
    {
        return std::make_shared<atlc>();
    }
    else if (type == Z0_CALC_MMTL)
    {
        return std::make_shared<mmtl>();
    }
    return std::make_shared<atlc>();
}