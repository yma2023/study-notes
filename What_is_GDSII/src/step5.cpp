#include <fstream>
#include <iostream>
#include <cstdint>
#include <iomanip>
#include <cstring>
#include <vector>
#include <map>

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

struct Point {
  int32_t x, y;
};

struct Polygon {
  std::string cell_name;
  int layer;
  std::vector<Point> vertices;
};

int main() {
  std::ifstream ifs("gcd.gds", std::ios::binary);
  if (!ifs) {
    std::cerr << "Failed to open gcd.gds\n";
    return 1;
  }

  std::cout << "=== Step 5: 正しいGDSII構造対応版 ===\n\n";

  uint8_t buffer[65536];
  int record_count = 0;
  int boundary_count = 0;
  std::vector<Polygon> polygons;
  
  std::string current_cell = "";
  bool in_boundary = false;
  Polygon current_polygon;
  int current_layer = -1;

  while (ifs.read(reinterpret_cast<char*>(buffer), 4)) {
    uint16_t record_length = (static_cast<uint16_t>(buffer[0]) << 8) | 
                             static_cast<uint16_t>(buffer[1]);
    uint8_t rtype = buffer[2];
    uint8_t dtype = buffer[3];

    uint8_t* payload = buffer + 4;
    ifs.read(reinterpret_cast<char*>(payload), record_length - 4);

    // セル名を追跡
    if (rtype == 6) {  // STRNAME
      current_cell = std::string(reinterpret_cast<const char*>(payload), record_length - 4);
      size_t pos = current_cell.find('\0');
      if (pos != std::string::npos) {
        current_cell = current_cell.substr(0, pos);
      }
    }

    // BOUNDARY 開始
    if (rtype == 8) {  // BOUNDARY
      // 前の BOUNDARY があれば保存
      if (in_boundary && current_polygon.vertices.size() > 0) {
        polygons.push_back(current_polygon);
        boundary_count++;
      }
      
      in_boundary = true;
      current_polygon.vertices.clear();
      current_polygon.cell_name = current_cell;
      current_layer = -1;
    }

    // XY座標を読む（型16）
    if (rtype == 16 && in_boundary) {
      int payload_size = record_length - 4;
      int num_coords = payload_size / 8;  // 各座標は8バイト（int32 x 2）
      
      for (int i = 0; i < num_coords; ++i) {
        int32_t x = parse_int32(&payload[i * 8]);
        int32_t y = parse_int32(&payload[i * 8 + 4]);
        current_polygon.vertices.push_back({x, y});
      }
    }

    // LAYER 記録（型17）
    if (rtype == 17) {
      // LAYER レコード自体はペイロードが無いようなので、
      // 次のステップで layer を読む必要があるかも
    }

    // 型43, 44 で LAYER 情報が含まれているかもしれません
    if (rtype == 43 && record_length > 4) {
      // 最初の2バイトが layer かもしれません
      int16_t layer = parse_int16(payload);
      current_polygon.layer = layer;
    }

    // ENDSTR で現在のセルが終わる
    if (rtype == 7) {  // ENDSTR
      // 最後の BOUNDARY を保存
      if (in_boundary && current_polygon.vertices.size() > 0) {
        polygons.push_back(current_polygon);
        boundary_count++;
        in_boundary = false;
      }
    }

    record_count++;
  }

  // 最後の BOUNDARY があれば保存
  if (in_boundary && current_polygon.vertices.size() > 0) {
    polygons.push_back(current_polygon);
    boundary_count++;
  }

  // 最初の10個の BOUNDARY を詳しく表示
  std::cout << "最初の10個のPolygon:\n\n";
  for (int i = 0; i < std::min(10, static_cast<int>(polygons.size())); ++i) {
    std::cout << "--- Polygon #" << (i + 1) << " ---\n";
    std::cout << "Cell: " << polygons[i].cell_name << "\n";
    std::cout << "Layer: " << polygons[i].layer << "\n";
    std::cout << "Vertices (" << polygons[i].vertices.size() << "):\n";
    for (size_t j = 0; j < polygons[i].vertices.size() && j < 8; ++j) {
      std::cout << "  [" << j << "] x=" << std::setw(10) << polygons[i].vertices[j].x
                << " y=" << std::setw(10) << polygons[i].vertices[j].y << "\n";
    }
    if (polygons[i].vertices.size() > 8) {
      std::cout << "  ... (" << (polygons[i].vertices.size() - 8) << " more)\n";
    }
    std::cout << "\n";
  }

  std::cout << "\n=== 統計 ===\n";
  std::cout << "Total records: " << record_count << "\n";
  std::cout << "Polygons extracted: " << polygons.size() << "\n";

  // レイヤー別の統計
  std::map<int, int> layer_count;
  for (const auto& poly : polygons) {
    layer_count[poly.layer]++;
  }
  std::cout << "\nLayer distribution:\n";
  for (const auto& [layer, count] : layer_count) {
    std::cout << "  Layer " << std::setw(3) << layer << ": " << count << " polygons\n";
  }

  return 0;
}