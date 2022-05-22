#include <iostream>
#include <string>
#include <sstream>
#include <set>
#include <vector>
#include <set>

class OrderType {
 public:
  virtual uint32_t execute() {
    return 0;
  };
};

// MasterOrder is OrderType
class MarketOrder : public OrderType {
 public:
  MarketOrder() = default;
  MarketOrder(const MarketOrder&) = default;
  uint32_t execute() final {
    return 0;
  };
};

// LimitOrder is OrderType
class LimitOrder : public OrderType {
 public:
  LimitOrder() = default;
  LimitOrder(const LimitOrder&) = default;
};

using OrderID = std::string;

enum class OrderSide { B, S };

std::istream& operator>>(std::istream& is, OrderSide& os) {
  std::string str;
  is >> str;
  if (str == "B") {
    os = OrderSide::B;
  } else if (str == "S") {
    os = OrderSide::S;
  } else {
    os = OrderSide::B;
  }
  return is;
}

enum class TheTypeOfOrder { Market, Limit };

// class Order that has OrderType as strategy and OrderID and Quantity
class Order {
  OrderType order_type_;
  OrderID id_;
  uint32_t quantity_;
  OrderSide side_;
  uint32_t price_;
  TheTypeOfOrder type_;

 public:
  Order() = default;
  Order(const Order&) = default;

  void set_order_type(OrderType&& order_type) {
    order_type_ = order_type;
  }

  OrderID get_id() const {
    return id_;
  }

  uint32_t get_quantity() const {
    return quantity_;
  }

  uint32_t get_price() const {
    return price_;
  }

  TheTypeOfOrder get_type() const {
    return type_;
  }

  void set_id(OrderID id) {
    id_ = id;
  }

  void set_quantity(uint32_t quantity) {
    quantity_ = quantity;
  }
  void set_side(OrderSide side) {
    side_ = side;
  }

  void set_price(uint32_t price) {
    price_ = price;
  }

  void set_type(TheTypeOfOrder type) {
    type_ = type;
  }

  uint32_t execute() {
    return order_type_.execute();
  }

  bool operator<(const Order& rhs) const {
    return price_ > rhs.price_;
  }
};

class OrderBuilder {
 private:
  Order order_;

 public:
  OrderBuilder() = default;
  OrderBuilder(Order& order) : order_(order) {}

  OrderBuilder& with_id(OrderID id) {
    order_.set_id(id);
    return *this;
  }

  OrderBuilder& with_quantity(uint32_t quantity) {
    order_.set_quantity(quantity);
    return *this;
  }

  OrderBuilder& with_side(OrderSide side) {
    order_.set_side(side);
    return *this;
  }

  OrderBuilder& with_price(uint32_t price) {
    order_.set_price(price);
    return *this;
  }

  OrderBuilder& with_type(TheTypeOfOrder type) {
    order_.set_type(type);
    return *this;
  }

  operator Order() const {
    return std::move(order_);
  }
};

enum class TokenType { SUB, CXL, END };

// overload operator from string to TokenType
std::istream& operator>>(std::istream& is, TokenType& token_type) {
  std::string token_type_str;
  is >> token_type_str;
  if (token_type_str == "SUB") {
    token_type = TokenType::SUB;
  } else if (token_type_str == "CXL") {
    token_type = TokenType::CXL;
  } else if (token_type_str == "END") {
    token_type = TokenType::END;
  } else {
    throw std::invalid_argument("Invalid token type");
  }
  return is;
};

enum class TokenOrderType { LO, MO };

std::istream& operator>>(std::istream& is, TokenOrderType& token_order_type) {
  std::string token_order_type_str;
  is >> token_order_type_str;
  if (token_order_type_str == "LO") {
    token_order_type = TokenOrderType::LO;
  } else if (token_order_type_str == "MO") {
    token_order_type = TokenOrderType::MO;
  } else {
    token_order_type = TokenOrderType::MO;
  }
  return is;
};

class Parser {
  std::string input_;
  TokenType token_type_;
  TokenOrderType token_order_type_;
  OrderSide token_side_;
  OrderID id_;
  uint32_t quantity_;
  uint32_t price_;
  Order order_;

 public:
  Parser() = default;
  Parser(std::string input) : input_(input) {}
  Parser(const Parser&) = default;

  TokenType parse(std::string input) {
    std::string token_quantity;
    std::string token_price;
    std::istringstream is(input);
    is >> token_type_;
    OrderBuilder ob(order_);
    switch (token_type_) {
      case TokenType::SUB:
        is >> token_order_type_;
        if (token_order_type_ == TokenOrderType::LO) {
          order_.set_order_type(std::move(LimitOrder()));
          order_.set_type(TheTypeOfOrder::Limit);
          price_ = 1;  // to check later that price should be set
        } else if (token_order_type_ == TokenOrderType::MO) {
          order_.set_order_type(std::move(MarketOrder()));
          order_.set_type(TheTypeOfOrder::Market);
          price_ = 0;
        } else {
          throw std::invalid_argument("Invalid token order type");
        }
        is >> token_side_;
        is >> id_;
        is >> token_quantity;
        quantity_ = std::stoi(token_quantity);
        if (price_ == 1) {
          is >> token_price;
          price_ = std::stoi(token_price);
        }
        order_.set_quantity(quantity_);
        order_.set_price(price_);
        order_.set_side(token_side_);
        order_.set_id(id_);
        return TokenType::SUB;
        break;
      case TokenType::CXL:
        is >> id_;
        order_.set_id(id_);
        return TokenType::CXL;
      case TokenType::END:
        return TokenType::END;
      default:
        return TokenType::END;
    }
  }

  Order get_order() {
    return order_;
  }

  OrderSide get_side() {
    return token_side_;
  }
};

class OrderBook {
  std::multiset<Order> buy_orders_;
  std::multiset<Order> sell_orders_;

 public:
  OrderBook() = default;
  OrderBook(const OrderBook&) = delete;

  uint32_t add_buy_order(Order order) {
    std::set<Order> to_erase;
    uint32_t result = 0;
    if (order.get_type() == TheTypeOfOrder::Limit) {
      for (auto sell_order : sell_orders_) {
        if (order.get_price() >= sell_order.get_price()) {
          if (order.get_quantity() >= sell_order.get_quantity()) {
            order.set_quantity(order.get_quantity() -
                               sell_order.get_quantity());
            result += sell_order.get_quantity() * sell_order.get_price();
            to_erase.insert(sell_order);
          } else {
            result += order.get_quantity() * sell_order.get_price();
            uint32_t q = order.get_quantity();
            sell_order.set_quantity(sell_order.get_quantity() -
                                    order.get_quantity());
            order.set_quantity(0);
            break;
          }
        }
      }
      if (order.get_quantity() != 0) {
        buy_orders_.insert(order);
      }
    } else if (order.get_type() == TheTypeOfOrder::Market) {
      for (auto sell_order : sell_orders_) {
        if (order.get_quantity() >= sell_order.get_quantity()) {
          order.set_quantity(order.get_quantity() - sell_order.get_quantity());
          result += sell_order.get_quantity() * sell_order.get_price();
          to_erase.insert(sell_order);
        } else {
          result += order.get_quantity() * sell_order.get_price();
          sell_order.set_quantity(sell_order.get_quantity() -
                                  order.get_quantity());
          order.set_quantity(0);
          break;
        }
      }
    }
    for (auto sell_order : to_erase) {
      sell_orders_.erase(sell_order);
    }
    return result;
  }

  uint32_t add_sell_order(Order order) {
    uint32_t result = 0;
    std::set<Order> to_erase;
    if (order.get_type() == TheTypeOfOrder::Limit) {
      for (auto buy_order : buy_orders_) {
        if (order.get_price() < buy_order.get_price()) {
          if (order.get_quantity() >= buy_order.get_quantity()) {
            order.set_quantity(order.get_quantity() - buy_order.get_quantity());
            result += buy_order.get_quantity() * buy_order.get_price();
            buy_order.set_quantity(0);
            to_erase.erase(buy_order);
          } else {
            result += order.get_quantity() * buy_order.get_price();
            buy_order.set_quantity(buy_order.get_quantity() -
                                   order.get_quantity());
            order.set_quantity(0);
            break;
          }
        }
      }
      if (order.get_quantity() != 0) {
        sell_orders_.insert(order);
      }
    } else if (order.get_type() == TheTypeOfOrder::Market) {
      for (auto buy_order : buy_orders_) {
        if (order.get_quantity() >= buy_order.get_quantity()) {
          order.set_quantity(order.get_quantity() - buy_order.get_quantity());
          result += buy_order.get_quantity() * buy_order.get_price();
          buy_order.set_quantity(0);
          to_erase.erase(buy_order);
        } else {
          result += order.get_quantity() * buy_order.get_price();
          buy_order.set_quantity(buy_order.get_quantity() -
                                 order.get_quantity());
          order.set_quantity(0);
          break;
        }
      }
    }

    for (auto buy_order : to_erase) {
      buy_orders_.erase(buy_order);
    }
    return result;
  }

  void remove_order(OrderID id) {
    for (auto it = buy_orders_.begin(); it != buy_orders_.end(); ++it) {
      if (it->get_id() == id) {
        buy_orders_.erase(it);
        return;
      }
    }
    for (auto it = sell_orders_.begin(); it != sell_orders_.end(); ++it) {
      if (it->get_id() == id) {
        sell_orders_.erase(it);
        return;
      }
    }
  }

  // get all orders
  std::string get_orders() {
    std::ostringstream order_str;
    order_str << "B:";
    for (auto order_it = buy_orders_.cbegin(); order_it != buy_orders_.cend();
         ++order_it) {
      order_str << " " << order_it->get_quantity() << std::string{"@"}
                << order_it->get_price() << std::string{"#"}
                << order_it->get_id();
    }
    order_str << "\nS:";
    for (auto order_it = sell_orders_.cbegin(); order_it != sell_orders_.cend();
         ++order_it) {
      order_str << " " << order_it->get_quantity() << std::string{"@"}
                << order_it->get_price() << std::string{"#"}
                << order_it->get_id();
    }

    return order_str.str();
  }
};

int main() {
  Parser parser;
  OrderBook order_book;
  for (std::string line; std::getline(std::cin, line);) {
    auto token = parser.parse(line);
    if (token == TokenType::SUB) {
      Order order = parser.get_order();
      if (parser.get_side() == OrderSide::B) {
        std::cout << order_book.add_buy_order(order) << std::endl;
        // std::cout << order_book.get_orders() << std::endl;
      } else if (parser.get_side() == OrderSide::S) {
        std::cout << order_book.add_sell_order(order) << std::endl;
      }
    } else if (token == TokenType::CXL) {
      order_book.remove_order(parser.get_order().get_id());
    } else if (token == TokenType::END) {
      std::cout << order_book.get_orders() << std::endl;
      break;
    }
  }

  return 0;
}
