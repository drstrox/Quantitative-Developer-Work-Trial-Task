#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <iomanip>
#include <cstdlib> // For atof, atoll, atoi

// --- OrderBook Class Definition ---
// This class encapsulates all the logic for managing the order book.
class OrderBook {
public:
    // Represents a single order. Nested struct.
    struct Order {
        double price;
        int size;
        char side;
    };

    // Processes an 'Add' event.
    void addOrder(long long order_id, double price, int size, char side) {
        if (order_id != 0 && size > 0) {
            order_map[order_id] = {price, size, side};
            updateBook(side, price, size);
        }
    }

    // Processes a 'Cancel' event.
    void cancelOrder(long long order_id) {
        if (order_map.count(order_id)) {
            const auto& ord = order_map.at(order_id);
            updateBook(ord.side, ord.price, -ord.size);
            order_map.erase(order_id);
        }
    }

    // Processes a 'Fill' event.
    void fillOrder(long long order_id, int size) {
        if (order_map.count(order_id) && size > 0) {
            auto& ord = order_map.at(order_id);
            updateBook(ord.side, ord.price, -size);
            ord.size -= size;
            if (ord.size <= 0) {
                order_map.erase(order_id);
            }
        }
    }
    
    // Clears all books.
    void reset() {
        order_map.clear();
        bid_book.clear();
        ask_book.clear();
    }

    // Writes a snapshot of the book to a stringstream.
    void writeSnapshot(std::stringstream& oss, std::string_view ts) const {
        oss << ts;
        int count = 0;
        for (const auto& [price, size] : bid_book) {
            if (count >= 10) break;
            oss << "," << std::fixed << std::setprecision(2) << price << "," << size;
            count++;
        }
        for (int i = count; i < 10; ++i) oss << ",,";

        count = 0;
        for (const auto& [price, size] : ask_book) {
            if (count >= 10) break;
            oss << "," << std::fixed << std::setprecision(2) << price << "," << size;
            count++;
        }
        for (int i = count; i < 10; ++i) oss << ",,";
        oss << "\n";
    }

private:
    std::unordered_map<long long, Order> order_map;
    std::map<double, int, std::greater<double>> bid_book;
    std::map<double, int> ask_book;

    void updateBook(char side, double price, int size_diff) {
        if (side == 'B') {
            bid_book[price] += size_diff;
            if (bid_book[price] <= 0) bid_book.erase(price);
        } else if (side == 'A') {
            ask_book[price] += size_diff;
            if (ask_book[price] <= 0) ask_book.erase(price);
        }
    }
};


// --- Utility and Main Functions ---

// Fast, lightweight CSV string splitter.
std::vector<std::string_view> splitString(std::string_view s, char delimiter) {
    std::vector<std::string_view> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != std::string_view::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

// Helper to convert a string_view to a number without heap allocation.
// Uses a stack buffer to create a temporary null-terminated string.
template<typename T>
T sv_to_num(std::string_view sv) {
    if (sv.empty()) return 0;
    char buf[32]; // Stack buffer, sufficient for prices/sizes/ids
    if (sv.length() >= sizeof(buf)) return 0; // Avoid overflow
    sv.copy(buf, sv.length());
    buf[sv.length()] = '\0';

    if constexpr (std::is_same_v<T, double>) return atof(buf);
    if constexpr (std::is_same_v<T, long long>) return atoll(buf);
    if constexpr (std::is_same_v<T, int>) return atoi(buf);
    return 0;
}


int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "Usage: ./reconstruction <input_csv_path>\n";
        return 1;
    }

    // --- Optimization: Read entire file into memory at once ---
    std::ifstream fin(argv[1], std::ios::binary | std::ios::ate);
    if (!fin.is_open()) {
        std::cerr << "Error: Could not open input file " << argv[1] << "\n";
        return 1;
    }
    std::streamsize size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    std::string file_buffer(size, '\0');
    if (!fin.read(file_buffer.data(), size)) {
        std::cerr << "Error: Could not read file into buffer.\n";
        return 1;
    }
    fin.close();

    OrderBook book;
    std::stringstream output_buffer;

    output_buffer << "ts_event";
    for (int i = 0; i < 10; ++i) output_buffer << ",bid_price_" << i << ",bid_size_" << i;
    for (int i = 0; i < 10; ++i) output_buffer << ",ask_price_" << i << ",ask_size_" << i;
    output_buffer << "\n";

    // --- Optimization: Process the in-memory buffer ---
    std::string_view file_view(file_buffer);
    size_t start_pos = 0;
    
    // Skip header line
    size_t first_newline = file_view.find('\n');
    if (first_newline != std::string_view::npos) {
        start_pos = first_newline + 1;
    }

    bool is_first_event = true;

    while (start_pos < file_view.size()) {
        size_t end_pos = file_view.find('\n', start_pos);
        if (end_pos == std::string_view::npos) {
            end_pos = file_view.size();
        }
        std::string_view line = file_view.substr(start_pos, end_pos - start_pos);
        start_pos = end_pos + 1;

        if (line.empty()) continue;

        std::vector<std::string_view> buffer = splitString(line, ',');
        if (buffer.size() < 11) continue;

        std::string_view ts = buffer[1];
        std::string_view action = buffer[5];
        
        if (is_first_event && action == "R") {
            is_first_event = false;
            continue;
        }
        is_first_event = false;

        char side = buffer[6].empty() ? 'N' : buffer[6][0];
        long long oid = sv_to_num<long long>(buffer[10]);

        if (action == "R") {
            book.reset();
        } else if (action == "A") {
            double price = sv_to_num<double>(buffer[7]);
            int size = sv_to_num<int>(buffer[8]);
            book.addOrder(oid, price, size, side);
        } else if (action == "C") {
            book.cancelOrder(oid);
        } else if (action == "F") {
            int size = sv_to_num<int>(buffer[8]);
            book.fillOrder(oid, size);
        }
        
        book.writeSnapshot(output_buffer, ts);
    }

    std::ofstream fout("output/mbp_output.csv");
    if (!fout.is_open()) {
        std::cerr << "Error: Could not open output file output/mbp_output.csv. Make sure the 'output' directory exists.\n";
        return 1;
    }
    fout << output_buffer.rdbuf();
    fout.close();

    return 0;
}