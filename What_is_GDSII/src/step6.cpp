#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <vector>
#include <map>
#include <string>
#include <cmath>

// ============================================================
// パーサーユーティリティ
// ============================================================

int32_t parse_int32(const uint8_t* bytes) {
  return (static_cast<int32_t>(bytes[0]) << 24) |
         (static_cast<int32_t>(bytes[1]) << 16) |
         (static_cast<int32_t>(bytes[2]) << 8) |
         static_cast<int32_t>(bytes[3]);
}

int16_t parse_int16(const uint8_t* bytes) {
  return (static_cast<int16_t>(bytes[0]) << 8) | 
         static_cast<int16_t>(bytes[1]);
}

double parse_real64(const uint8_t* bytes) {
  uint64_t data = 0;
  for (int i = 0; i < 8; ++i) {
    data = (data << 8) + static_cast<uint64_t>(bytes[i]);
  }
  int exponent = ((data & 0x7f00'0000'0000'0000) >> 56) - (64 + 14);
  double mantissa = data & 0x00ff'ffff'ffff'ffff;
  double result = mantissa * std::pow(2.0, exponent * 4);
  return (data & 0x8000'0000'0000'0000) > 0 ? -result : result;
}

// ============================================================
// データ構造
// ============================================================

struct Point {
  int32_t x, y;
};

struct Polygon {
  std::string cell_name;
  int layer;
  int datatype;
  std::vector<Point> vertices;
  
  void print_info(int index) const {
    std::cout << "--- Polygon #" << (index + 1) << " ---\n";
    std::cout << "Cell: " << cell_name << "\n";
    std::cout << "Layer: " << layer << ", DataType: " << datatype << "\n";
    std::cout << "Vertices (" << vertices.size() << "):\n";
    size_t max_display = std::min(size_t(8), vertices.size());
    for (size_t i = 0; i < max_display; ++i) {
      std::cout << "  [" << i << "] x=" << std::setw(10) << vertices[i].x
                << " y=" << std::setw(10) << vertices[i].y << "\n";
    }
    if (vertices.size() > 8) {
      std::cout << "  ... (" << (vertices.size() - 8) << " more)\n";
    }
    std::cout << "\n";
  }
};

struct GDSIILibrary {
  std::string name;
  double dbu_in_user_unit;
  double dbu_in_meter;
  std::vector<std::string> cell_names;
  std::vector<Polygon> polygons;
  std::map<int, int> layer_distribution;
  
  void analyze() {
    for (const auto& poly : polygons) {
      layer_distribution[poly.layer]++;
    }
  }
  
  void print_summary() const {
    std::cout << "\n========== GDSII Library Summary ==========\n\n";
    std::cout << "Library Name: " << name << "\n";
    std::cout << "DBU in user unit: " << dbu_in_user_unit << "\n";
    std::cout << "DBU in meter: " << dbu_in_meter << "\n";
    std::cout << "Total cells: " << cell_names.size() << "\n";
    std::cout << "Total polygons: " << polygons.size() << "\n";
    
    std::cout << "\nLayer Distribution:\n";
    for (const auto& [layer, count] : layer_distribution) {
      std::cout << "  Layer " << std::setw(3) << layer << ": " 
                << std::setw(6) << count << " polygons\n";
    }
  }
};

// ============================================================
// GDSIIパーサー
// ============================================================

class GDSIIParser {
public:
  GDSIIParser(const std::string& filepath) : filepath_(filepath) {}
  
  GDSIILibrary parse() {
    std::ifstream ifs(filepath_, std::ios::binary);
    if (!ifs) {
      throw std::runtime_error("Cannot open file: " + filepath_);
    }
    
    GDSIILibrary lib;
    uint8_t buffer[65536];
    
    std::string current_cell = "";
    bool in_boundary = false;
    Polygon current_polygon;
    
    int record_count = 0;
    
    while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
      uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                               static_cast<uint16_t>(buffer[1]);
      uint8_t rtype = buffer[2];
      
      uint8_t* payload = buffer + 4;
      ifs.read(reinterpret_cast<char*>(payload), record_length - 4);
      
      // ============================================
      // ヘッダー レベルのレコード
      // ============================================
      
      if (rtype == 2) {  // LIBNAME
        lib.name = std::string(reinterpret_cast<const char*>(payload), 
                              record_length - 5);  // null終端を除く
      }
      
      if (rtype == 3) {  // UNITS
        lib.dbu_in_user_unit = parse_real64(payload);
        lib.dbu_in_meter = parse_real64(payload + 8);
      }
      
      // ============================================
      // セル レベルのレコード
      // ============================================
      
      if (rtype == 6) {  // STRNAME
        current_cell = std::string(reinterpret_cast<const char*>(payload), 
                                  record_length - 5);
        lib.cell_names.push_back(current_cell);
      }
      
      // ============================================
      // 要素 レベルのレコード
      // ============================================
      
      if (rtype == 8) {  // BOUNDARY
        if (in_boundary && current_polygon.vertices.size() > 0) {
          lib.polygons.push_back(current_polygon);
        }
        in_boundary = true;
        current_polygon.vertices.clear();
        current_polygon.cell_name = current_cell;
        current_polygon.layer = -1;
        current_polygon.datatype = -1;
      }
      
      if (rtype == 16 && in_boundary) {  // XY
        int payload_size = record_length - 4;
        int num_coords = payload_size / 8;
        for (int i = 0; i < num_coords; ++i) {
          int32_t x = parse_int32(&payload[i * 8]);
          int32_t y = parse_int32(&payload[i * 8 + 4]);
          current_polygon.vertices.push_back({x, y});
        }
      }
      
      if (rtype == 43 && record_length >= 6) {  // LAYER (型43?)
        int16_t layer = parse_int16(payload);
        current_polygon.layer = layer;
      }
      
      if (rtype == 44 && record_length >= 6) {  // DATATYPE (型44?)
        int16_t datatype = parse_int16(payload);
        current_polygon.datatype = datatype;
      }
      
      if (rtype == 7) {  // ENDSTR
        if (in_boundary && current_polygon.vertices.size() > 0) {
          lib.polygons.push_back(current_polygon);
          in_boundary = false;
        }
      }
      
      record_count++;
    }
    
    // 最後の BOUNDARY を保存
    if (in_boundary && current_polygon.vertices.size() > 0) {
      lib.polygons.push_back(current_polygon);
    }
    
    lib.analyze();
    return lib;
  }
  
private:
  std::string filepath_;
};

// ============================================================
// メイン処理
// ============================================================

int main() {
  try {
    std::cout << "=== Step 6: 完全なGDSII解析システム ===\n\n";
    
    GDSIIParser parser("gcd.gds");
    GDSIILibrary lib = parser.parse();
    
    // サマリーを表示
    lib.print_summary();
    
    // 最初の5個のポリゴンを詳しく表示
    std::cout << "\n\nFirst 5 Polygons Details:\n";
    std::cout << "==========================\n";
    for (int i = 0; i < std::min(5, static_cast<int>(lib.polygons.size())); ++i) {
      lib.polygons[i].print_info(i);
    }
    
    std::cout << "✓ Successfully parsed gcd.gds\n";
    return 0;
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}