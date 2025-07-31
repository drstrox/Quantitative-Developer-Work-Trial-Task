#include <bits/stdc++.h>

// Using standard namespace for convenience
using namespace std;

/**
 * @struct Order
 * @brief Represents a single order in the order book.
 */
struct Order {
    double price;
    int size;
    char side;
};

// Global data structures to maintain the state of the order book.
// Using unordered_map for fast O(1) average time complexity for order lookups by ID.
unordered_map<long long, Order> order_map;

// Using map for the bid/ask books to keep them sorted by price.
// Bids are sorted high-to-low, asks are sorted low-to-high.
map<double, int, greater<double>> bid_book;
map<double, int> ask_book;

/**
 * @brief Updates the size of a price level in the bid or ask book.
 * @param side The side of the book to update ('B' for Bid, 'A' for Ask).
 * @param price The price level to update.
 * @param size_diff The change in size (can be positive or negative).
 */
void updateBook(char side, double price, int size_diff) {
    if (side == 'B') {
        bid_book[price] += size_diff;
        if (bid_book[price] <= 0) {
            bid_book.erase(price);
        }
    } else if (side == 'A') {
        ask_book[price] += size_diff;
        if (ask_book[price] <= 0) {
            ask_book.erase(price);
        }
    }
}

/**
 * @brief Writes a snapshot of the top 10 levels of the order book to an output stream.
 * @param oss The output string stream.
 * @param ts The timestamp for the current event.
 */
void writeSnapshot(stringstream& oss, string_view ts) {
    oss << ts;

    // Write top 10 bid levels
    int count = 0;
    auto it_bid = bid_book.begin();
    while (it_bid != bid_book.end() && count < 10) {
        oss << "," << fixed << setprecision(2) << it_bid->first << "," << it_bid->second;
        ++it_bid;
        ++count;
    }
    // Pad with empty columns to match sample format
    for (int i = count; i < 10; ++i) {
        oss << ",,";
    }

    // Write top 10 ask levels
    count = 0;
    auto it_ask = ask_book.begin();
    while (it_ask != ask_book.end() && count < 10) {
        oss << "," << fixed << setprecision(2) << it_ask->first << "," << it_ask->second;
        ++it_ask;
        ++count;
    }
    // Pad with empty columns to match sample format
    for (int i = count; i < 10; ++i) {
        oss << ",,";
    }

    oss << "\n";
}

/**
 * @brief A fast, lightweight CSV string splitter using string_view.
 * @param s The string to split.
 * @param delimiter The character to split by.
 * @return A vector of string_views, avoiding string copies.
 */
vector<string_view> splitString(string_view s, char delimiter) {
    vector<string_view> tokens;
    size_t start = 0;
    size_t end = s.find(delimiter);
    while (end != string_view::npos) {
        tokens.push_back(s.substr(start, end - start));
        start = end + 1;
        end = s.find(delimiter, start);
    }
    tokens.push_back(s.substr(start));
    return tokens;
}

/**
 * @brief Main function to process MBO data and reconstruct the MBP-10 book.
 */
int main(int argc, char* argv[]) {
    // --- Optimization: Disable synchronization with C-style I/O ---
    ios_base::sync_with_stdio(false);

    if (argc < 2) {
        cerr << "Usage: ./reconstruction <input_csv_path>\n";
        return 1;
    }

    ifstream fin(argv[1]);
    if (!fin.is_open()) {
        cerr << "Error: Could not open input file " << argv[1] << "\n";
        return 1;
    }

    // --- Optimization: Buffer output in a stringstream ---
    stringstream output_buffer;

    // Write the header to the buffer.
    output_buffer << "ts_event";
    for (int i = 0; i < 10; ++i) output_buffer << ",bid_price_" << i << ",bid_size_" << i;
    for (int i = 0; i < 10; ++i) output_buffer << ",ask_price_" << i << ",ask_size_" << i;
    output_buffer << "\n";

    string line;
    getline(fin, line); // Skip header

    bool is_first_event = true;

    while (getline(fin, line)) {
        // --- Optimization: Use fast manual parser ---
        vector<string_view> buffer = splitString(line, ',');

        if (buffer.size() < 11) continue;

        string_view ts = buffer[1];
        string_view action = buffer[5];
        
        if (is_first_event && action == "R") {
            is_first_event = false;
            continue;
        }
        is_first_event = false;

        char side = buffer[6].empty() ? 'N' : buffer[6][0];
        long long oid = 0;
        if (!buffer[10].empty()) {
            // Reverted to stoll for better compiler compatibility.
            oid = stoll(string(buffer[10]));
        }

        if (action == "R") {
            order_map.clear();
            bid_book.clear();
            ask_book.clear();
        } else if (action == "A") {
            double price = 0.0;
            int size = 0;
            if (!buffer[7].empty()) price = stod(string(buffer[7]));
            if (!buffer[8].empty()) size = stoi(string(buffer[8]));
            
            if (oid != 0 && size > 0) {
                order_map[oid] = {price, size, side};
                updateBook(side, price, size);
            }
        } else if (action == "C") {
            if (order_map.count(oid)) {
                Order& ord = order_map.at(oid);
                updateBook(ord.side, ord.price, -ord.size);
                order_map.erase(oid);
            }
        } else if (action == "F") {
            int size = 0;
            if (!buffer[8].empty()) size = stoi(string(buffer[8]));

            if (order_map.count(oid) && size > 0) {
                Order& ord = order_map.at(oid);
                updateBook(ord.side, ord.price, -size);
                ord.size -= size;
                if (ord.size <= 0) {
                    order_map.erase(oid);
                }
            }
        }
        
        writeSnapshot(output_buffer, ts);
    }
    fin.close();

    // --- Optimization: Write the entire buffer to file at once ---
    ofstream fout("output/mbp_output.csv");
    if (!fout.is_open()) {
        cerr << "Error: Could not open output file output/mbp_output.csv. Make sure the 'output' directory exists.\n";
        return 1;
    }
    fout << output_buffer.rdbuf();
    fout.close();

    return 0;
}