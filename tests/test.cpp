#include <iostream>
#include <vector>
#include <assert.h>

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
//        std::cerr << row << "\n";
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

void test_pull() {
    std::cout << "pull" << std::endl;

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
    input.emplace_back("PULL,1");
    input.emplace_back("PULL,2");
    input.emplace_back("PULL,3");
    input.emplace_back("PULL,4");
    input.emplace_back("PULL,5");
    input.emplace_back("PULL,6");
    input.emplace_back("PULL,7");

    std::vector<std::string> result = run(input);
    assert(result.size() == 4);
    assert(result[0] == "WEBB,46,3,2,4");
    assert(result[1] == "WEBB,45.95,1,5,1");
    assert(result[2] == "WEBB,45.95,1,6,1");
    assert(result[3] == "WEBB,45.95,1,7,3");
}


void test_many_trades() {
    std::cout << "many trades" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    int count = 1000000;
    int sell_id = count + 1;
    for (int buy_id = 1; buy_id <= count; ++buy_id) {
        input.emplace_back("INSERT," + std::to_string(buy_id) + ",WEBB,BUY,45.95,10");
    }
    input.emplace_back("INSERT," + std::to_string(sell_id) + ",WEBB,SELL,45.95," + std::to_string(count * 10 + 1));


    std::vector<std::string> result = run(input);
    assert(result.size() == count + 2);
    for (int i = 0; i < count; ++i) {
        assert(result[i] == "WEBB,45.95,10," + std::to_string(sell_id) + "," + std::to_string(i + 1));
    }
    assert(result[count] == "===WEBB===");
    assert(result[count + 1] == ",,45.95,1");
}

void test_bad_queries() {
    std::cout << "bad queries" << std::endl;
    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,WEBB,BUY,10,5");
    input.emplace_back("INSERT,2,WEBB,SELL,20,6");
    input.emplace_back("AMEND,3,30,6");
    input.emplace_back("PULL,4");

    std::vector<std::string> result = run(input);
    assert(result.size() == 2);
    assert(result[0] == "===WEBB===");
    assert(result[1] == "10,5,20,6");
}


void test_alphabetical_order() {
    std::cout << "alphabetical order" << std::endl;
    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,C,BUY,10,5");
    input.emplace_back("INSERT,2,A,BUY,10,5");
    input.emplace_back("INSERT,3,B,BUY,10,5");
    input.emplace_back("INSERT,4,E,BUY,10,5");
    input.emplace_back("INSERT,5,D,BUY,10,5");

    std::vector<std::string> result = run(input);
    assert(result.size() == 10);
    assert(result[0] == "===A===");
    assert(result[1] == "10,5,,");
    assert(result[2] == "===B===");
    assert(result[3] == "10,5,,");
    assert(result[4] == "===C===");
    assert(result[5] == "10,5,,");
    assert(result[6] == "===D===");
    assert(result[7] == "10,5,,");
    assert(result[8] == "===E===");
    assert(result[9] == "10,5,,");
}

void test_no_commands() {
    std::cout << "no commands" << std::endl;
    std::vector<std::string> input = std::vector<std::string>();
    std::vector<std::string> result = run(input);
    assert(result.size() == 0);
}

void test_insert_2() {
    std::cout << "insert 2" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");

    std::vector<std::string> result = run(input);
    assert(result.size() == 2);
    assert(result[0] == "===AAPL===");
    assert(result[1] == "12.2,5,,");
}

void test_insert_pull_insert() {
    std::cout << "insert pull insert" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");
    input.emplace_back("PULL,1");
    input.emplace_back("INSERT,1,AAPL,BUY,12.2,5");

    std::vector<std::string> result = run(input);
    assert(result.size() == 0);
}


void test_amend_2() {
    std::cout << "insert pull insert" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,SELL,3,1");
    input.emplace_back("INSERT,2,A,SELL,3,1");
    input.emplace_back("INSERT,3,A,SELL,3,1");
    input.emplace_back("INSERT,4,A,BUY,1,4");
    input.emplace_back("AMEND,4,3,4");

    std::vector<std::string> result = run(input);
    assert(result.size() == 5);
    assert(result[0] == "A,3,1,4,1");
    assert(result[1] == "A,3,1,4,2");
    assert(result[2] == "A,3,1,4,3");
    assert(result[3] == "===A===");
    assert(result[4] == "3,1,,");
}

void test_amend_3() {
    std::cout << "insert pull insert" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,3,1");
    input.emplace_back("INSERT,2,A,BUY,3,1");
    input.emplace_back("INSERT,3,A,BUY,3,1");
    input.emplace_back("INSERT,4,A,SELL,5,4");
    input.emplace_back("AMEND,4,3,4");

    std::vector<std::string> result = run(input);
    assert(result.size() == 5);
    assert(result[0] == "A,3,1,4,1");
    assert(result[1] == "A,3,1,4,2");
    assert(result[2] == "A,3,1,4,3");
    assert(result[3] == "===A===");
    assert(result[4] == ",,3,1");
}

void test_simple_match_2() {
    std::cout << "simple match 2" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,3,1");
    input.emplace_back("INSERT,2,A,SELL,3,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 1);
    assert(result[0] == "A,3,1,2,1");
}

void test_simple_match_3() {
    std::cout << "simple match 3" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,3,1");
    input.emplace_back("INSERT,2,A,SELL,3,1");
    input.emplace_back("INSERT,4,B,BUY,3,1");
    input.emplace_back("INSERT,3,B,SELL,3,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 2);
    assert(result[0] == "A,3,1,2,1");
    assert(result[1] == "B,3,1,3,4");
}

void test_insert_3() {
    std::cout << "insert 3" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,3,1");
    input.emplace_back("INSERT,2,A,BUY,3,1");
    input.emplace_back("INSERT,3,A,BUY,3,1");
    input.emplace_back("INSERT,4,B,SELL,3,1");
    input.emplace_back("INSERT,5,B,SELL,3,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 4);
    assert(result[0] == "===A===");
    assert(result[1] == "3,3,,");
    assert(result[2] == "===B===");
    assert(result[3] == ",,3,2");
}


void test_insert_4() {
    std::cout << "insert 3" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,3,1");
    input.emplace_back("INSERT,2,A,BUY,3,1");
    input.emplace_back("INSERT,3,A,BUY,3,1");
    input.emplace_back("INSERT,4,A,BUY,4,10");
    input.emplace_back("INSERT,5,A,BUY,4,10");

    std::vector<std::string> result = run(input);
    assert(result.size() == 3);
    assert(result[0] == "===A===");
    assert(result[1] == "4,20,,");
    assert(result[2] == "3,3,,");
}

void test_insert_5() {
    std::cout << "insert 5" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,6,1");
    input.emplace_back("INSERT,2,A,BUY,5,1");
    input.emplace_back("INSERT,3,A,BUY,4,1");
    input.emplace_back("INSERT,4,A,BUY,3,1");
    input.emplace_back("INSERT,5,A,BUY,2,1");
    input.emplace_back("INSERT,6,A,BUY,1,1");
    input.emplace_back("INSERT,7,A,SELL,6,1");
    input.emplace_back("INSERT,8,A,SELL,5,1");
    input.emplace_back("INSERT,9,A,SELL,4,1");
    input.emplace_back("INSERT,10,A,SELL,3,1");
    input.emplace_back("INSERT,11,A,SELL,2,1");
    input.emplace_back("INSERT,12,A,SELL,1,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 6);
    assert(result[0] == "A,6,1,7,1");
    assert(result[1] == "A,5,1,8,2");
    assert(result[2] == "A,4,1,9,3");
    assert(result[3] == "A,3,1,10,4");
    assert(result[4] == "A,2,1,11,5");
    assert(result[5] == "A,1,1,12,6");
}

void test_insert_6() {
    std::cout << "insert 6" << std::endl;

    std::vector<std::string> input = std::vector<std::string>();
    input.emplace_back("INSERT,1,A,BUY,6,1");
    input.emplace_back("INSERT,2,A,BUY,5,1");
    input.emplace_back("INSERT,3,A,BUY,4,1");
    input.emplace_back("INSERT,4,A,BUY,3,1");
    input.emplace_back("INSERT,5,A,BUY,2,1");
    input.emplace_back("INSERT,6,A,BUY,1,1");
    input.emplace_back("INSERT,7,A,SELL,1,1");
    input.emplace_back("INSERT,8,A,SELL,2,1");
    input.emplace_back("INSERT,9,A,SELL,3,1");
    input.emplace_back("INSERT,10,A,SELL,4,1");
    input.emplace_back("INSERT,11,A,SELL,5,1");
    input.emplace_back("INSERT,12,A,SELL,6,1");

    std::vector<std::string> result = run(input);
    assert(result.size() == 7);
    assert(result[0] == "A,6,1,7,1");
    assert(result[1] == "A,5,1,8,2");
    assert(result[2] == "A,4,1,9,3");
    assert(result[3] == "===A===");
    assert(result[4] == "3,1,4,1");
    assert(result[5] == "2,1,5,1");
    assert(result[6] == "1,1,6,1");
}


int main() {
    test_insert();
    test_simple_match();
    test_multi_insert_multi_match();
    test_multi_symbol();
    test_amend();
    test_pull();
    test_bad_queries();
    test_alphabetical_order();
    test_no_commands();
    test_insert_2();
    test_insert_pull_insert();
    test_amend_2();
    test_amend_3();
    test_simple_match_2();
    test_simple_match_3();
    test_simple_match_3();
    test_insert_3();
    test_insert_4();
    test_insert_5();
    test_insert_6();

    test_many_trades();
    std::cout << "OK" << std::endl;
    return 0;
}