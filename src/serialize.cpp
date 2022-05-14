#include "commands.hpp"
#include "serialize.hpp"

#include <vector>
#include <sstream>


std::vector<std::string> splitByChar(const std::string &string, char c);

std::shared_ptr<Insert> parseInsert(const std::vector<std::string> &insert_parts);

std::shared_ptr<Amend> parseAmend(const std::vector<std::string> &amend_parts);

std::shared_ptr<Pull> parsePull(const std::vector<std::string> &pull_parts);

/**
 * Every command starts with either "INSERT", "AMEND" or "PULL" with additional
 * data in the columns after the command.
 */
std::vector<std::shared_ptr<Command>> parseCommands(std::vector<std::string> const &input) {
    auto result = std::vector<std::shared_ptr<Command>>();

    for (std::string const &command_serialized : input) {
        auto command_parts = splitByChar(command_serialized, ',');
        if (command_parts.empty()) {
            throw std::runtime_error("invalid command");
        }
        if (command_parts[0] == "INSERT") {
            result.push_back(parseInsert(command_parts));
        } else if (command_parts[0] == "AMEND") {
            result.push_back(parseAmend(command_parts));
        } else if (command_parts[0] == "PULL") {
            result.push_back(parsePull(command_parts));
        } else {
            throw std::runtime_error("unknown command");
        }
    }
    return result;
}

std::vector<std::string> toString(std::vector<OrderBook> order_books) {

    std::vector<std::string> result = std::vector<std::string>();
    for (OrderBook const &order_book : order_books) {
        result.push_back("===" + order_book.symbol + "===");
        for (OrderBook::OrderBookItem const &info_item : order_book.rows) {
            std::stringstream stringstream;
            if (info_item.bid_price != OrderBook::OrderBookItem::NONE) {
                stringstream << (double) info_item.bid_price / 10000;
            }
            stringstream << ',';
            if (info_item.bid_volume != OrderBook::OrderBookItem::NONE) {
                stringstream << info_item.bid_volume;
            }
            stringstream << ',';
            if (info_item.ask_price != OrderBook::OrderBookItem::NONE) {
                stringstream << (double) info_item.ask_price / 10000;
            }
            stringstream << ',';
            if (info_item.ask_volume != OrderBook::OrderBookItem::NONE) {
                stringstream << info_item.ask_volume;
            }
            result.push_back(stringstream.str());
        }
    }
    return result;
}


/**
 * In case of insert the line will have the format:
 * INSERT,<order_id>,<symbol>,<side>,<price>,<volume>
 * e.g. INSERT,4,AAPL,BUY,23.45,12
 */
std::shared_ptr<Insert> parseInsert(const std::vector<std::string> &insert_parts) {
    if (insert_parts.size() != 6) {
        throw std::runtime_error("invalid insert");
    }
    OrderId order_id = std::stoi(insert_parts[1]);
    Symbol symbol = insert_parts[2];
    Side side;
    if (insert_parts[3] == "SELL") {
        side = Side::SELL;
    } else if (insert_parts[3] == "BUY") {
        side = Side::BUY;
    } else {
        throw std::runtime_error("invalid insert");
    }
    int price = (int) (std::stod(insert_parts[4]) * 10000);
    int volume = std::stoi(insert_parts[5]);
    return std::make_shared<Insert>(order_id, symbol, side, price, volume);
}

/**
 * In case of amend the line will have the format:
 * AMEND,<order_id>,<price>,<volume>
 * e.g. AMEND,4,23.12,11
 */
std::shared_ptr<Amend> parseAmend(const std::vector<std::string> &amend_parts) {
    if (amend_parts.size() != 4) {
        throw std::runtime_error("invalid amend");
    }
    OrderId order_id = std::stoi(amend_parts[1]);
    int price = (int) (std::stod(amend_parts[2]) * 10000);
    int volume = std::stoi(amend_parts[3]);
    return std::make_shared<Amend>(order_id, price, volume);
}

/**
 * In case of pull the line will have the format:
 * <PULL>,<order_id>
 * e.g. PULL,4
 */
std::shared_ptr<Pull> parsePull(const std::vector<std::string> &pull_parts) {
    if (pull_parts.size() != 2) {
        throw std::runtime_error("invalid pull");
    }
    OrderId order_id = std::stoi(pull_parts[1]);
    return std::make_shared<Pull>(order_id);
}


std::vector<std::string> splitByChar(const std::string &string, char c) {
    std::stringstream stream = std::stringstream(string);
    auto result = std::vector<std::string>();
    std::string segment;
    while (std::getline(stream, segment, c)) {
        result.push_back(segment);
    }
    return result;
}
