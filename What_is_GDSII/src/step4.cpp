#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <vector>

int main() {
  std::ifstream ifs("gcd.gds", std::ios::binary);
  if (!ifs) {
    std::cerr << "Failed to open gcd.gds\n";
    return 1;
  }

  std::cout << "=== Step 4.5: 詳細なレコード構造分析 ===\n\n";

  uint8_t buffer[65536];
  int record_count = 0;
  int target_start = 5;  // レコード5 付近から表示
  int target_end = 50;

  while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
    uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                             static_cast<uint16_t>(buffer[1]);
    uint8_t rtype = buffer[2];

    uint8_t* payload = buffer + 4;
    ifs.read(reinterpret_cast<char*>(payload), record_length - 4);

    if (record_count >= target_start && record_count <= target_end) {
      std::cout << std::dec << std::setw(3) << record_count << ": Type=" 
                << std::setw(2) << static_cast<int>(rtype) 
                << " Len=" << std::setw(5) << record_length;

      // レコード型の意味を表示
      if (rtype == 0) std::cout << " HEADER";
      else if (rtype == 5) std::cout << " BGNSTR";
      else if (rtype == 6) std::cout << " STRNAME";
      else if (rtype == 7) std::cout << " ENDSTR";
      else if (rtype == 8) std::cout << " BOUNDARY";
      else if (rtype == 10) std::cout << " SREF";
      else if (rtype == 13) std::cout << " COLROW";
      else if (rtype == 14) std::cout << " NODE";
      else if (rtype == 16) std::cout << " XY";
      else if (rtype == 17) std::cout << " LAYER";
      else if (rtype == 18) std::cout << " DATATYPE";
      else if (rtype == 21) std::cout << " ENDEL";
      else if (rtype == 43) std::cout << " UNKNOWN(43)";
      else if (rtype == 44) std::cout << " UNKNOWN(44)";
      else std::cout << " TYPE:" << static_cast<int>(rtype);

      std::cout << " | ";

      // ペイロード内容を16進表示
      if (record_length <= 20) {
        for (int i = 0; i < record_length - 4; ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(payload[i]);
        }
      } else {
        for (int i = 0; i < 12; ++i) {
          std::cout << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(payload[i]);
        }
        std::cout << " ...";
      }
      std::cout << std::dec << "\n";
    }

    record_count++;
  }

  return 0;
}