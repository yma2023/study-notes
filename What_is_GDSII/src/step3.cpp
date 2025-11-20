#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <vector>

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
    case 16: return "XY";  // Likely
    case 17: return "LAYER";
    case 18: return "DATATYPE";
    case 19: return "WIDTH";
    case 20: return "XY_ALT";  // Alternative?
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
    case 43: return "UNKNOWN(43)";  // Need to investigate
    case 44: return "UNKNOWN(44)";  // Need to investigate
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

int16_t parse_int16(const uint8_t* bytes) {
  return (static_cast<int16_t>(bytes[0]) << 8) | 
         static_cast<int16_t>(bytes[1]);
}

int32_t parse_int32(const uint8_t* bytes) {
  return (static_cast<int32_t>(bytes[0]) << 24) |
         (static_cast<int32_t>(bytes[1]) << 16) |
         (static_cast<int32_t>(bytes[2]) << 8) |
         static_cast<int32_t>(bytes[3]);
}

int main() {
  std::ifstream ifs("gcd.gds", std::ios::binary);
  if (!ifs) {
    std::cerr << "Failed to open gcd.gds\n";
    return 1;
  }

  std::cout << "=== Step 3: レコード内容の詳細確認 ===\n\n";

  uint8_t buffer[65536];
  int record_count = 0;
  int display_limit = 150;  // 最初の150レコードを表示

  while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
    uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                             static_cast<uint16_t>(buffer[1]);
    uint8_t rtype = buffer[2];
    uint8_t dtype = buffer[3];

    // ペイロード部分を読む
    uint8_t* payload = buffer + 4;
    ifs.read(reinterpret_cast<char*>(payload), record_length - 4);

    if (record_count < display_limit) {
      std::cout << std::dec << std::setw(4) << record_count << ": "
                << "Len=" << std::setw(5) << record_length << " | "
                << std::setw(15) << record_type_to_string(rtype) << " | "
                << std::setw(12) << data_type_to_string(dtype);

      // ペイロードの内容を表示
      if (record_length > 4) {
        int payload_size = record_length - 4;
        std::cout << " | Data(hex): ";
        for (int i = 0; i < std::min(payload_size, 16); ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(payload[i]) << " ";
        }
        if (payload_size > 16) std::cout << "...";
        
        // 特定のレコードタイプでデータを解析
        std::cout << std::dec;
        if (dtype == 2 && payload_size == 2) {  // int16
          int16_t val = parse_int16(payload);
          std::cout << " -> int16=" << val;
        } else if (dtype == 3 && payload_size == 4) {  // int32
          int32_t val = parse_int32(payload);
          std::cout << " -> int32=" << val;
        } else if (dtype == 6 && payload_size > 0) {  // string
          std::string str(reinterpret_cast<const char*>(payload), payload_size);
          std::cout << " -> string=\"" << str << "\"";
        }
      }
      std::cout << "\n";
    }

    record_count++;
  }

  std::cout << "\nTotal records: " << record_count << "\n";

  return 0;
}