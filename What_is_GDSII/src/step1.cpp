#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>

// レコードタイプの定義
enum class record_type : uint8_t {
  HEADER = 0,
  BGNLIB = 1,
  LIBNAME = 2,
  UNITS = 3,
  ENDLIB = 4,
  BGNSTR = 5,
  STRNAME = 6,
  ENDSTR = 7,
  BOUNDARY = 8,
  PATH = 9,
  SREF = 10,
  AREF = 11,
  LAYER = 17,
  DATATYPE = 18,
  WIDTH = 19,
  XY = 20,
  ENDEL = 21,
  SNAME = 12,
  COLROW = 13,
  NODE = 14,
  STRANS = 24,
  MAG = 25,
  ANGLE = 26,
  PATHTYPE = 23,
  NODETYPE = 22,
  BOX = 30,
  BOXTYPE = 31,
  BGNEXTN = 34,
  ENDEXTN = 35,
};

enum class data_type : uint8_t {
  no_data = 0,
  bit_array = 1,
  int16 = 2,
  int32 = 3,
  real32 = 4,
  real64 = 5,
  ascii_string = 6,
};

const char* record_type_to_string(record_type rtype) {
  switch (rtype) {
    case record_type::HEADER: return "HEADER";
    case record_type::BGNLIB: return "BGNLIB";
    case record_type::LIBNAME: return "LIBNAME";
    case record_type::UNITS: return "UNITS";
    case record_type::ENDLIB: return "ENDLIB";
    case record_type::BGNSTR: return "BGNSTR";
    case record_type::STRNAME: return "STRNAME";
    case record_type::ENDSTR: return "ENDSTR";
    case record_type::BOUNDARY: return "BOUNDARY";
    case record_type::PATH: return "PATH";
    case record_type::SREF: return "SREF";
    case record_type::AREF: return "AREF";
    case record_type::LAYER: return "LAYER";
    case record_type::DATATYPE: return "DATATYPE";
    case record_type::WIDTH: return "WIDTH";
    case record_type::XY: return "XY";
    case record_type::ENDEL: return "ENDEL";
    case record_type::SNAME: return "SNAME";
    case record_type::COLROW: return "COLROW";
    case record_type::NODE: return "NODE";
    case record_type::STRANS: return "STRANS";
    case record_type::MAG: return "MAG";
    case record_type::ANGLE: return "ANGLE";
    case record_type::PATHTYPE: return "PATHTYPE";
    case record_type::NODETYPE: return "NODETYPE";
    case record_type::BOX: return "BOX";
    case record_type::BOXTYPE: return "BOXTYPE";
    case record_type::BGNEXTN: return "BGNEXTN";
    case record_type::ENDEXTN: return "ENDEXTN";
    default: return "UNKNOWN";
  }
}

const char* data_type_to_string(data_type dtype) {
  switch (dtype) {
    case data_type::no_data: return "no_data";
    case data_type::bit_array: return "bit_array";
    case data_type::int16: return "int16";
    case data_type::int32: return "int32";
    case data_type::real32: return "real32";
    case data_type::real64: return "real64";
    case data_type::ascii_string: return "ascii_string";
    default: return "UNKNOWN";
  }
}

int main() {
  std::ifstream ifs("gcd.gds", std::ios::binary);
  if (!ifs) {
    std::cerr << "Failed to open gcd.gds\n";
    return 1;
  }

  std::cout << "=== Step 1: GDSIIファイルレコードヘッダ読み込み ===\n\n";

  uint8_t buffer[4];
  int record_count = 0;
  int max_records = 100;  // 最初の100レコードを表示

  while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
    uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                             static_cast<uint16_t>(buffer[1]);
    record_type rtype = static_cast<record_type>(buffer[2]);
    data_type dtype = static_cast<data_type>(buffer[3]);

    if (record_count < max_records) {
      std::cout << std::dec << std::setw(4) << record_count << ": "
                << "Len=" << std::setw(5) << record_length << " | "
                << std::setw(12) << record_type_to_string(rtype) << " | "
                << std::setw(12) << data_type_to_string(dtype) << "\n";
    }

    // ペイロード部分をスキップ
    ifs.seekg(record_length - 4, std::ios::cur);
    record_count++;
  }

  std::cout << "\n=== 統計 ===\n";
  std::cout << "Total records: " << record_count << "\n";
  std::cout << "File size: " << ifs.tellg() << " bytes\n";

  return 0;
}