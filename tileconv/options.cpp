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
#include <cstdint>
#include <cstring>
#include <unistd.h>
#include "fileio.h"
#include "tilethreadpool.h"
#include "version.h"
#include "converterfactory.h"
#include "options.h"

namespace tc {

const int Options::MAX_THREADS          = 64;
const int Options::DEFLATE              = 256;

const bool Options::DEF_HALT_ON_ERROR   = true;
const bool Options::DEF_MOSC            = false;
const bool Options::DEF_DEFLATE         = true;
const bool Options::DEF_SHOWINFO        = false;
const bool Options::DEF_ASSUMETIS       = false;
const int Options::DEF_VERBOSITY        = 1;
const int Options::DEF_QUALITY_DECODING = 4;
const int Options::DEF_QUALITY_ENCODING = 9;
const int Options::DEF_THREADS          = 0;    // autodetect
const Encoding Options::DEF_ENCODING    = Encoding::BC1;

// Supported parameter names
const char Options::ParamNames[] = "esvt:uo:zdq:j:TIV";


Options::Options() noexcept
: m_haltOnError(DEF_HALT_ON_ERROR)
, m_mosc(DEF_MOSC)
, m_deflate(DEF_DEFLATE)
, m_showInfo(DEF_SHOWINFO)
, m_assumeTis(DEF_ASSUMETIS)
, m_verbosity(DEF_VERBOSITY)
, m_qualityDecoding(DEF_QUALITY_DECODING)
, m_qualityEncoding(DEF_QUALITY_ENCODING)
, m_threads(DEF_THREADS)
, m_encoding(DEF_ENCODING)
, m_inFiles()
, m_outPath()
, m_outFile()
{
}


Options::~Options() noexcept
{
  m_inFiles.clear();
}


bool Options::init(int argc, char *argv[]) noexcept
{
  if (argc <= 1) {
    showHelp();
    return false;
  }

  int c = 0;
  opterr = 0;
  while ((c = getopt(argc, argv, ParamNames)) != -1) {
    switch (c) {
      case 'e':
        setHaltOnError(false);
        break;
      case 's':
        setVerbosity(0);
        break;
      case 'v':
        setVerbosity(2);
        break;
      case 't':
        if (optarg != nullptr) {
          int type = std::atoi(optarg);
          ConverterPtr converter = ConverterFactory::GetConverter(*this, type);
          if (converter != nullptr && converter->canEncode()) {
            setEncoding(GetEncodingType(type));
          } else {
            std::printf("Unsupported pixel encoding type: %d\n", type);
            showHelp();
            return false;
          }
        } else {
          showHelp();
          return false;
        }
        break;
      case 'u':
        setDeflate(false);
        break;
      case 'o':
        if (optarg != nullptr) {
          setOutput(std::string(optarg));
        } else {
          std::printf("Missing output file for -o\n");
          showHelp();
          return false;
        }
        break;
      case 'z':
        setMosc(true);
        break;
      case 'd':
        std::printf("Warning: Parameter -d is deprecated. Use -q instead!\n");
        break;
      case 'q':
        if (optarg != nullptr) {
          int levelE = DEF_QUALITY_ENCODING;
          int levelD = DEF_QUALITY_DECODING;
          if (optarg[0] >= '0' && optarg[0] <= '9') {
            levelD = optarg[0] - '0';
          } else if (optarg[0] != '-') {
            std::printf("Error: Unrecognized decoding quality level or placeholder.\n");
            showHelp();
            return false;
          }
          if (optarg[1] >= '0' && optarg[1] <= '9') {
            levelE = optarg[1] - '0';
          } else if (optarg[1] != '-' && optarg[1] != 0) {
            std::printf("Error: Unrecognized encoding quality level or placeholder.\n");
            showHelp();
            return false;
          }
          setQuality(levelE, levelD);
        } else {
          showHelp();
          return false;
        }
        break;
      case 'j':
        if (optarg != nullptr) {
          int jobs = std::atoi(optarg);
          setThreads(jobs);
        } else {
          showHelp();
          return false;
        }
        break;
      case 'T':
        setAssumeTis(true);
        break;
      case 'I':
        setShowInfo(true);
        break;
      case 'V':
        if (std::strlen(vers_suffix)) {
          std::printf("%s %d.%d.%d (%s) by %s\n", prog_name, vers_major, vers_minor, vers_patch, vers_suffix, author);
        } else {
          if (vers_patch != 0) {
            std::printf("%s %d.%d.%d by %s\n", prog_name, vers_major, vers_minor, vers_patch, author);
          } else {
            std::printf("%s %d.%d by %s\n", prog_name, vers_major, vers_minor, author);
          }
        }
        return false;
      default:
        std::printf("Unrecognized parameter \"-%c\"\n", optopt);
        showHelp();
        return false;
    }
  }

  // finalizing options
  if (isDeflate()) {
    // disabling deflating for certain encoding types
    ConverterPtr converter =
        ConverterFactory::GetConverter(*this, GetEncodingCode(getEncoding(), isDeflate()));
    if (converter != nullptr) {
      setDeflate(converter->deflateAllowed());
    }
  }

  // remaining arguments are assumed to be input filenames
  for (; optind < argc; optind++) {
    if (argv[optind][0] == '-') {
      showHelp();
      return false;
    } else if (!addInput(std::string(argv[optind]))) {
      std::printf("Error opening file \"%s\"\n", argv[optind]);
      return false;
    }
  }

  // checking special conditions
  if (getInputCount() == 0) {
    std::printf("No input filename specified\n");
    showHelp();
    return false;
  } else if (getInputCount() > 1 && isOutFile()) {
    std::printf("You cannot specify output file with multiple input files\n");
    showHelp();
    return false;
  }

  return true;
}


void Options::showHelp() noexcept
{
  std::printf("\nUsage: %s [options] infile [infile2 [...]]\n", prog_name);
  std::printf("\nOptions:\n");
  std::printf("  -e          Do not halt on errors.\n");
  std::printf("  -s          Be silent.\n");
  std::printf("  -v          Be verbose.\n");
  std::printf("  -t type     Select pixel encoding type.\n");
  std::printf("              Supported types:\n");
  std::printf("                0: No pixel encoding\n");
  std::printf("                1: BC1/DXT1 (Default)\n");
  std::printf("                2: BC2/DXT3\n");
  std::printf("                3: BC3/DXT5\n");
  std::printf("  -u          Do not apply tile compression.\n");
  std::printf("  -o output   Select output file or folder.\n");
  std::printf("              (Note: Output file works only with single input file!)\n");
  std::printf("  -z          Decode MBC/MOZ into compressed MOS (MOSC).\n");
  std::printf("  -q Dec[Enc] Set quality levels for decoding and, optionally, encoding.\n");
  std::printf("              Supported levels: 0..9 (Defaults: 4 for decoding, 9 for encoding)\n");
  std::printf("              (0=fast and lower quality, 9=slow and higher quality)\n");
  std::printf("              Specify both levels as a single argument. First digit indicates\n");
  std::printf("              decoding quality and second digit indicates encoding quality.\n");
  std::printf("              Specify '-' as placeholder for default levels.\n");
  std::printf("              Example 1: -q 27 (decoding level: 2, encoding level: 7)\n");
  std::printf("              Example 2: -q -7 (default decoding level, encoding level: 7)\n");
  std::printf("              Example 3: -q 2  (decoding level: 2, default encoding level)\n");
  std::printf("              Applied level-dependent features for encoding (DXTn only):\n");
  std::printf("                  Iterative cluster fit:   levels 7 to 9\n");
  std::printf("                  Single cluster fit:      levels 3 to 6\n");
  std::printf("                  Range fit:               levels 0 to 2\n");
  std::printf("                  Weight color by alpha:   levels 5 to 9\n");
  std::printf("              Applied level-dependent features for decoding:\n");
  std::printf("                  Dithering:               levels 5 to 9\n");
  std::printf("                  Posterization:           levels 0 to 2\n");
  std::printf("                  Additional techniques:   levels 4 to 9\n");
  std::printf("  -j num      Number of parallel jobs to speed up the conversion process.\n");
  std::printf("              Valid numbers: 0 (autodetect), 1..%d (Default: 0)\n", TileThreadPool::MAX_THREADS);
  std::printf("  -T          Treat unrecognized input files as headerless TIS.\n");
  std::printf("  -I          Show file information and exit.\n");
  std::printf("  -V          Print version number and exit.\n\n");
  std::printf("Supported input file types: TIS, MOS, TBC, MBC, TIZ, MOZ\n");
  std::printf("Note: You can mix and match input files of each supported type.\n\n");
}

bool Options::addInput(const std::string &inFile) noexcept
{
  if (!inFile.empty()) {
    m_inFiles.emplace_back(std::string(inFile));
    return true;
  }
  return false;
}


void Options::removeInput(int idx) noexcept
{
  if (idx >= 0 && (uint32_t)idx < m_inFiles.size()) {
    int i = 0;
    auto iter = m_inFiles.begin();
    for (; iter != m_inFiles.end() && i != idx; ++iter, ++i) {
      m_inFiles.erase(iter);
      break;
    }
  }
}


const std::string& Options::getInput(int idx) const noexcept
{
  static const std::string defString;   // empty default string

  if (idx >= 0 && (uint32_t)idx < m_inFiles.size()) {
    int i = 0;
    auto iter = m_inFiles.cbegin();
    for (; iter != m_inFiles.cend(); ++iter, ++i) {
      if (i == idx) return *iter;
    }
  }
  return defString;
}


bool Options::setOutput(const std::string &outFile) noexcept
{
  if (!outFile.empty()) {
    if (File::IsDirectory(outFile.c_str())) {
      m_outPath.assign(outFile);
      m_outFile.clear();
    } else {
      m_outPath.assign(File::ExtractFilePath(outFile));
      m_outFile.assign(File::ExtractFileName(outFile));
    }
    return true;
  }
  return false;
}


void Options::setVerbosity(int level) noexcept
{
  m_verbosity = std::max(0, std::min(2, level));
}


void Options::setQuality(int enc, int dec) noexcept
{
  setDecodingQuality(dec);
  setEncodingQuality(enc);
}


void Options::setDecodingQuality(int v) noexcept
{
  m_qualityDecoding = std::max(0, std::min(9, v));
}


void Options::setEncodingQuality(int v) noexcept
{
  m_qualityEncoding = std::max(0, std::min(9, v));
}


void Options::setThreads(int  v) noexcept
{
  // limiting threads to a sane number
  m_threads = std::max(0, std::min((int)TileThreadPool::MAX_THREADS, v));
}


int Options::getThreads() const noexcept
{
  return m_threads ? m_threads : getThreadPoolAutoThreads();
}


// ----------------------- STATIC METHODS -----------------------


FileType Options::GetFileType(const std::string &fileName, bool assumeTis) noexcept
{
  if (!fileName.empty()) {
    File f(fileName.c_str(), "rb");
    if (f.error()) {
      return FileType::UNKNOWN;
    }

    char sig[4];
    if (f.read(sig, 1, 4) != 4) {
      return FileType::UNKNOWN;
    }

    FileType retVal;
    if (std::strncmp(sig, "TIS ", 4) == 0) {
      retVal = FileType::TIS;
    } else if (std::strncmp(sig, "MOS ", 4) == 0 ||
               std::strncmp(sig, "MOSC", 4) == 0) {
      retVal = FileType::MOS;
    } else if (std::strncmp(sig, "TBC ", 4) == 0) {
      retVal = FileType::TBC;
    } else if (std::strncmp(sig, "MBC ", 4) == 0) {
      retVal = FileType::MBC;
    } else if (std::strncmp(sig, "TIZ0", 4) == 0) {
      retVal = FileType::TIZ;
    } else if (std::strncmp(sig, "MOZ0", 4) == 0) {
      retVal = FileType::MOZ;
    } else {
      if (assumeTis) {
        long size = f.getsize();
        if ((size % 0x1400) == 0) {
          return FileType::TIS;
        } else {
          return FileType::UNKNOWN;
        }
      } else {
        return FileType::UNKNOWN;
      }
    }

    return retVal;
  }
  return FileType::UNKNOWN;
}

std::string Options::GetFileExt(FileType type) noexcept
{
  switch (type) {
    case FileType::TIS: return std::string(".tis");
    case FileType::MOS: return std::string(".mos");
    case FileType::TBC: return std::string(".tbc");
    case FileType::MBC: return std::string(".mbc");
    case FileType::TIZ: return std::string(".tiz");
    case FileType::MOZ: return std::string(".moz");
    default:            return std::string();
  }
}

std::string Options::GetOutputFileName(const std::string &path, const std::string inputFile,
                                       FileType type, bool overwrite) noexcept
{
  static const std::string defString;   // empty default string

  if (!inputFile.empty() && type != FileType::UNKNOWN) {
    std::string outFileBase, outFileExt, outPath;

    // getting path
    if (path.empty()) {
      // using input file and path
      outPath = File::ExtractFilePath(inputFile);
    } else {
      // using separate path with input file
      outPath = path;
    }

    // getting filename
    outFileBase = File::ExtractFileBase(inputFile);
    outFileExt = GetFileExt(type);

    // making output file unique if necessary
    std::string output(File::CreateFileName(outPath, outFileBase + outFileExt));
    for (int idx = 0; !overwrite && File::Exists(output); idx++) {
      std::string suffix("-" +  std::to_string(idx));
      output = File::CreateFileName(outPath, outFileBase + outFileExt + suffix);
    }
    return output;
  }
  return defString;
}

Encoding Options::GetEncodingType(int code) noexcept
{
  switch (code & 0xff) {
    case ENCODE_RAW:  return Encoding::RAW;
    case ENCODE_DXT1: return Encoding::BC1;
    case ENCODE_DXT3: return Encoding::BC2;
    case ENCODE_DXT5: return Encoding::BC3;
//    case ENCODE_WEBP: return Encoding::WEBP;
    case ENCODE_Z:    return Encoding::Z;
    default:          return Encoding::UNKNOWN;
  }
}

bool Options::IsTileDeflated(int code) noexcept
{
  return (code & DEFLATE) == 0;
}

unsigned Options::GetEncodingCode(Encoding type, bool deflate) noexcept
{
  unsigned retVal = deflate ? 0 : DEFLATE;
  switch (type) {
    case Encoding::RAW:
      retVal |= ENCODE_RAW;
      break;
    case Encoding::BC1:
      retVal |= ENCODE_DXT1;
      break;
    case Encoding::BC2:
      retVal |= ENCODE_DXT3;
      break;
    case Encoding::BC3:
      retVal |= ENCODE_DXT5;
      break;
    case Encoding::Z:
      retVal |= ENCODE_Z;
      break;
    default:
      retVal = -1;
      break;
  }
  return retVal;
}

const std::string& Options::GetEncodingName(int code) noexcept
{
  static const std::string descUnk("Unknown (unknown)");
  static const std::string descRawDef("Not encoded (zlib-compressed)");
  static const std::string descDxt1Def("BC1/DXT1 (zlib-compressed)");
  static const std::string descDxt3Def("BC2/DXT3 (zlib-compressed)");
  static const std::string descDxt5Def("BC3/DXT5 (zlib-compressed)");
  static const std::string descZ("JPEG compressed TIZ/MOZ");
  static const std::string descRaw("Not encoded (uncompressed)");
  static const std::string descDxt1("BC1/DXT1 (uncompressed)");
  static const std::string descDxt3("BC2/DXT3 (uncompressed)");
  static const std::string descDxt5("BC3/DXT5 (uncompressed)");

  switch (code) {
    case ENCODE_RAW: return descRawDef;
    case ENCODE_DXT1: return descDxt1Def;
    case ENCODE_DXT3: return descDxt3Def;
    case ENCODE_DXT5: return descDxt5Def;
    case ENCODE_Z: return descZ;
    case ENCODE_RAW | 256: return descRaw;
    case ENCODE_DXT1 | 256: return descDxt1;
    case ENCODE_DXT3 | 256: return descDxt3;
    case ENCODE_DXT5 | 256: return descDxt5;
    case ENCODE_Z | 256: return descZ;
    default: return descUnk;
  }
}

std::string Options::getOptionsSummary(bool complete) const noexcept
{
  std::string sum;

  if (complete || getEncoding() != DEF_ENCODING || isDeflate() != DEF_DEFLATE) {
    if (!sum.empty()) sum += ", ";
    sum += "pixel encoding = ";
    sum += GetEncodingName(GetEncodingCode(m_encoding, m_deflate));
  }

  if (complete || isHaltOnError() != DEF_HALT_ON_ERROR) {
    if (!sum.empty()) sum += ", ";
    sum += "halt on errors = ";
    sum += isHaltOnError() ? "enabled" : "disabled";
  }

  if (complete || getVerbosity() != DEF_VERBOSITY) {
    if (!sum.empty()) sum += ", ";
    sum += "verbosity level = ";
    switch (getVerbosity()) {
      case 0:  sum += "silent"; break;
      case 2:  sum += "verbose"; break;
      default: sum += "default"; break;
    }
  }

  if (complete || getDecodingQuality() != DEF_QUALITY_DECODING) {
    if (!sum.empty()) sum += ", ";
    sum += "decoding quality = " + std::to_string(getDecodingQuality());
  }

  if (complete || getEncodingQuality() != DEF_QUALITY_ENCODING) {
    if (!sum.empty()) sum += ", ";
    sum += "encoding quality = " + std::to_string(getEncodingQuality());
  }

  if (complete || isMosc() != DEF_MOSC) {
    if (!sum.empty()) sum += ", ";
    if (isMosc()) sum += "convert MBC to MOSC";
    else sum += "convert MBC to MOS";
  }

  if (complete || assumeTis() != DEF_ASSUMETIS) {
    if (!sum.empty()) sum += ", ";
    if (assumeTis()) sum += "headerless TIS allowed";
    else sum += "headerless TIS not allowed";
  }

  if (complete || getThreads() != DEF_THREADS) {
    if (!sum.empty()) sum += ", ";
    sum += "jobs = ";
    if (getThreads() == 0) {
      sum += "autodetected (" + std::to_string(getThreadPoolAutoThreads()) + ")";
    } else {
      sum += std::to_string(getThreads());
    }
  }

  return sum;
}

}   // namespace tc
