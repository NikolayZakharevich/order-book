#include <iostream>
#include <vector>

#include "../src/engine.hpp"
#include "../src/serialize.hpp"

std::vector<std::string> run(std::vector<std::string> const &input) {
    CLOBEngine engine = CLOBEngine();

    std::vector<std::shared_ptr<Command>> commands = parseCommands(input);
    for (auto const &command : commands) {
        command->accept(&engine);
    }

    std::vector<std::string> result = toString(engine.getTrades(), engine.getOrderBooks());
    for (const auto &row : result) {
        std::cerr << row << "\n";
    }
    return result;
}


void test_insert() {
    std::cout << "insert" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");

    std::vector<std::string> result = run(input);
    assert(result.size() == 2);
    assert(result[0] == "===AAPL===");
    assert(result[1] == "12.2,5,,");
}

void test_simple_match() {
    std::cout << "simple match" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");
    input.emplace_back("INSERT,2,AAPL,SELL,12.1,8");

    std::vector<std::string> result = run(input);
    assert(result.size() == 3);
    assert(result[0] == "AAPL,12.2,5,2,1");
    assert(result[1] == "===AAPL===");
    assert(result[2] == ",,12.1,3");
}

void test_multi_insert_multi_match() {
    std::cout << "multi insert and multi match" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,AAPL,BUY,14.235,5");
    input.emplace_back("INSERT,2,AAPL,BUY,14.235,6");
    input.emplace_back("INSERT,3,AAPL,BUY,14.235,12");
    input.emplace_back("INSERT,4,AAPL,BUY,14.234,5");
    input.emplace_back("INSERT,5,AAPL,BUY,14.23,3");
    input.emplace_back("INSERT,6,AAPL,SELL,14.237,8");
    input.emplace_back("INSERT,7,AAPL,SELL,14.24,9");
    input.emplace_back("PULL,1");
    input.emplace_back("INSERT,8,AAPL,SELL,14.234,25");

    std::vector<std::string> result = run(input);
    assert(result.size() == 7);
    assert(result[0] == "AAPL,14.235,6,8,2");
    assert(result[1] == "AAPL,14.235,12,8,3");
    assert(result[2] == "AAPL,14.234,5,8,4");
    assert(result[3] == "===AAPL===");
    assert(result[4] == "14.23,3,14.234,2");
    assert(result[5] == ",,14.237,8");
    assert(result[6] == ",,14.24,9");
};


void test_multi_symbol() {
    std::cout << "multi symbol" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,WEBB,BUY,0.3854,5");
    input.emplace_back("INSERT,2,TSLA,BUY,412,31");
    input.emplace_back("INSERT,3,TSLA,BUY,410.5,27");
    input.emplace_back("INSERT,4,AAPL,SELL,21,8");
    input.emplace_back("INSERT,11,WEBB,SELL,0.3854,4");
    input.emplace_back("INSERT,13,WEBB,SELL,0.3853,6");

    std::vector<std::string> result = run(input);
    assert(result.size() == 9);
    assert(result[0] == "WEBB,0.3854,4,11,1");
    assert(result[1] == "WEBB,0.3854,1,13,1");
    assert(result[2] == "===AAPL===");
    assert(result[3] == ",,21,8");
    assert(result[4] == "===TSLA===");
    assert(result[5] == "412,31,,");
    assert(result[6] == "410.5,27,,");
    assert(result[7] == "===WEBB===");
    assert(result[8] == ",,0.3853,5");
}


void test_amend() {
    std::cout << "amend" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,WEBB,BUY,45.95,5");
    input.emplace_back("INSERT,2,WEBB,BUY,45.95,6");
    input.emplace_back("INSERT,3,WEBB,BUY,45.95,12");
    input.emplace_back("INSERT,4,WEBB,SELL,46,8");
    input.emplace_back("AMEND,2,46,3");
    input.emplace_back("INSERT,5,WEBB,SELL,45.95,1");
    input.emplace_back("AMEND,1,45.95,3");
    input.emplace_back("INSERT,6,WEBB,SELL,45.95,1");
    input.emplace_back("AMEND,1,45.95,5");
    input.emplace_back("INSERT,7,WEBB,SELL,45.95,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 6);
    assert(result[0] == "WEBB,46,3,2,4");
    assert(result[1] == "WEBB,45.95,1,5,1");
    assert(result[2] == "WEBB,45.95,1,6,1");
    assert(result[3] == "WEBB,45.95,1,7,3");
    assert(result[4] == "===WEBB===");
    assert(result[5] == "45.95,16,46,5");
}

int main() {
    test_insert();
    test_simple_match();
    test_multi_insert_multi_match();
    test_multi_symbol();
    test_amend();
    return 0;
}