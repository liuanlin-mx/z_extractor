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

#ifndef __MATRIX_H__
#define __MATRIX_H__
#include <cstdint>
#include <stdlib.h>
#include <new>

template <typename T>
class matrix
{
public:
    matrix()
        : _data(NULL)
        , _rows(0)
        , _cols(0)
        , _border(0)
    {
        
    }
    
    ~matrix()
    {
        _free();
    }
    
public:
    
    bool create(std::int32_t rows, std::int32_t cols, std::int32_t border = 0, const T& v = T())
    {
        _free();
        _data = (T*)malloc(sizeof(T) * (rows + border * 2) * (cols + border * 2));
        if (_data == NULL)
        {
            return false;
        }
        
        for (std::int32_t i = 0; i < (rows + border * 2) * (cols + border * 2); i++)
        {
            new(_data + i) T(v);
        }
        
        _rows = rows;
        _cols = cols;
        _border = border;
        return true;
    }
    
    inline T& at(std::int32_t row, std::int32_t col)
    {
        return _data[(row + _border) * (_cols + _border * 2) + col + _border];
    }
    
    inline const T& at(std::int32_t row, std::int32_t col) const
    {
        return _data[(row + _border) * (_cols + _border * 2) + col + _border];
    }
    
    inline T* data()
    {
        return _data;
    }
    
    inline const T* data() const
    {
        return _data;
    }
    
    inline std::int32_t rows() const
    {
        return _rows;
    }
    
    inline std::int32_t cols() const
    {
        return _cols;
    }

    inline std::int32_t border_size()
    {
        return _border;
    }
    
private:
    void _free()
    {
        if (_data)
        {
            for (std::int32_t i = 0; i < (_rows + _border * 2) * (_cols + _border * 2); i++)
            {
                ((T*)(_data + i))->~T();
            }
            free(_data);
            _data = 0;
            _rows = 0;
            _cols = 0;
            _border = 0;
        }
    }
    
private:
    T *_data;
    std::int32_t _rows;
    std::int32_t _cols;
    std::int32_t _border;
};
#endif