#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>
#include <map>

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
  SNAME = 12,
  COLROW = 13,
  NODE = 14,
  // 15-16 は予約済み
  LAYER = 17,
  DATATYPE = 18,
  WIDTH = 19,
  XY = 20,
  ENDEL = 21,
  NODETYPE = 22,
  PATHTYPE = 23,
  STRANS = 24,
  MAG = 25,
  ANGLE = 26,
  // 27-29 は予約済み
  BOX = 30,
  BOXTYPE = 31,
  // 32-33 は予約済み
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

const char* record_type_to_string(uint8_t rtype) {
  switch (rtype) {
    case 0: return "HEADER";
    case 1: return "BGNLIB";
    case 2: return "LIBNAME";
    case 3: return "UNITS";
    case 4: return "ENDLIB";
    case 5: return "BGNSTR";
    case 6: return "STRNAME";
    case 7: return "ENDSTR";
    case 8: return "BOUNDARY";
    case 9: return "PATH";
    case 10: return "SREF";
    case 11: return "AREF";
    case 12: return "SNAME";
    case 13: return "COLROW";
    case 14: return "NODE";
    case 17: return "LAYER";
    case 18: return "DATATYPE";
    case 19: return "WIDTH";
    case 20: return "XY";
    case 21: return "ENDEL";
    case 22: return "NODETYPE";
    case 23: return "PATHTYPE";
    case 24: return "STRANS";
    case 25: return "MAG";
    case 26: return "ANGLE";
    case 30: return "BOX";
    case 31: return "BOXTYPE";
    case 34: return "BGNEXTN";
    case 35: return "ENDEXTN";
    default: {
      static char buf[20];
      sprintf(buf, "UNKNOWN(%d)", rtype);
      return buf;
    }
  }
}

const char* data_type_to_string(uint8_t dtype) {
  switch (dtype) {
    case 0: return "no_data";
    case 1: return "bit_array";
    case 2: return "int16";
    case 3: return "int32";
    case 4: return "real32";
    case 5: return "real64";
    case 6: return "ascii_string";
    default: return "UNKNOWN";
  }
}

int main() {
  std::ifstream ifs("gcd.gds", std::ios::binary);
  if (!ifs) {
    std::cerr << "Failed to open gcd.gds\n";
    return 1;
  }

  std::cout << "=== Step 2: レコード型の統計 ===\n\n";

  std::map<uint8_t, int> record_count_map;
  std::map<uint8_t, int> unknown_records;

  uint8_t buffer[4];
  int total_records = 0;

  while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
    uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                             static_cast<uint16_t>(buffer[1]);
    uint8_t rtype = buffer[2];
    uint8_t dtype = buffer[3];

    record_count_map[rtype]++;

    // 不明なレコード型を記録
    if (rtype > 35 || (rtype > 26 && rtype < 30) || (rtype > 31 && rtype < 34) ||
        (rtype > 13 && rtype < 17) || rtype == 15 || rtype == 16) {
      unknown_records[rtype]++;
    }

    // ペイロード部分をスキップ
    ifs.seekg(record_length - 4, std::ios::cur);
    total_records++;
  }

  std::cout << "Record Type Distribution:\n";
  std::cout << "========================\n";
  for (auto& [rtype, count] : record_count_map) {
    std::cout << std::setw(3) << static_cast<int>(rtype) << ": "
              << std::setw(15) << record_type_to_string(rtype) << " : "
              << std::setw(8) << count << " occurrences\n";
  }

  std::cout << "\n=== 統計 ===\n";
  std::cout << "Total records: " << total_records << "\n";

  if (!unknown_records.empty()) {
    std::cout << "\nUnknown record types found:\n";
    for (auto& [rtype, count] : unknown_records) {
      std::cout << "  Type " << static_cast<int>(rtype) << ": " << count << " times\n";
    }
  }

  return 0;
}