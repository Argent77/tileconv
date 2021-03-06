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
#include "converter_raw.h"
#include "converter_dxt.h"
#include "converter_z.h"
//#include "converter_webp.h"
#include "converterfactory.h"

namespace tc {

ConverterPtr ConverterFactory::GetConverter(const Options& options, unsigned type) noexcept
{
  type &= 0xff;
  switch (type) {
    case ENCODE_RAW:
      // No conversion: RAW encoder/decoder
      return ConverterPtr(new ConverterRaw(options, type));
    case ENCODE_DXT1:
    case ENCODE_DXT3:
    case ENCODE_DXT5:
      // DXTn encoder/decoder
      return ConverterPtr(new ConverterDxt(options, type));
//    case ENCODE_WEBP:
      // WebP encoder/decoder
//      return ConverterPtr(new ConverterWebP(options, type));
    case ENCODE_Z:
      // MOZ/TIZ decoder
      return ConverterPtr(new ConverterZ(options, type));
    default:
      return nullptr;
  }
}

}   // namespace tc
