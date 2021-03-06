/*
Copyright (c) 2014 Argent77

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#include <cstring>
#include "funcs.h"
#include "converter_raw.h"

namespace tc {

ConverterRaw::ConverterRaw(const Options& options, unsigned type) noexcept
: Converter(options, type)
{
}


ConverterRaw::~ConverterRaw() noexcept
{
}


int ConverterRaw::getRequiredSpace(int width, int height) const noexcept
{
  if (width > 0 && height > 0) {
    return 1024 + width*height;   // palette + indexed data
  }
  return 0;
}


int ConverterRaw::convert(uint8_t *palette, uint8_t *indexed, uint8_t *encoded, int width, int height) noexcept
{
  if (palette != nullptr && indexed != nullptr && encoded != nullptr) {
    if (isEncoding() && width > 0 && height > 0) {
      uint16_t v16;
      // initializing pixel encoding header
      v16 = (uint16_t)width; *((uint16_t*)encoded) = get16u_le(&v16); encoded += 2;
      v16 = (uint16_t)height; *((uint16_t*)encoded) = get16u_le(&v16); encoded += 2;
      setWidth(width); setHeight(height);
      // encoding graphics
      std::memcpy(encoded, palette, 1024);
      ReorderColors(encoded, 256, getColorFormat(), ColorFormat::ARGB);
      std::memcpy(encoded+1024, indexed, getWidth()*getHeight());
      return getRequiredSpace(getWidth(), getHeight()) + HEADER_TILE_ENCODED_SIZE;
    } else if (!isEncoding()) {
      setWidth(get16u_le((uint16_t*)encoded));
      setHeight(get16u_le((uint16_t*)(encoded+2)));
      std::memcpy(palette, encoded+4, 1024);
      ReorderColors(palette, 256, getColorFormat(), ColorFormat::ARGB);
      std::memcpy(indexed, encoded+1024+4, getWidth()*getHeight());
      return getRequiredSpace(getWidth(), getHeight());
    }
  }
  return 0;
}


bool ConverterRaw::isTypeValid() const noexcept
{
  return (getType() == 0);
}


}   // namespace tc
