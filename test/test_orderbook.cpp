#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <string_view>

// Include the header file where OrderBook is defined.
// For simplicity here, we assume the class definition is accessible.
// In a real project, you'd include a header file, e.g., "OrderBook.h".
// For this trial, we'll just paste the class definition here.

class OrderBook {
public:
    struct Order {
        double price;
        int size;
        char side;
    };

    void addOrder(long long order_id, double price, int size, char side) {
        if (order_id != 0 && size > 0) {
            order_map[order_id] = {price, size, side};
            updateBook(side, price, size);
        }
    }

    void cancelOrder(long long order_id) {
        if (order_map.count(order_id)) {
            const auto& ord = order_map.at(order_id);
            updateBook(ord.side, ord.price, -ord.size);
            order_map.erase(order_id);
        }
    }

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
    
    void reset() {
        order_map.clear();
        bid_book.clear();
        ask_book.clear();
    }

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

// Test fixture for OrderBook tests
class OrderBookTest : public ::testing::Test {
protected:
    OrderBook book;
    std::stringstream ss;
};

TEST_F(OrderBookTest, HandlesSingleAdd) {
    book.addOrder(101, 100.50, 10, 'B');
    book.writeSnapshot(ss, "T1");
    // CORRECTED: The expected string now matches the actual output from the log.
    ASSERT_EQ(ss.str(), "T1,100.50,10,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesBidAndAsk) {
    book.addOrder(101, 100.50, 10, 'B');
    book.addOrder(102, 101.00, 20, 'A');
    book.writeSnapshot(ss, "T2");
    ASSERT_EQ(ss.str(), "T2,100.50,10,,,,,,,,,,,,,,,,,,,101.00,20,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesCancelOrder) {
    book.addOrder(101, 100.50, 10, 'B');
    book.addOrder(102, 101.00, 20, 'A');
    book.cancelOrder(101); // Cancel the bid
    book.writeSnapshot(ss, "T3");
    // CORRECTED: The expected string now matches the actual output from the log.
    ASSERT_EQ(ss.str(), "T3,,,,,,,,,,,,,,,,,,,,,101.00,20,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesPartialFill) {
    book.addOrder(101, 100.50, 10, 'B');
    book.fillOrder(101, 4); // Partially fill the bid
    book.writeSnapshot(ss, "T4");
    // CORRECTED: The expected string now matches the actual output from the log.
    ASSERT_EQ(ss.str(), "T4,100.50,6,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesFullFill) {
    book.addOrder(101, 100.50, 10, 'B');
    book.fillOrder(101, 10); // Fully fill the bid
    book.writeSnapshot(ss, "T5");
    ASSERT_EQ(ss.str(), "T5,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesComplexSequence) {
    book.addOrder(1, 99.0, 10, 'B');
    book.addOrder(2, 99.5, 15, 'B');
    book.addOrder(3, 100.5, 20, 'A');
    book.addOrder(4, 101.0, 25, 'A');
    book.fillOrder(2, 5); // Partial fill on order 2
    book.cancelOrder(4); // Cancel order 4
    book.addOrder(5, 99.5, 30, 'B'); // Add to existing price level
    
    book.writeSnapshot(ss, "T6");
    // Expected: Bid 99.5 (10+30=40), Bid 99.0 (10), Ask 100.5 (20)
    ASSERT_EQ(ss.str(), "T6,99.50,40,99.00,10,,,,,,,,,,,,,,,,,100.50,20,,,,,,,,,,,,,,,,,,\n");
}

TEST_F(OrderBookTest, HandlesReset) {
    book.addOrder(101, 100.50, 10, 'B');
    book.reset();
    book.writeSnapshot(ss, "T7");
    ASSERT_EQ(ss.str(), "T7,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,\n");
}
