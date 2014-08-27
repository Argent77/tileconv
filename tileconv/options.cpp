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
#include "fileio.h"
#include "version.h"
#include "options.h"

const int Options::MAX_NAME_LENGTH = 1024;
const int Options::MAX_THREADS = 64;
const int Options::DEFLATE = 256;


Options::Options() noexcept
: m_haltOnError(true)
, m_silent(1)
, m_mosc(false)
, m_dithering(false)
, m_deflate(true)
, m_encoding(Encoding::BC1)
, m_inFiles()
, m_outFile()
{
}


Options::~Options() noexcept
{
  m_inFiles.clear();
}


bool Options::init(int argc, char *argv[]) noexcept
{
  int i = 1;
  if (argc <= 1) {
    showHelp();
    return false;
  } else {
    // loop through the args
    while (i < argc) {
      if (argv[i][0] != '-') {
        break;
      }

      if (std::strlen(argv[i]) != 2) {
        std::printf("Unrecognized argument: %s\n", argv[i]);
        showHelp();
        return false;
      }

      switch (argv[i][1]) {
      case 'e':
        setHaltOnError(true);
        break;
      case 's':
        setSilence(2);
        break;
      case 'v':
        setSilence(0);
        break;
      case 't':
        if (i < argc - 1) {
          int type = std::atoi(argv[i+1]);
          setEncoding(GetEncodingType(type));
          i++;    // skip type argument
        } else {
          showHelp();
          return false;
        }
        break;
      case 'o':
        if (i < argc - 1) {
          setOutput(std::string(argv[i+1], MAX_NAME_LENGTH - 1));
          i++;    // skip outfile argument
        } else {
          std::printf("Missing output file for -o\n");
          showHelp();
          return false;
        }
        break;
      case 'u':
        setDeflate(false);
        break;
      case 'd':
        setDithering(true);
        break;
      case 'z':
        setMosc(true);
        break;
//      case 'j':
//        if (i < argc - 1) {
//          int jobs = std::atoi(argv[i+1]);
//          jobs = std::max(0, std::min(MAX_THREADS, jobs));    // limiting jobs to a sane number
//          setThreads(jobs);
//          i++;    // skip jobs argument
//        } else {
//          showHelp();
//          return false;
//        }
//        break;
      case 'V':
        std::printf("%s %d.%d by %s\n", prog_name, vers_major, vers_minor, author);
        return false;
      default:
        std::printf("Unrecognized argument \"%s\"\n", argv[i]);
        showHelp();
        return false;
      }
      i++;
    }
  }

  if (i == argc && getInputCount() == 0) {
    std::printf("No input filename\n");
    showHelp();
    return false;
  }

  if (isOutput()) {
    // processing a single input file
    if (i != argc - 1) {
      std::printf("Cannot use parameter -o with multiple input files\n");
      showHelp();
      return false;
    }
    if (argv[i][0] == '-') {
      showHelp();
      return false;
    }
    if (!addInput(std::string(argv[i]))) {
      std::printf("Error opening file \"%s\"\n", argv[i]);
      return false;
    }
  } else {
    // processing multiple input files
    while (i < argc) {
      if (!addInput(std::string(argv[i]))) {
        std::printf("Error opening file \"%s\"\n", argv[i]);
        return false;
      }
      i++;
    }
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
  std::printf("  -o outfile  Select output file. (Works with single input file only!)\n");
  std::printf("  -d          Use color dithering when converting to TIS or MOS.\n");
  std::printf("  -z          MOS only: Decompress MBC into compressed MOS (MOSC).\n");
//  std::printf("  -j num      Number of threads to use for the conversion process (0=auto)");
  std::printf("  -V          Print version number and exit.\n\n");
  std::printf("Supported input file types: TIS, MOS, TBC, MBC\n");
  std::printf("Note: You can mix and match input files of each supported type.\n\n");
}

bool Options::addInput(const std::string &inFile) noexcept
{
  if (!inFile.empty()) {
    File f(inFile.c_str(), "rb");
    if (!f.error()) {
      for (auto iter = m_inFiles.cbegin(); iter != m_inFiles.cend(); ++iter) {
        if (*iter == inFile) return true;
      }
      m_inFiles.emplace_back(std::string(inFile));
      return true;
    }
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
    m_outFile.assign(outFile);
    return true;
  }
  return false;
}


void Options::setSilence(int level) noexcept
{
  m_silent = std::max(0, std::min(2, level));
}


//void Options::setThreads(int v) noexcept
//{
//  m_threads = std::max(0, std::min(MAX_THREADS, v));
//}


// ----------------------- STATIC METHODS -----------------------


FileType Options::GetFileType(const std::string &fileName) noexcept
{
  if (!fileName.empty()) {
    File f(fileName.c_str(), "rb");
    if (f.error()) {
      std::printf("Could not open \"%s\"\n", fileName.c_str());
      return FileType::UNKNOWN;
    }

    char sig[4];
    if (f.read(sig, 1, 4) != 4) {
      std::printf("Unable to read file signature from \"%s\"\n", fileName.c_str());
      return FileType::UNKNOWN;
    }

    FileType retval;
    if (std::strncmp(sig, "TIS ", 4) == 0) {
      retval = FileType::TIS;
    } else if (std::strncmp(sig, "MOS ", 4) == 0 ||
               std::strncmp(sig, "MOSC", 4) == 0) {
      retval = FileType::MOS;
    } else if (std::strncmp(sig, "TBC ", 4) == 0) {
      retval = FileType::TBC;
    } else if (std::strncmp(sig, "MBC ", 4) == 0) {
      retval = FileType::MBC;
    } else {
      std::printf("Unable to detect file type for \"%s\"\n", fileName.c_str());
      return FileType::UNKNOWN;
    }

    return retval;
  }
  return FileType::UNKNOWN;
}


std::string Options::SetFileExt(const std::string &fileName, FileType type) noexcept
{
  std::string retVal;
  if (!fileName.empty() && type != FileType::UNKNOWN) {
    uint32_t pos = fileName.find_last_of('.', fileName.size());
    if (pos != std::string::npos) {
      retVal.assign(fileName.substr(0, pos));
    }
    if (retVal.size() > MAX_NAME_LENGTH - 4) {
      retVal.erase(retVal.size(), retVal.size() - MAX_NAME_LENGTH + 4);
    }
    switch (type) {
    case FileType::TIS:
      retVal.append(".tis");
      break;
    case FileType::MOS:
      retVal.append(".mos");
      break;
    case FileType::TBC:
      retVal.append(".tbc");
      break;
    case FileType::MBC:
      retVal.append(".mbc");
      break;
    default:
      break;
    }
  }
  return retVal;
}


Encoding Options::GetEncodingType(int code) noexcept
{
  switch (code & 0xff) {
    case 0:  return Encoding::RAW;
    case 2:  return Encoding::BC2;
    case 3:  return Encoding::BC3;
    default: return Encoding::BC1;
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
    case Encoding::RAW: retVal |= 0; break;
    case Encoding::BC2: retVal |= 2; break;
    case Encoding::BC3: retVal |= 3; break;
    default:            retVal |= 1; break;
  }
  return retVal;
}


const std::string& Options::GetEncodingName(int code) noexcept
{
  static const std::string unkDesc("Unknown (unknown)");
  static const std::string desc0("Not encoded (zlib-compressed)");
  static const std::string desc1("BC1/DXT1 (zlib-compressed)");
  static const std::string desc2("BC2/DXT3 (zlib-compressed)");
  static const std::string desc3("BC3/DXT5 (zlib-compressed)");
  static const std::string desc256("Not encoded (uncompressed)");
  static const std::string desc257("BC1/DXT1 (uncompressed)");
  static const std::string desc258("BC2/DXT3 (uncompressed)");
  static const std::string desc259("BC3/DXT5 (uncompressed)");

  switch (code) {
    case 0:     return desc0;
    case 1:     return desc1;
    case 2:     return desc2;
    case 3:     return desc3;
    case 256+0: return desc256;
    case 256+1: return desc257;
    case 256+2: return desc258;
    case 256+3: return desc259;
    default:    return unkDesc;
  }
}

