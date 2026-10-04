#include <iostream>
#include <string>
#include <vector>
#include "clase.h"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <variant>
#include <memory>
#include <utility>
#include <chrono>
#include <unordered_set>
#include <optional>

using namespace std;
ofstream terminal("output.txt");

struct ExecutionResult
{
    bool did_return = false;
    bool break_state = false;
    bool continue_state = false;
    std::optional<Value> value = std::nullopt;
};

unordered_set<const ListValue *> error_list;

void print_value(const Value &value, ostream &out)
{
    if (holds_alternative<BigInt>(value))
    {
        out << get<BigInt>(value);
    }
    else if (holds_alternative<bool>(value))
    {
        out << (get<bool>(value) ? "true" : "false");
    }
    else if (holds_alternative<string>(value))
    {
        out << get<string>(value);
    }
    else if (holds_alternative<shared_ptr<ListValue>>(value))
    {

        auto list = get<shared_ptr<ListValue>>(value);
        auto pointer_to_list = list.get();
        error_list.insert(pointer_to_list);
        out << "[";

        for (size_t i = 0; i < list->elements.size(); i++)
        {
            if (i > 0)
                out << ", ";
            if (holds_alternative<shared_ptr<ListValue>>(list->elements[i]) && error_list.count(get<shared_ptr<ListValue>>(list->elements[i]).get()))
            {
                cerr << "Eroare lista care se contine pe sine nu se poate afisa " << endl;
                return;
            }
            if (holds_alternative<shared_ptr<ListValue>>(list->elements[i]))
            {
                print_value(list->elements[i], out);
                error_list.erase(get<shared_ptr<ListValue>>(list->elements[i]).get());
            }
            else
            {
                print_value(list->elements[i], out);
            }
        }
        error_list.erase(pointer_to_list);

        out << "]";
    }
    else if (holds_alternative<shared_ptr<DictionaryValue>>(value))
    {
        auto dict = get<shared_ptr<DictionaryValue>>(value);
        out << "{";
        for (auto p = dict->map.begin(); p != dict->map.end(); p++)
        {
            if (p != dict->map.begin())
                out << " , ";
            print_value(p->first, out);
            out << " : ";
            print_value(p->second, out);
        }
        out << "}";
    }
}
bool turn_boolean(Value x)
{
    if (holds_alternative<BigInt>(x))
        return get<BigInt>(x) != 0;

    if (holds_alternative<string>(x))
    {
        string s = get<string>(x);
        return !s.empty();
    }
    if (holds_alternative<shared_ptr<ListValue>>(x))
    {
        auto list = get<shared_ptr<ListValue>>(x);
        return !list->elements.empty();
    }
    if (holds_alternative<shared_ptr<DictionaryValue>>(x))
    {
        auto dict = get<shared_ptr<DictionaryValue>>(x);
        return !dict->map.empty();
    }

    return get<bool>(x);
}

bool value_equals(Value &a, Value &b)
{
    if (holds_alternative<string>(a) && holds_alternative<string>(b))
        return get<string>(a) == get<string>(b);
    if (holds_alternative<BigInt>(a) && holds_alternative<BigInt>(b))
        return get<BigInt>(a) == get<BigInt>(b);
    if (holds_alternative<bool>(a) && holds_alternative<bool>(b))
        return get<bool>(a) == get<bool>(b);

    if (holds_alternative<shared_ptr<ListValue>>(a) && holds_alternative<shared_ptr<ListValue>>(b))
    {
        auto l1 = get<shared_ptr<ListValue>>(a);
        auto l2 = get<shared_ptr<ListValue>>(b);
        if (l1->elements.size() != l2->elements.size())
            return false;
        for (size_t i = 0; i < l1->elements.size(); i++)
            if (!value_equals(l1->elements[i], l2->elements[i]))
                return false;

        return true;
    }

    if (holds_alternative<shared_ptr<DictionaryValue>>(a) && holds_alternative<shared_ptr<DictionaryValue>>(b))
    {
        auto d1 = get<shared_ptr<DictionaryValue>>(a);
        auto d2 = get<shared_ptr<DictionaryValue>>(b);
        if (d1->map.size() != d2->map.size())
            return false;

        for (auto p : d1->map)
            if (!d2->map.count(p.first) || !value_equals(p.second, d2->map[p.first]))
                return false;

        return true;
    }

    return false;
}

Value copy_value(Value &p)
{
    if (holds_alternative<BigInt>(p))
    {
        Value ans = BigInt(get<BigInt>(p));
        return ans;
    }
    if (holds_alternative<string>(p))
    {
        string s = "";
        string p_string = get<string>(p);
        for (size_t i = 0; i < p_string.size(); i++)
            s += p_string[i];
        return Value{s};
    }
    if (holds_alternative<bool>(p))
    {
        bool new_p = get<bool>(p);
        return Value{new_p};
    }
    if (holds_alternative<shared_ptr<ListValue>>(p))
    {
        auto list = get<shared_ptr<ListValue>>(p);
        auto pointer_to_list = list.get();
        auto new_list = make_shared<ListValue>();
        for (size_t i = 0; i < list->elements.size(); i++)
        {
            if (holds_alternative<shared_ptr<ListValue>>(list->elements[i]) &&
                pointer_to_list == get<shared_ptr<ListValue>>(list->elements[i]).get())
            {
                cerr << "Eroare lista care se contine pe sine nu se poate copia" << endl;
                return Value{false};
            }
            Value cp = copy_value(list->elements[i]);
            new_list->elements.push_back(cp);
        }
        return Value{new_list};
    }
    if (holds_alternative<shared_ptr<DictionaryValue>>(p))
    {
        auto dict = get<shared_ptr<DictionaryValue>>(p);
        auto mp = make_shared<DictionaryValue>();
        for (auto indx : dict->map)
        {
            string key = indx.first;
            Value val = copy_value(indx.second);
            mp->map[key] = val;
        }
        return Value{mp};
    }
    return Value{false};
}

string value_to_string(const Value &value)
{
    ostringstream output;
    print_value(value, output);
    return output.str();
}

vector<string> tokenizer(string line)
{

    vector<string> ans;

    string stack = "";

    bool expected_string = false;

    for (size_t indx = 0; indx < line.size(); indx++)
    {

        if (indx < line.size() && expected_string && line[indx] != '"')
        {
            stack += line[indx];
            continue;
        }

        if (line[indx] == '"' && !expected_string)
        {
            expected_string = true;
            stack += '"';
            continue;
        }
        else if (line[indx] == '"' && expected_string)
        {
            stack += '"';
            ans.push_back(stack);
            stack = "";
            expected_string = false;
            continue;
        }
        char ch = line[indx];

        if (ch == '[' || ch == ']' || ch == ',' || ch == '.' || ch == '{' || ch == '}' || ch == ':')
        {
            if (!stack.empty())
                ans.push_back(stack);
            stack = "";
            stack += ch;
            ans.push_back(stack);
            stack = "";
            continue;
        }
        if (ch == '+' || ch == '-' || ch == '*' || ch == '/' || ch == '%' || ch == '(' || ch == ')')
        {
            if (!stack.empty())
                ans.push_back(stack);
            stack = ch;
            ans.push_back(stack);
            stack = "";
            continue;
        }
        if (ch == '=')
        {
            if (indx < line.size() - 1 && line[indx + 1] == '=')
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back("==");
                stack = "";
                indx++;
                continue;
            }
            else
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back("=");
                stack = "";
                continue;
            }
        }
        if (ch == '!' && indx < line.size() - 1 && line[indx + 1] == '=')
        {
            if (!stack.empty())
                ans.push_back(stack);
            ans.push_back("!=");
            stack = "";
            indx++;
            continue;
        }
        else if (ch == '!' && indx < line.size() - 1 && line[indx + 1] != '=')
        {
            if (!stack.empty())
                ans.push_back(stack);
            ans.push_back("!");
            stack = "";
            continue;
        }

        if (ch == '>')
        {
            if (indx < line.size() - 1 && line[indx + 1] == '=')
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back(">=");
                indx++;
                stack = "";
                continue;
            }
            else
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back(">");
                stack = "";
                continue;
            }
        }
        if (ch == '<')
        {
            if (indx < line.size() - 1 && line[indx + 1] == '=')
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back("<=");
                indx++;
                stack = "";
                continue;
            }
            else
            {
                if (!stack.empty())
                    ans.push_back(stack);
                ans.push_back("<");
                stack = "";
                continue;
            }
        }

        if (line[indx] == ' ')
        {
            if (!stack.empty())
                ans.push_back(stack);
            stack = "";
        }
        else
            stack += ch;
    }
    if (!stack.empty())
        ans.push_back(stack);
    return ans;
}

unordered_map<string, Value> variables;
unordered_map<string, FunctionStatement *> functions;

bool is_comparison(TokenType p)
{

    return p == TokenType::EQUALS || p == TokenType::NOT_EQUALS || p == TokenType::LESS_THAN || p == TokenType::LESSEQQ_THAN || p == TokenType::GRATER_THAN || p == TokenType::GRETAREQQ_THAN;
}

Value get_value(Token p)
{
    Value ans;
    if (p.get_type() == TokenType::NUMBER)
        ans = BigInt(p.get_original_text());
    else if (p.get_type() == TokenType::STRING)
        ans = p.get_original_text();
    else if (p.get_type() == TokenType::IDENTIFIER)
    {
        if (!variables.count(p.get_original_text()))
        {
            cerr << "Variabila inexistenta la linia " << p.get_line_number() << endl;
            ans = "NaN";
            return ans;
        }
        else
            ans = variables[p.get_original_text()];
    }
    return ans;
}

unique_ptr<Expression> parse_comparison(vector<Token> &line_tokens, size_t &current);
unique_ptr<Expression> parse_primary(vector<Token> &line_tokens, size_t &current);
unique_ptr<Expression> parse_or(vector<Token> &line_tokens, size_t &current);
unique_ptr<Expression> parse_postfix(vector<Token> &line_tokens, size_t &current)
{
    if (current >= line_tokens.size())
        return nullptr;

    auto expression = parse_primary(line_tokens, current);
    if (!expression)
        return nullptr;
    while (current < line_tokens.size())
    {
        if (line_tokens[current].get_type() == TokenType::OPEN_LIST)
        {
            current++;
            auto indx = parse_or(line_tokens, current);
            if (!indx)
            {
                cerr << "Eroare de indexare " << endl;
                return nullptr;
            }
            if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::CLOSED_LIST)
            {
                cerr << "Lipsa ] " << endl;
                return nullptr;
            }
            current++;
            expression = make_unique<IndexExpression>(move(expression), move(indx));
        }
        else if (line_tokens[current].get_type() == TokenType::DOT)
        {
            current++;
            if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::IDENTIFIER)
            {
                cerr << "Eroare methoda necunoscuta ";
                return nullptr;
            }

            string method_name = line_tokens[current].get_original_text();
            current++;
            if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::OPEN_NORMAL)
            {
                cerr << "Metoda Necunoscuta " << endl;
                return nullptr;
            }
            current++;
            vector<unique_ptr<Expression>> arguments;
            while (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
            {

                auto argument = parse_or(line_tokens, current);

                if (!argument)
                    return nullptr;

                if (current >= line_tokens.size())
                {
                    cerr << "Eroare Lipsa ) " << endl;
                    return nullptr;
                }
                arguments.push_back(move(argument));
                if (line_tokens[current].get_type() == TokenType::COMMA)
                {
                    current++;
                    if (current >= line_tokens.size() || line_tokens[current].get_type() == TokenType::CLOSED_NORMAL)
                    {
                        cerr << "Lipseste argument dupa virgula " << endl;
                        return nullptr;
                    }
                    continue;
                }
                if (line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
                {
                    cerr << "Lipseste virgula intre argumente" << endl;
                    return nullptr;
                }
            }

            if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
            {
                cerr << "Lipseste )" << endl;
                return nullptr;
            }
            current++;
            expression = make_unique<MethodCallExpression>(move(expression), method_name, move(arguments));
        }
        else if (line_tokens[current].get_type() == TokenType::OPEN_NORMAL)
        {
            auto function_name_expression = dynamic_cast<VariableExpression *>(expression.get());

            if (!function_name_expression)
            {
                cerr << "Expresia nu poate fii apelata ca functie " << endl;
                return nullptr;
            }
            string function_name = function_name_expression->name;

            current++;
            vector<unique_ptr<Expression>> args;
            while (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
            {
                auto arg = parse_or(line_tokens, current);
                if (!arg)
                {
                    cerr << "Argument prost " << endl;
                    return nullptr;
                }
                args.push_back(move(arg));

                if (current >= line_tokens.size())
                {
                    cerr << "Lipseste )" << endl;
                    return nullptr;
                }

                if (line_tokens[current].get_type() == TokenType::COMMA)
                {
                    current++;

                    if (current >= line_tokens.size() || line_tokens[current].get_type() == TokenType::CLOSED_NORMAL)
                    {
                        cerr << "Lipseste argumentul dupa virgula" << endl;
                        return nullptr;
                    }

                    continue;
                }

                if (line_tokens[current].get_type() == TokenType::CLOSED_NORMAL)
                {
                    break;
                }

                cerr << "Lipseste virgula intre argumente" << endl;
                return nullptr;
            }
            if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
            {
                cerr << "Lipsa )" << endl;
                return nullptr;
            }
            current++;
            expression = make_unique<FunctionCallExpression>(function_name, move(args));
        }
        else
            break;
    }
    return expression;
}
unique_ptr<Expression> parse_unary(vector<Token> &line_tokens, size_t &current)
{
    if (current >= line_tokens.size())
    {
        return nullptr;
    }
    if (line_tokens[current].get_type() == TokenType::MINUS)
    {
        TokenType op = line_tokens[current].get_type();
        current++;
        auto right = parse_unary(line_tokens, current);
        if (right == nullptr)
        {
            cerr << "Minus necompletat la linia " << line_tokens[current - 1].get_line_number() << endl;
            return nullptr;
        }
        return make_unique<UnaryExpression>(op, move(right));
    }
    else if (line_tokens[current].get_type() == TokenType::NOT)
    {
        TokenType op = TokenType::NOT;
        current++;
        auto right = parse_unary(line_tokens, current);
        if (right == nullptr)
        {
            cerr << "Not necompletat la lina " << line_tokens[current - 1].get_line_number() << endl;
            return nullptr;
        }
        return make_unique<UnaryExpression>(op, move(right));
    }
    return parse_postfix(line_tokens, current);
}

unique_ptr<Expression> parse_primary(vector<Token> &line_tokens, size_t &current)
{
    if (current >= line_tokens.size())
    {
        return nullptr;
    }

    if (line_tokens[current].get_type() == TokenType::OPEN_SWIRLY)
    {
        vector<unique_ptr<Expression>> keys, vals;

        current++;
        if (current >= line_tokens.size())
            return nullptr;
        if (line_tokens[current].get_type() == TokenType::CLOSED_SWIRLY)
        {
            current++;
            return make_unique<MapExpression>(move(keys), move(vals));
        }
        if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::COMMA)
        {
            cerr << "Eroare initializare lista la lina " << line_tokens[current].get_line_number() << endl;
            return nullptr;
        }
        while (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::CLOSED_SWIRLY)
        {
            if (current < line_tokens.size() && (line_tokens[current].get_type() == TokenType::COMMA || line_tokens[current].get_type() == TokenType::DOUBLE_DOTS))
            {
                cerr << "Eroare elemente lipsa la lina " << line_tokens[current].get_line_number() << endl;
                return nullptr;
            }

            auto key = parse_or(line_tokens, current);
            if (!key)
            {
                cerr << "Eroare dictionar" << endl;
                return nullptr;
            }
            if (current >= line_tokens.size() || (line_tokens[current].get_type() != TokenType::DOUBLE_DOTS || line_tokens[current].get_type() == TokenType::COMMA || line_tokens[current].get_type() == TokenType::END_OF_LINE))
            {
                cerr << "Eroare dictionar" << endl;
                return nullptr;
            }
            current++;
            auto val = parse_or(line_tokens, current);
            if (!val)
            {
                cerr << "Eroare dictionar" << endl;
                return nullptr;
            }
            keys.push_back(move(key));
            vals.push_back(move(val));
            if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::CLOSED_SWIRLY)
            {
                current++;
                return make_unique<MapExpression>(move(keys), move(vals));
            }

            if (current >= line_tokens.size() || (line_tokens[current].get_type() != TokenType::COMMA))
            {
                cerr << "Eroare dictionar" << endl;
                return nullptr;
            }
            if (current + 1 > line_tokens.size())
            {
                cerr << "Eroare dictionar" << endl;
                return nullptr;
            }
            current++;
        }
        return nullptr;
    }

    if (line_tokens[current].get_type() == TokenType::OPEN_LIST)
    {
        vector<unique_ptr<Expression>> elements;
        current++;
        if (current >= line_tokens.size())
            return nullptr;
        if (line_tokens[current].get_type() == TokenType::CLOSED_LIST)
        {
            current++;
            return make_unique<ListExpression>(move(elements));
        }
        if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::COMMA)
        {
            cerr << "Virgula eronata la linia " << line_tokens[current].get_line_number() << endl;
            return nullptr;
        }
        while (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::CLOSED_LIST)
        {
            if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::COMMA)
            {
                cerr << "Lipseste numar intre virgule " << endl;
                return nullptr;
            }
            auto left = parse_or(line_tokens, current);

            if (!left)
            {
                cerr << "Lista gresita " << endl;
                return nullptr;
            }
            elements.push_back(move(left));

            if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::CLOSED_LIST)
            {
                current++;
                return make_unique<ListExpression>(move(elements));
            }

            if (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::COMMA)
            {
                cerr << "virgula lipsa " << endl;
                return nullptr;
            }
            if (current + 1 >= line_tokens.size())
            {
                cerr << "Eroare" << endl;
                return nullptr;
            }
            if (line_tokens[current + 1].get_type() == TokenType::CLOSED_LIST)
            {
                cerr << "Eroare de listare" << endl;
                return nullptr;
            }
            current++;
        }
        return nullptr;
    }

    if (line_tokens[current].get_type() == TokenType::TRUE)
    {
        Value ans = true;
        current++;
        return make_unique<LiteralExpression>(ans);
    }
    if (line_tokens[current].get_type() == TokenType::FALSE)
    {
        Value ans = false;
        current++;
        return make_unique<LiteralExpression>(ans);
    }

    if (line_tokens[current].get_type() == TokenType::ENDL)
    {
        Value val = "\n";
        auto final_ans = make_unique<LiteralExpression>(val);
        current++;
        return final_ans;
    }

    if (line_tokens[current].get_type() == TokenType::STRING)
    {
        Value val = line_tokens[current].get_original_text();
        auto final_ans = make_unique<LiteralExpression>(val);
        current++;
        return final_ans;
    }
    else if (line_tokens[current].get_type() == TokenType::NUMBER)
    {
        Value val = BigInt(line_tokens[current].get_original_text());
        auto final_ans = make_unique<LiteralExpression>(val);
        current++;
        return final_ans;
    }
    else if (line_tokens[current].get_type() == TokenType::IDENTIFIER)
    {
        string name = line_tokens[current].get_original_text();
        auto final_ans = make_unique<VariableExpression>(name);
        current++;
        return final_ans;
    }
    else if (line_tokens[current].get_type() == TokenType::OPEN_NORMAL)
    {
        current++;
        auto final_ans = parse_or(line_tokens, current);

        if (current >= line_tokens.size())
        {
            cerr << "Paranteza inchisa negasita la linia " << line_tokens[0].get_line_number() << endl;
            return nullptr;
        }
        if (final_ans == nullptr)
        {
            cerr << "Eroare de paranteze la linia " << line_tokens[current].get_line_number() << endl;
            return nullptr;
        }
        if (line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
        {
            cerr << "Paranteza neinchisa la linia " << line_tokens[current].get_line_number() << endl;
            return nullptr;
        }
        current++;
        return final_ans;
    }
    else if (line_tokens[current].get_type() == TokenType::HEAR)
    {
        current++;
        if (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::INT)
        {
            current++;
            return make_unique<HearExpression>(TokenType::INT);
        }
        else if (current < line_tokens.size())
        {
            return make_unique<HearExpression>(TokenType::STRING);
        }
        else
        {
            cerr << "Hear Incorect   " << endl;
            return nullptr;
        }
    }
    else
    {
        cerr << "Eroare la linia " << line_tokens[current].get_line_number() << endl;
        return nullptr;
    }
}

unique_ptr<Expression> parese_term(vector<Token> &line_tokens, size_t &current)
{
    auto left = parse_unary(line_tokens, current);
    if (left == nullptr)
        return nullptr;
    while (current < line_tokens.size() && (line_tokens[current].get_type() == TokenType::MULTIPLY || line_tokens[current].get_type() == TokenType::DIV || line_tokens[current].get_type() == TokenType::MOD))
    {
        TokenType op = line_tokens[current].get_type();
        current++;
        auto right = parse_unary(line_tokens, current);
        if (right == nullptr)
            return nullptr;

        left = make_unique<BinaryExpression>(
            move(left),
            op,
            move(right));
    }
    return left;
}
unique_ptr<Expression> parse_expresion(vector<Token> &line_tokens, size_t &current)
{
    auto left = parese_term(line_tokens, current);
    if (left == nullptr)
        return nullptr;
    while (current < line_tokens.size() && (line_tokens[current].get_type() == TokenType::PLUS || line_tokens[current].get_type() == TokenType::MINUS))
    {
        TokenType op = line_tokens[current].get_type();
        current++;
        auto right = parese_term(line_tokens, current);
        if (right == nullptr)
            return nullptr;

        left = make_unique<BinaryExpression>(
            move(left),
            op,
            move(right));
    }
    return left;
}

unique_ptr<Expression> parse_comparison(vector<Token> &line_tokens, size_t &current)
{
    auto left = parse_expresion(line_tokens, current);
    if (left == nullptr)
        return nullptr;

    while (current < line_tokens.size() && (is_comparison(line_tokens[current].get_type())))
    {
        TokenType op = line_tokens[current].get_type();
        current++;
        auto right = parse_expresion(line_tokens, current);
        if (right == nullptr)
        {
            cerr << "Eroare de Sintaxa " << endl;
            return nullptr;
        }
        left = make_unique<BinaryExpression>(
            move(left),
            op,
            move(right));
    }
    return left;
}

unique_ptr<Expression> parse_and(vector<Token> &line_tokens, size_t &current)
{

    auto left = parse_comparison(line_tokens, current);
    if (left == nullptr)
    {
        cerr << "Eroare " << endl;
        return nullptr;
    }
    while (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::AND)
    {
        TokenType op = line_tokens[current].get_type();
        current++;
        auto right = parse_comparison(line_tokens, current);
        if (right == nullptr)
        {
            cerr << "Eroare " << endl;
            return nullptr;
        }
        left = make_unique<BinaryExpression>(move(left), op, move(right));
    }
    return left;
}
unique_ptr<Expression> parse_or(vector<Token> &line_tokens, size_t &current)
{
    auto left = parse_and(line_tokens, current);
    if (left == nullptr)
    {
        cerr << "Eroare" << endl;
        return nullptr;
    }
    while (current < line_tokens.size() && line_tokens[current].get_type() == TokenType::OR)
    {
        TokenType op = TokenType::OR;
        current++;
        auto right = parse_and(line_tokens, current);
        if (right == nullptr)
        {
            cerr << "Eroare tip or " << endl;
            return nullptr;
        }
        left = make_unique<BinaryExpression>(move(left), op, move(right));
    }
    return left;
}

unique_ptr<Statement> Parser(vector<vector<Token>> &whole_tokens, size_t &line_number)
{

    vector<Token> line_tokens = whole_tokens[line_number];

    if (int(line_tokens.size()) == 0)
        return nullptr;

    if (line_tokens[0].get_type() == TokenType::UNKNOWN)
    {
        cerr << "Intstructiune Necunoscuta la linia " << line_tokens[0].get_line_number() << endl;
        return nullptr;
    }

    if (line_tokens[0].get_type() == TokenType::COMMENTARY)
    {
        return nullptr;
    }

    if (line_tokens[0].get_type() == TokenType::BREAK)
    {
        if (line_tokens.size() > 2)
        {
            cerr << "Eroare break + unknown command " << endl;
            return nullptr;
        }

        line_number++;
        return make_unique<BreakStatement>();
    }
    if (line_tokens[0].get_type() == TokenType::CONTINUE)
    {
        if (line_tokens.size() > 2)
        {
            cerr << "Eroare continue + unknown command" << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<ContinueStatement>();
    }

    if (line_tokens[0].get_type() == TokenType::RETURN)
    {
        size_t current = 1;
        auto exp = parse_or(line_tokens, current);
        if (!exp)
        {
            cerr << "Eroare de sintaxa return la lina " << line_number << endl;
            return nullptr;
        }
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare de sintaxa return la lina " << line_number << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<ReturnStatemet>(move(exp));
    }

    if (line_tokens[0].get_type() == TokenType::FUNCTION)
    {
        size_t current = 1;
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::IDENTIFIER)
        {
            cerr << "Eroare de sintaxa pentru functie " << endl;
            return nullptr;
        }
        string function_name = line_tokens[current].get_original_text();
        current++;
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::OPEN_NORMAL)
        {
            cerr << "Eroare de sintaxa pentru functie " << endl;
            return nullptr;
        }
        current++;
        vector<string> parameters;

        bool expected_param = true;

        while (current < line_tokens.size() && line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
        {
            if (expected_param && line_tokens[current].get_type() == TokenType::COMMA)
            {
                cerr << "Eroare de sintaxa la linia " << line_number << endl;
                return nullptr;
            }
            if (!expected_param && line_tokens[current].get_type() == TokenType::IDENTIFIER)
            {
                cerr << "Eroare de sintaxa la linia" << line_number << endl;
                return nullptr;
            }
            if (line_tokens[current].get_type() == TokenType::COMMA)
            {
                current++;
                expected_param = true;
                continue;
            }

            if (expected_param)
            {
                if (line_tokens[current].get_type() != TokenType::IDENTIFIER)
                {
                    cerr << "Se astepta un Identifier la linia " << line_number << endl;
                    return nullptr;
                }
                string param = line_tokens[current].get_original_text();
                parameters.push_back(param);
                expected_param = false;
            }

            current++;
        }
        if (expected_param && !parameters.empty())
        {
            cerr << "Eroare la linia " << line_number << endl;
            return nullptr;
        }
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::CLOSED_NORMAL)
        {
            cerr << "Eroare de sintaxa la lina " << line_number << endl;
            return nullptr;
        }
        current++;
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare de sintaxa la linia " << line_number << endl;
            return nullptr;
        }
        line_number++;
        vector<unique_ptr<Statement>> body;
        while (line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() != TokenType::END)
        {
            auto p = Parser(whole_tokens, line_number);
            if (p == nullptr)
            {
                cerr << "Eroare la linia " << line_number << endl;
                return nullptr;
            }
            body.push_back(move(p));
        }
        if (line_number >= whole_tokens.size() || whole_tokens[line_number][0].get_type() != TokenType::END)
        {
            cerr << "Eroare de sintaxa " << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<FunctionStatement>(function_name, parameters, move(body));
    }

    if (line_tokens[0].get_type() == TokenType::FOR)
    {
        size_t current = 1;
        if (current + 2 >= line_tokens.size() || line_tokens[current].get_type() != TokenType::LET || line_tokens[current + 1].get_type() != TokenType::IDENTIFIER || line_tokens[current + 2].get_type() != TokenType::ASSIGN)
        {
            cerr << "Eroare For la lina " << line_tokens[current].get_line_number() << endl;
            return nullptr;
        }
        current += 3;
        auto exp = parse_or(line_tokens, current);
        if (exp == nullptr)
        {
            cerr << "Eroare For , start incorect la lina " << line_number << endl;
            return nullptr;
        }
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::COMMA)
        {
            cerr << "Eroare de sintaxa For la lina " << line_number << endl;
            return nullptr;
        }
        auto start = make_unique<LetStatement>(line_tokens[2].get_original_text(), move(exp));
        current++;
        auto condition = parse_or(line_tokens, current);
        if (condition == nullptr)
        {
            cerr << "Eroare For , conditie invalida la lina " << line_number << endl;
            return nullptr;
        }
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::COMMA)
        {
            cerr << "Eroare de sintaxa for la lina " << line_number << endl;
            return nullptr;
        }
        current++;
        if (current + 1 >= line_tokens.size() || line_tokens[current].get_type() != TokenType::IDENTIFIER || line_tokens[current + 1].get_type() != TokenType::ASSIGN)
        {
            cerr << "Eroare for la lina " << line_number << endl;
            return nullptr;
        }
        string step_name = line_tokens[current].get_original_text();
        current += 2;
        auto exp_step = parse_or(line_tokens, current);
        if (!exp_step)
        {
            cerr << "Eroare For, Step invalid la lina " << line_number << endl;
            return nullptr;
        }
        if (current >= line_tokens.size())
        {
            cerr << "Eroare For, sintaxa invalida la lina " << line_number << endl;
            return nullptr;
        }
        auto step = make_unique<AssignmentStatement>(step_name, move(exp_step));
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare for la lina " << line_number << endl;
            return nullptr;
        }
        line_number++;
        vector<unique_ptr<Statement>> body;
        while (line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() != TokenType::END)
        {
            auto p = Parser(whole_tokens, line_number);
            if (!p)
            {
                cerr << "Eroare la lina " << line_number << endl;
                return nullptr;
            }
            body.push_back(move(p));
        }
        if (line_number >= whole_tokens.size() || whole_tokens[line_number][0].get_type() != TokenType::END)
        {
            cerr << "Eroare FOR, lipsa END" << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<ForStatement>(move(start), move(condition), move(step), move(body));
    }

    if (line_tokens[0].get_type() == TokenType::WHILE)
    {
        size_t current = 1;
        auto expresion = parse_or(whole_tokens[line_number], current);
        if (expresion == nullptr)
        {
            cerr << "Expresie invalida la linia " << line_number << endl;
            return nullptr;
        }
        if (whole_tokens[line_number][current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare la lina " << whole_tokens[line_number][current].get_line_number() << endl;
            return nullptr;
        }
        line_number++;
        vector<unique_ptr<Statement>> body;
        while (line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() != TokenType::END)
        {
            auto p = Parser(whole_tokens, line_number);
            if (p == nullptr)
            {
                cerr << "Eroare la linia " << line_number << endl;
                return nullptr;
            }
            body.push_back(move(p));
        }
        if (line_number >= whole_tokens.size())
        {
            cerr << "End lipsa la linia " << line_number << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<WhileStatement>(move(expresion), move(body));
    }

    if (line_tokens[0].get_type() == TokenType::IF)
    {
        size_t current = 1;
        auto expresion = parse_or(whole_tokens[line_number], current);
        if (expresion == nullptr)
        {
            cerr << "expresie invalida la linia " << whole_tokens[line_number][0].get_line_number() << endl;
            return nullptr;
        }
        if (whole_tokens[line_number][current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare la lina  " << whole_tokens[line_number][current].get_line_number() << endl;
            return nullptr;
        }
        line_number++;
        vector<unique_ptr<Statement>> do_body;
        vector<unique_ptr<Statement>> else_body;
        while (line_number < whole_tokens.size() && (whole_tokens[line_number][0].get_type() != TokenType::ELSE && whole_tokens[line_number][0].get_type() != TokenType::END))
        {
            auto p = Parser(whole_tokens, line_number);
            if (p == nullptr)
            {
                cerr << "Eroare";
                return nullptr;
            }
            do_body.push_back(move(p));
        }
        if (line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() == TokenType::ELSE)
        {
            line_number++;
            while (line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() != TokenType::END)
            {
                auto p = Parser(whole_tokens, line_number);
                if (p == nullptr)
                {
                    cerr << "Eroare" << endl;
                    return nullptr;
                }
                else_body.push_back(move(p));
            }
        }
        if ((line_number < whole_tokens.size() && whole_tokens[line_number][0].get_type() != TokenType::END))
        {
            cerr << "Ednd lipsa la linia " << whole_tokens[line_number][0].get_line_number() << endl;
            return nullptr;
        }
        if (line_number >= whole_tokens.size())
        {
            cerr << "Eroare" << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<IfStatement>(move(expresion), move(do_body), move(else_body));
    }

    if (line_tokens[0].get_type() == TokenType::LET)
    {

        if (line_tokens.size() < 5)
        {
            cerr << "Asignare Invalida la linia " << line_tokens[0].get_line_number() << endl;
            return nullptr;
        }
        else if (line_tokens[1].get_type() != TokenType::IDENTIFIER || line_tokens[2].get_type() != TokenType::ASSIGN)
        {
            cerr << "Let gresit la linia " << line_tokens[0].get_line_number() << endl;
            return nullptr;
        }

        size_t current = 3;

        auto expresion = parse_or(line_tokens, current);

        if (expresion == nullptr)
            return nullptr;

        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
            return nullptr;
        line_number++;
        return make_unique<LetStatement>(line_tokens[1].get_original_text(), move(expresion));
    }

    if (line_tokens[0].get_type() == TokenType::SHOUT)
    {

        if (line_tokens.size() < 3)
        {
            cerr << "Shouter incorect la linia " << line_tokens[0].get_line_number() << endl;
            return nullptr;
        }

        size_t current = 1;
        auto expresion = parse_or(line_tokens, current);

        if (expresion == nullptr)
            return nullptr;

        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
            return nullptr;
        line_number++;
        return make_unique<ShoutStatement>(move(expresion));
    }

    if (line_tokens[0].get_type() == TokenType::IDENTIFIER)
    {
        size_t current = 0;
        auto left = parse_or(line_tokens, current);
        if (left)
        {
            auto left_res = dynamic_cast<IndexExpression *>(left.get());

            if (left_res)
            {

                if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::ASSIGN)
                {
                    cerr << "Eroare indxexpression" << endl;
                    return nullptr;
                }
                current++;
                auto new_val = parse_or(line_tokens, current);
                if (!new_val)
                {
                    cerr << "Eroare indx_expression + assign + bad value" << endl;
                    return nullptr;
                }
                if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
                {
                    cerr << "Eroare Index-assign-statement + bad remeinder " << endl;
                    return nullptr;
                }

                left.release();
                std::unique_ptr<IndexExpression> target(left_res);
                line_number++;
                return make_unique<IndexAssignmentStatement>(move(target), move(new_val));
            }
        }
    }

    if (line_tokens.size() >= 2 && line_tokens[0].get_type() == TokenType::IDENTIFIER && line_tokens[1].get_type() == TokenType::ASSIGN)
    {
        size_t current = 2;
        auto expression = parse_or(line_tokens, current);
        if (!expression)
            return nullptr;
        if (current >= line_tokens.size() || line_tokens[current].get_type() != TokenType::END_OF_LINE)
        {
            cerr << "Eroare de sintaxa " << endl;
            return nullptr;
        }
        line_number++;
        return make_unique<AssignmentStatement>(line_tokens[0].get_original_text(), move(expression));
    }

    size_t current = 0;
    auto expression = parse_or(line_tokens, current);

    if (!expression)
        return nullptr;

    if (current >= line_tokens.size() ||
        line_tokens[current].get_type() != TokenType::END_OF_LINE)
    {
        return nullptr;
    }

    line_number++;

    return make_unique<ExpressionStatement>(
        move(expression));

    return nullptr;
}

ExecutionResult Execute_statement(Statement *statement);

optional<Value> evaluate_expresion(Expression *expresion)
{

    auto *hear_exp = dynamic_cast<HearExpression *>(expresion);
    if (hear_exp)
    {
        Value ans;
        string s;
        getline(cin, s);
        if (s.empty())
        {
            cerr << "Eroare Hear nu s-a auzit nimic " << endl;
            return nullopt;
        }
        if (hear_exp->token == TokenType::INT)
        {
            if (!is_number(s))
            {
                cerr << "Eroare hear se astepta un numar " << endl;
                return nullopt;
            }
            ans = BigInt(s);
            return ans;
        }
        else if (hear_exp->token == TokenType::STRING)
        {
            ans = s;
            return ans;
        }
        else
        {
            cerr << "Eroare hear" << endl;
            return nullopt;
        }
    }

    auto *method_call_exp = dynamic_cast<MethodCallExpression *>(expresion);
    if (method_call_exp)
    {
        auto object_result = evaluate_expresion(method_call_exp->object.get());
        if (!object_result)
            return nullopt;

        Value object = *object_result;

        if (holds_alternative<shared_ptr<ListValue>>(object))
        {
            auto list = get<shared_ptr<ListValue>>(object);
            if (method_call_exp->mothod_name == "size")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda nu acepta argumente " << endl;
                    return nullopt;
                }
                return Value{static_cast<BigInt>(list->elements.size())};
            }
            else if (method_call_exp->mothod_name == "to_string")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Eroare metoda to_string nu are argumente" << endl;
                    return nullopt;
                }
                return Value{value_to_string(object)};
            }
            else if (method_call_exp->mothod_name == "join")
            {

                if (method_call_exp->arguments.size() == 0)
                {

                    string ans = "";
                    for (size_t i = 0; i < list->elements.size(); i++)
                    {
                        if (!holds_alternative<string>(list->elements[i]))
                        {
                            cerr << "Eroare join se aplica pe liste de stringuri " << endl;
                            return nullopt;
                        }
                        ans += get<string>(list->elements[i]);
                    }
                    return Value{ans};
                }
                else if (method_call_exp->arguments.size() == 1)
                {
                    auto padding = evaluate_expresion(method_call_exp->arguments[0].get());
                    if (!padding)
                    {
                        cerr << "Eroare apelare incorecta --> join(expresie invalida)" << endl;
                        return nullopt;
                    }
                    if (!holds_alternative<string>(*padding))
                    {
                        cerr << "Eroare apelare corecta --> join(string)" << endl;
                        return nullopt;
                    }
                    string ans = "";

                    for (size_t i = 0; i < list->elements.size(); i++)
                    {
                        if (!holds_alternative<string>(list->elements[i]))
                        {
                            cerr << "Eroare join se aplica pe liste de stringuri " << endl;
                            return nullopt;
                        }
                        ans += get<string>(list->elements[i]);
                        if (i != list->elements.size() - 1)
                            ans += get<string>(*padding);
                    }
                    return Value{ans};
                }
                else
                {
                    cerr << "Metoda join nu are mai mult de 1 argument" << endl;
                    return nullopt;
                }
            }
            else if (method_call_exp->mothod_name == "copy")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda copy nu are argumente " << endl;
                    return nullopt;
                }
                return copy_value(object);
            }
            else if (method_call_exp->mothod_name == "pop")
            {
                if (method_call_exp->arguments.empty())
                {
                    if (list->elements.size() <= 0)
                    {
                        cerr << "lita goala operatie pop imposibila " << endl;
                        return nullopt;
                    }
                    Value val = list->elements.back();
                    list->elements.pop_back();
                    return val;
                }
                else if (method_call_exp->arguments.size() == 1)
                {
                    auto indx_result = evaluate_expresion(method_call_exp->arguments[0].get());
                    if (!indx_result)
                        return nullopt;
                    Value indx = *indx_result;

                    if (!holds_alternative<BigInt>(indx))
                    {
                        cerr << "Indexare imposibila" << endl;
                        return nullopt;
                    }

                    if (get<BigInt>(indx) < 0 || list->elements.size() <= (get<BigInt>(indx)))
                    {
                        cerr << "Index imposibil" << endl;
                        return nullopt;
                    }

                    if (list->elements.size() <= 0)
                    {
                        cerr << "lita goala operatie pop imposibila " << endl;
                        return nullopt;
                    }
                    Value to_be_returned = list->elements[static_cast<size_t>(get<BigInt>(indx))];
                    list->elements.erase(list->elements.begin() + static_cast<size_t>(get<BigInt>(indx)));
                    return to_be_returned;
                }
                else
                {
                    cerr << "metoda pop are maxim un argument " << endl;
                    return nullopt;
                }
            }
            else if (method_call_exp->mothod_name == "push")
            {
                if (method_call_exp->arguments.size() != 1)
                {
                    cerr << "Eroare de sintaxa -- push are doar un argument" << endl;
                    return nullopt;
                }
                if (method_call_exp->arguments.size() == 1)
                {
                    auto to_be_pushed_res = evaluate_expresion(method_call_exp->arguments[0].get());
                    if (!to_be_pushed_res)
                        return nullopt;

                    Value to_be_pushed = *to_be_pushed_res;

                    list->elements.push_back(to_be_pushed);
                    return {Value{list}};
                }
            }

            cerr << "Meoda necunaoscuta: " << method_call_exp->mothod_name << endl;
            return nullopt;
        }
        else if (holds_alternative<string>(object))
        {
            auto s = get<string>(object);
            if (method_call_exp->mothod_name == "size")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda size nu accepta argumente " << endl;
                    return nullopt;
                }
                return Value{static_cast<BigInt>(s.size())};
            }
            else if (method_call_exp->mothod_name == "list")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda list() nu accepta argumente" << endl;
                    return nullopt;
                }
                auto list_return = make_shared<ListValue>();
                for (size_t i = 0; i < s.size(); i++)
                {
                    string p = "";
                    p += s[i];
                    list_return->elements.push_back(Value{p});
                }
                return Value{list_return};
            }
            else if (method_call_exp->mothod_name == "find")
            {
                if (method_call_exp->arguments.empty())
                {
                    cerr << "Metoda find asteapta argument" << endl;
                    return nullopt;
                }
                if (method_call_exp->arguments.size() == 1)
                {
                    auto to_be_found_res = evaluate_expresion(method_call_exp->arguments[0].get());
                    if (!to_be_found_res)
                    {
                        cerr << "Eroare metoda find asteapta argument valid";
                        return nullopt;
                    }
                    if (!holds_alternative<string>(*to_be_found_res))
                    {
                        cerr << "Eroare metoda find asteapta argument string" << endl;
                        return nullopt;
                    }
                    string s_to_be_found = get<string>(*to_be_found_res);
                    if (s.find(s_to_be_found) == string::npos)
                        return Value{BigInt(-1)};
                    return Value{BigInt(s.find(s_to_be_found))};
                }
            }
            else if (method_call_exp->mothod_name == "split")
            {
                if (method_call_exp->arguments.empty())
                {
                    string s_to_be_split = s;
                    stringstream ss(s);
                    auto list = make_shared<ListValue>();
                    string word;
                    while (ss >> word)
                        list->elements.push_back(Value{word});

                    return Value{list};
                }
                else if (method_call_exp->arguments.size() == 1)
                {
                    auto exp = evaluate_expresion(method_call_exp->arguments[0].get());
                    if (!exp)
                    {
                        cerr << "Eroare spli(expresie invalida) " << endl;
                        return nullopt;
                    }
                    if (!holds_alternative<string>(*exp))
                    {
                        cerr << "Eroare split(non-string)" << endl;
                        return nullopt;
                    }

                    if (get<string>(*exp) == "")
                    {
                        string s_to_be_split = s;
                        stringstream ss(s);
                        auto list = make_shared<ListValue>();
                        string word;
                        while (ss >> word)
                            list->elements.push_back(Value{word});

                        return Value{list};
                    }

                    string divider = get<string>(*exp);
                    auto list = make_shared<ListValue>();
                    size_t start = 0;
                    size_t poz;
                    while ((poz = s.find(divider, start)) != string::npos)
                    {
                        list->elements.push_back(Value{s.substr(start, poz - start)});
                        start = poz + divider.size();
                    }
                    list->elements.push_back(Value{s.substr(start)});
                    return Value{list};
                }
            }
            cerr << "Meoda necunaoscuta: " << method_call_exp->mothod_name << endl;
            return nullopt;
        }
        else if (holds_alternative<shared_ptr<DictionaryValue>>(object))
        {
            auto dict = get<shared_ptr<DictionaryValue>>(object);
            if (method_call_exp->mothod_name == "count")
            {
                if (method_call_exp->arguments.empty() || method_call_exp->arguments.size() > 1)
                {
                    cerr << "Metoda accepta doar un argument" << endl;
                    return nullopt;
                }
                auto to_be_foun = evaluate_expresion(method_call_exp->arguments[0].get());
                if (!to_be_foun)
                {
                    cerr << "Expresie invalida inauntru metodei count" << endl;
                    return nullopt;
                }
                Value val = *to_be_foun;
                if (!holds_alternative<string>(val))
                {
                    cerr << "Cheie invalida " << endl;
                    return nullopt;
                }
                if (dict->map.count(get<string>(val)))
                    return Value{true};
                else
                    return Value{false};
            }
            else if (method_call_exp->mothod_name == "to_string")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda to_string nu are argumente" << endl;
                    return nullopt;
                }
                return Value{value_to_string(object)};
            }
            else if (method_call_exp->mothod_name == "size")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "eroare metoda size nu accepta argumente " << endl;
                    return nullopt;
                }
                return Value{BigInt(dict->map.size())};
            }
            else if (method_call_exp->mothod_name == "keys")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Eroare metoda keys nu accepta argumente" << endl;
                    return nullopt;
                }
                auto list = make_shared<ListValue>();
                for (auto p : dict->map)
                    list->elements.push_back(Value{p.first});
                return Value{list};
            }
            else if (method_call_exp->mothod_name == "values")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Eroare metoda values nu accepta argumente" << endl;
                    return nullopt;
                }
                auto list = make_shared<ListValue>();
                for (auto p : dict->map)
                    list->elements.push_back(Value{p.second});
                return Value{list};
            }
            else if (method_call_exp->mothod_name == "remove")
            {
                if (method_call_exp->arguments.size() != 1)
                {
                    cerr << "Eroare metoda remove asteapta un argument" << endl;
                    return nullopt;
                }
                auto string_res = evaluate_expresion(method_call_exp->arguments[0].get());
                if (!string_res)
                {
                    cerr << "Eroare remve(expresie invalida) " << endl;
                    return nullopt;
                }
                if (!holds_alternative<string>(*string_res))
                {
                    cerr << "Eroare remove asteapta argument tip string" << endl;
                    return nullopt;
                }
                string s = get<string>(*string_res);
                if (dict->map.count(s))
                    dict->map.erase(s);
            }
            else if (method_call_exp->mothod_name == "copy")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Eroare metoda empty nu accepta argumente" << endl;
                    return nullopt;
                }
                auto new_dict = copy_value(object);
                return new_dict;
            }
            else
            {
                cerr << "Meoda necunaoscuta: " << method_call_exp->mothod_name << endl;
                return nullopt;
            }
        }
        else if (holds_alternative<BigInt>(object))
        {
            if (method_call_exp->mothod_name == "to_string")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda to_string nu accepta argumente" << endl;
                    return nullopt;
                }
                return Value{value_to_string(object)};
            }
            else
            {

                cerr << "Metoda " << method_call_exp->mothod_name << " nu exista " << endl;
                return nullopt;
            }
        }
        else if (holds_alternative<bool>(object))
        {

            if (method_call_exp->mothod_name == "to_string")
            {
                if (!method_call_exp->arguments.empty())
                {
                    cerr << "Metoda to_string nu are argumente" << endl;
                    return nullopt;
                }
                return Value{value_to_string(object)};
            }
            else
            {
                cerr << "Metoda " << method_call_exp->mothod_name << " nu exista " << endl;
                return nullopt;
            }
        }
    }

    auto function_call_exp = dynamic_cast<FunctionCallExpression *>(expresion);

    if (function_call_exp)
    {

        if (!functions.count(function_call_exp->function_name))
        {
            cerr << "Functia " << function_call_exp->function_name << " nu exista " << endl;
            return nullopt;
        }

        auto func = functions[function_call_exp->function_name];
        if (func->parameters.size() < function_call_exp->args.size())
        {
            cerr << "Paraetrii insuficienti" << endl;
            return nullopt;
        }
        else if (func->parameters.size() > function_call_exp->args.size())
        {
            cerr << "Prea multi parametrii " << endl;
            return nullopt;
        }
        auto saved_variables = variables;

        vector<Value> arguments;
        for (auto &argument : function_call_exp->args)
        {
            auto p = evaluate_expresion(argument.get());
            if (!p)
            {
                return nullopt;
            }
            arguments.push_back(*p);
        }
        for (size_t i = 0; i < func->parameters.size(); i++)
            variables[func->parameters[i]] = arguments[i];

        optional<Value> return_value = nullopt;

        for (const auto &state : func->body)
        {
            auto result = Execute_statement(state.get());
            if (result.did_return)
            {
                return_value = result.value;
                break;
            }
        }
        variables = saved_variables;
        if (!return_value)
        {
            cerr << "Functia nu a returnat o valoare" << endl;
            return nullopt;
        }
        return return_value;
    }

    auto *indx_expression = dynamic_cast<IndexExpression *>(expresion);
    if (indx_expression)
    {
        auto collection_result = evaluate_expresion(indx_expression->collection.get());
        if (!collection_result)
        {
            return nullopt;
        }
        Value collection_value = *collection_result;

        if (holds_alternative<shared_ptr<ListValue>>(collection_value))
        {

            auto list = get<shared_ptr<ListValue>>(collection_value);
            auto indx_result = evaluate_expresion(indx_expression->indx.get());
            if (!indx_result)
                return nullopt;

            Value indx = *indx_result;

            if (!holds_alternative<BigInt>(indx))
            {
                cerr << "Indexare imposibila" << endl;
                return nullopt;
            }
            BigInt index = get<BigInt>(indx);

            if (index < 0 || (index) >= list->elements.size())
            {
                cerr << "Index in afara listei" << endl;
                return nullopt;
            }
            return list->elements[static_cast<size_t>(get<BigInt>(indx))];
        }
        else if (holds_alternative<shared_ptr<DictionaryValue>>(collection_value))
        {
            auto dict = get<shared_ptr<DictionaryValue>>(collection_value);
            auto indx_res = evaluate_expresion(indx_expression->indx.get());
            if (!indx_res)
                return nullopt;
            Value indx = *indx_res;
            if (!holds_alternative<string>(indx))
                return nullopt;
            if (!dict->map.count(get<string>(indx)))
            {
                Value temp = BigInt(0);
                dict->map[get<string>(indx)] = temp;
            }
            return dict->map[get<string>(indx)];
        }
        else if (holds_alternative<string>(collection_value))
        {
            string s = get<string>(collection_value);
            auto indx_almost = evaluate_expresion(indx_expression->indx.get());
            if (!indx_almost)
                return nullopt;
            Value indx = *indx_almost;
            if (!holds_alternative<BigInt>(indx))
            {
                cerr << "Eroare indx !int " << endl;
                return nullopt;
            }
            if (get<BigInt>(indx) < 0 || (get<BigInt>(indx)) >= s.size())
            {
                cerr << "Index out of bounds";
                return nullopt;
            }
            string ans = "";
            ans += s[static_cast<size_t>(get<BigInt>(indx))];
            return Value{ans};
        }
        return nullopt;
    }

    auto *dict_exp = dynamic_cast<MapExpression *>(expresion);

    if (dict_exp)
    {
        auto dict = make_shared<DictionaryValue>();
        for (size_t i = 0; i < dict_exp->keys.size(); i++)
        {
            auto key = evaluate_expresion(dict_exp->keys[i].get());
            auto val = evaluate_expresion(dict_exp->vals[i].get());
            if (!key || !val)
                return nullopt;
            if (!holds_alternative<string>(*key))
                return nullopt;
            dict->map[get<string>(*key)] = *val;
        }
        return dict;
    }

    auto *list_exp = dynamic_cast<ListExpression *>(expresion);
    if (list_exp != nullptr)
    {
        auto list = make_shared<ListValue>();

        for (const auto &elem : list_exp->elements)
        {
            auto p = evaluate_expresion(elem.get());
            if (!p)
                return nullopt;

            list->elements.push_back(*p);
        }
        return Value{list};
    }

    auto *literal = dynamic_cast<LiteralExpression *>(expresion);
    if (literal != nullptr)
        return literal->val;

    auto *varialble = dynamic_cast<VariableExpression *>(expresion);
    if (varialble != nullptr)
    {
        if (variables.count(varialble->name))
            return variables[varialble->name];
        else
        {
            cerr << "Variabila inexistenta " << endl;
            return nullopt;
        }
    }

    auto *binary = dynamic_cast<BinaryExpression *>(expresion);
    if (binary != nullptr)
    {
        Value ans;
        auto left_result = evaluate_expresion(binary->left.get());
        if (!left_result)
            return nullopt;
        Value left = *left_result;
        TokenType op = binary->op;
        if (op == TokenType::AND && !turn_boolean(left))
        {
            ans = false;
            return ans;
        }
        if (op == TokenType::OR && turn_boolean(left))
        {
            ans = true;
            return ans;
        }
        auto righ_result = evaluate_expresion(binary->right.get());

        if (!righ_result)
            return nullopt;

        Value right = *righ_result;

        if (op == TokenType::AND)
        {
            ans = turn_boolean(left) && turn_boolean(right);
            return ans;
        }
        if (op == TokenType::OR)
        {
            ans = turn_boolean(left) || turn_boolean(right);
            return ans;
        }

        if (holds_alternative<string>(left) && holds_alternative<BigInt>(right) && op == TokenType::MULTIPLY)
        {
            string ans1 = "";
            BigInt num_times = get<BigInt>(right);
            string Left = get<string>(left);
            if (num_times < 0)
            {
                cerr << "Operatie invalida " << endl;
                return nullopt;
            }
            while (num_times)
            {
                ans1 += Left;
                num_times--;
            }
            ans = ans1;
            return ans;
        }
        if ((holds_alternative<string>(left) || holds_alternative<string>(right)) && op == TokenType::PLUS)
        {
            return Value{
                value_to_string(left) + value_to_string(right)};
        }
        if (holds_alternative<BigInt>(left) != holds_alternative<BigInt>(right))
        {
            cerr << "Operatie invalida" << endl;

            return nullopt;
        }

        if (holds_alternative<string>(left) && holds_alternative<string>(right))
        {
            string Left = get<string>(left);
            string Right = get<string>(right);

            if (op == TokenType::PLUS)
            {
                string p = Left + Right;
                ans = p;
                return ans;
            }
            else if (op == TokenType::MINUS)
            {
                cerr << "operatie invalida" << endl;
                return nullopt;
            }
            else if (op == TokenType::MULTIPLY)
            {
                cerr << "Operatie invalida" << endl;
                return nullopt;
            }
            else if (op == TokenType::DIV)
            {
                cerr << "Operatie invalida" << endl;
                return nullopt;
            }
        }

        if (is_comparison(op))
        {

            if (holds_alternative<string>(left) && holds_alternative<string>(right))
            {
                if (op == TokenType::EQUALS)
                {
                    ans = (get<string>(left) == get<string>(right));
                    return ans;
                }
                else if (op == TokenType::NOT_EQUALS)
                {
                    ans = (get<string>(left) != get<string>(right));
                    return ans;
                }
                else if (op == TokenType::LESS_THAN)
                {
                    ans = (get<string>(left) < get<string>(right));
                    return ans;
                }
                else if (op == TokenType::LESSEQQ_THAN)
                {
                    ans = (get<string>(left) <= get<string>(right));
                    return ans;
                }
                else if (op == TokenType::GRATER_THAN)
                {
                    ans = (get<string>(left) > get<string>(right));
                    return ans;
                }
                else
                {
                    ans = (get<string>(left) >= get<string>(right));
                    return ans;
                }
            }
            else if (holds_alternative<BigInt>(left) && holds_alternative<BigInt>(right))
            {
                if (op == TokenType::EQUALS)
                {
                    ans = (get<BigInt>(left) == get<BigInt>(right));
                    return ans;
                }
                else if (op == TokenType::NOT_EQUALS)
                {
                    ans = (get<BigInt>(left) != get<BigInt>(right));
                    return ans;
                }
                else if (op == TokenType::LESS_THAN)
                {
                    ans = (get<BigInt>(left) < get<BigInt>(right));
                    return ans;
                }
                else if (op == TokenType::LESSEQQ_THAN)
                {
                    ans = (get<BigInt>(left) <= get<BigInt>(right));
                    return ans;
                }
                else if (op == TokenType::GRATER_THAN)
                {
                    ans = (get<BigInt>(left) > get<BigInt>(right));
                    return ans;
                }
                else
                {
                    ans = (get<BigInt>(left) >= get<BigInt>(right));
                    return ans;
                }
            }
            else if (holds_alternative<bool>(left) && holds_alternative<bool>(right))
            {
                if (op == TokenType::EQUALS)
                    return Value{get<bool>(left) == get<bool>(right)};
                if (op == TokenType::NOT_EQUALS)
                    return Value{(get<bool>(left) != get<bool>(right))};
                cerr << "Operatie invalida pe doua obiecte de tip bool" << endl;
                return nullopt;
            }
            else if (holds_alternative<shared_ptr<ListValue>>(left) && holds_alternative<shared_ptr<ListValue>>(right))
            {
                auto l1 = get<shared_ptr<ListValue>>(left);
                auto l2 = get<shared_ptr<ListValue>>(right);
                if (op == TokenType::EQUALS)
                {
                    if (l1->elements.size() != l2->elements.size())
                        return Value{false};

                    for (size_t i = 0; i < l1->elements.size(); i++)
                        if (!value_equals(l1->elements[i], l2->elements[i]))
                            return Value{false};

                    return Value{true};
                }
                if (op == TokenType::NOT_EQUALS)
                {
                    if (l1->elements.size() != l2->elements.size())
                        return Value{true};

                    for (size_t i = 0; i < l1->elements.size(); i++)
                        if (!value_equals(l1->elements[i], l2->elements[i]))
                            return Value{true};

                    return Value{false};
                }
                cerr << "Operatorul nu poate fii executat pe acest tip de date" << endl;
                return nullopt;
            }
            else if (holds_alternative<shared_ptr<DictionaryValue>>(left) && holds_alternative<shared_ptr<DictionaryValue>>(right))
            {
                auto d1 = get<shared_ptr<DictionaryValue>>(left);
                auto d2 = get<shared_ptr<DictionaryValue>>(right);
                if (op == TokenType::EQUALS)
                {
                    return Value{value_equals(left, right)};
                }
                if (op == TokenType::NOT_EQUALS)
                    return Value{!value_equals(left, right)};
                cerr << "Operatorul nu poate fii aplicat pe acest tip de date" << endl;
                return nullopt;
            }
        }

        if (op == TokenType::PLUS)
            ans = get<BigInt>(left) + get<BigInt>(right);
        else if (op == TokenType::MINUS)
            ans = get<BigInt>(left) - get<BigInt>(right);
        else if (op == TokenType::MULTIPLY)
            ans = get<BigInt>(left) * get<BigInt>(right);
        else if (op == TokenType::DIV)
        {
            if (get<BigInt>(right) == 0)
            {
                cerr << "Division by zero imposibile " << endl;
                return nullopt;
            }
            ans = get<BigInt>(left) / get<BigInt>(right);
        }
        else if (op == TokenType::MOD)
        {
            if (get<BigInt>(right) == 0)
            {
                cerr << "Modulo by 0 imposibile" << endl;
                return nullopt;
            }
            ans = get<BigInt>(left) % get<BigInt>(right);
        }

        return ans;
    }

    auto *unary = dynamic_cast<UnaryExpression *>(expresion);
    if (unary != nullptr)
    {
        if (unary->op == TokenType::MINUS)
        {
            auto right_result = evaluate_expresion(unary->right.get());
            if (!right_result)
                return nullopt;
            Value right = *right_result;
            if (holds_alternative<BigInt>(right))
            {
                Value ans = -1 * (get<BigInt>(right));
                return ans;
            }
            else if (holds_alternative<string>(right))
            {

                string s = get<string>(right);
                if (s.empty())
                    return Value{s};
                size_t i, j;
                i = 0;
                j = s.size() - 1;
                while (i < j)
                {
                    char aux = s[i];
                    s[i] = s[j];
                    s[j] = aux;
                    i++;
                    j--;
                }
                Value ans = s;
                return ans;
            }
            else
            {
                cerr << "Operatie invalida " << endl;
                return nullopt;
            }
        }
        else if (unary->op == TokenType::NOT)
        {
            auto right_result = evaluate_expresion(unary->right.get());
            if (!right_result)
                return nullopt;
            Value right = *right_result;
            if (holds_alternative<bool>(right))
            {
                Value ans = !(get<bool>(right));
                return ans;
            }
        }
    }

    return nullopt;
}

optional<bool> evaluate_condition(Expression *expression)
{
    auto is_ok_res = evaluate_expresion(expression);
    if (!is_ok_res)
        return nullopt;

    Value val = *is_ok_res;
    return turn_boolean(val);
}

ExecutionResult Execute_statement(Statement *statement)
{

    auto *continue_statement = dynamic_cast<ContinueStatement *>(statement);
    if (continue_statement)
    {
        return {false, false, true, nullopt};
    }

    auto *break_statement = dynamic_cast<BreakStatement *>(statement);
    if (break_statement)
    {
        return {false, true, false, nullopt};
    }

    auto *indx_assign_statement = dynamic_cast<IndexAssignmentStatement *>(statement);
    if (indx_assign_statement)
    {
        auto element = indx_assign_statement->target.get();
        auto list_res = evaluate_expresion(element->collection.get());
        if (!list_res)
            return {false, false, false, nullopt};

        auto indx_res = evaluate_expresion(element->indx.get());
        if (!indx_res)
            return {false, false, false, nullopt};
        if (holds_alternative<shared_ptr<ListValue>>(*list_res))
        {
            auto list = get<shared_ptr<ListValue>>(*list_res);
            if (!holds_alternative<BigInt>(*indx_res))
            {
                cerr << "Indxare cu noninteger " << endl;
                return {false, false, false, nullopt};
            }
            auto indx = get<BigInt>(*indx_res);
            if ((indx) < 0 || (indx) >= list->elements.size())
            {
                cerr << "Eroare Index out of bounds  " << endl;
                return {false, false, false, nullopt};
            }
            auto new_val_res = evaluate_expresion(indx_assign_statement->new_val.get());
            if (!new_val_res)
                return {false, false, false, nullopt};
            Value var = *new_val_res;
            list->elements[size_t(indx)] = var;
        }
        else if (holds_alternative<shared_ptr<DictionaryValue>>(*list_res))
        {
            auto dict = get<shared_ptr<DictionaryValue>>(*list_res);
            if (!holds_alternative<string>(*indx_res))
            {
                cerr << "Cheie incorecta type !string " << endl;
                return {false, false, false, nullopt};
            }
            string key = get<string>(*indx_res);
            auto new_val_res = evaluate_expresion(indx_assign_statement->new_val.get());
            if (!new_val_res)
                return {false, false, false, nullopt};
            Value new_val = *new_val_res;
            dict->map[key] = new_val;
        }
        return {false, false, false, nullopt};
    }

    auto *shout = dynamic_cast<ShoutStatement *>(statement);
    if (shout != nullptr)
    {
        auto var_to_be_shouted_res = evaluate_expresion(shout->expresion.get());
        if (!var_to_be_shouted_res)
            return {false, false, false, nullopt};
        Value var_to_be_shouted = *var_to_be_shouted_res;
        print_value(var_to_be_shouted, terminal);
        return {false, false, false, nullopt};
        ;
    }

    auto for_state = dynamic_cast<ForStatement *>(statement);
    if (for_state)
    {
        Execute_statement(for_state->start.get());
        auto is_ok_res = evaluate_expresion(for_state->condition.get());
        if (!is_ok_res)
            return {false, false, false, nullopt};
        Value is_ok = *is_ok_res;
        Value copy = is_ok;
        while (turn_boolean(is_ok))
        {
            for (size_t i = 0; i < for_state->body.size(); i++)
            {
                auto res = Execute_statement(for_state->body[i].get());
                if (res.did_return)
                    return res;
                if (res.continue_state)
                {
                    break;
                }
                if (res.break_state)
                {
                    return {false, false, false, nullopt};
                }
            }
            Execute_statement(for_state->step.get());
            auto is_ok_1 = evaluate_expresion(for_state->condition.get());
            if (!is_ok_1)
                return {false, false, false, nullopt};
            ;
            is_ok = *is_ok_1;
            if ((holds_alternative<bool>(is_ok) != holds_alternative<bool>(copy)) || (holds_alternative<BigInt>(is_ok) != holds_alternative<BigInt>(copy)) || (holds_alternative<string>(is_ok) != holds_alternative<string>(copy)))
            {
                cerr << "Conditia nu isi poate schimba tipul " << endl;
                return {false, false, false, nullopt};
                ;
            }
        }
    }

    auto return_state = dynamic_cast<ReturnStatemet *>(statement);
    if (return_state)
    {
        auto val_res = evaluate_expresion(return_state->exp.get());
        if (!val_res)
            return {false, false, false, nullopt};

        return {true, false, false, *val_res};
    }

    auto function_state = dynamic_cast<FunctionStatement *>(statement);
    if (function_state)
    {
        functions[function_state->name] = function_state;
        return {false, false, false, nullopt};
    }

    auto *assign_state = dynamic_cast<AssignmentStatement *>(statement);
    if (assign_state)
    {
        string var_name = assign_state->variable_name;
        auto val_to_res = evaluate_expresion(assign_state->expresion.get());
        if (!val_to_res)
            return {false, false, false, nullopt};
        ;

        Value val_to = *val_to_res;
        if (variables.count(var_name))
        {
            variables[var_name] = val_to;
            return {false, false, false, nullopt};
            ;
        }
        else
        {
            cerr << "Variabila nu exista " << endl;
            return {false, false, false, nullopt};
            ;
        }
    }

    auto *expression_statement = dynamic_cast<ExpressionStatement *>(statement);
    if (expression_statement)
    {
        evaluate_expresion(expression_statement->expression.get());

        return {false, false, false, nullopt};
    }

    auto *let = dynamic_cast<LetStatement *>(statement);
    if (let != nullptr)
    {
        string var_name = let->variable_name;
        auto var_res = evaluate_expresion(let->expression.get());
        if (!var_res)
            return {false, false, false, nullopt};
        Value var = *var_res;
        variables[var_name] = var;
    }
    auto if_state = dynamic_cast<IfStatement *>(statement);
    if (if_state != nullptr)
    {
        auto is_ok_res = evaluate_condition(if_state->condition.get());
        if (!is_ok_res)
            return {false, false, false, nullopt};
        ;
        Value is_ok = *is_ok_res;
        if (turn_boolean(is_ok))
        {
            for (size_t i = 0; i < if_state->then_body.size(); i++)
            {
                auto res = Execute_statement(if_state->then_body[i].get());
                if (res.did_return)
                    return res;
                if (res.break_state)
                {
                    return {false, true, false, nullopt};
                }
                if (res.continue_state)
                    return {false, false, true, nullopt};
            }
        }
        else
        {
            for (size_t i = 0; i < if_state->else_body.size(); i++)
            {
                auto res = Execute_statement(if_state->else_body[i].get());
                if (res.did_return)
                    return res;
                if (res.break_state)
                {
                    return {false, true, false, nullopt};
                }
                if (res.continue_state)
                {
                    return {false, false, true, nullopt};
                }
            }
        }
    }
    auto while_state = dynamic_cast<WhileStatement *>(statement);
    if (while_state != nullptr)
    {
        auto is_ok_res = evaluate_expresion(while_state->condition.get());
        if (!is_ok_res)
            return {false, false, false, nullopt};
        Value is_ok = *is_ok_res;
        Value copy = is_ok;
        while (turn_boolean(is_ok))
        {
            for (size_t i = 0; i < while_state->body.size(); i++)
            {
                auto res = Execute_statement(while_state->body[i].get());
                if (res.did_return)
                    return res;
                if (res.break_state)
                {
                    return {false, false, false, nullopt};
                }
                if (res.continue_state)
                    break;
            }
            auto is_ok_1 = evaluate_expresion(while_state->condition.get());
            if (!is_ok_1)
                return {false, false, false, nullopt};
            ;
            is_ok = *is_ok_1;
            if ((holds_alternative<bool>(is_ok) != holds_alternative<bool>(copy)) || (holds_alternative<BigInt>(is_ok) != holds_alternative<BigInt>(copy)) || (holds_alternative<string>(is_ok) != holds_alternative<string>(copy)))
            {
                cerr << "Conditia nu isi poate schimba tipul " << endl;
                return {false, false, false, nullopt};
                ;
            }
        }
    }
    return {false, false, false, nullopt};
}
void Evaluator(vector<unique_ptr<Statement>> &state)
{
    for (size_t i = 0; i < state.size(); i++)
    {
        Statement *p = state[i].get();
        Execute_statement(p);
    }
}

int main()
{

    auto start = chrono::steady_clock::now();
    ifstream file("code.txt");
    if (!file.is_open())
    {
        cerr << "File could not be open";
        return 1;
    }
    if (!terminal.is_open())
    {
        cerr << "Terminal nu a putut fii deschis";
        return 2;
    }

    string line;
    int line_number = 0;
    vector<Token> line_tokens;
    vector<vector<Token>> whole_tokens;
    vector<unique_ptr<Statement>> state;
    while (getline(file, line))
    {
        line_number++;
        if (line.empty())
            continue;

        if (line[0] == '#')
            continue;

        vector<string> token = tokenizer(line);

        for (auto word : token)
        {
            line_tokens.push_back(Token(type_of_token(word), word, line_number));
        }

        line_tokens.push_back(Token(TokenType::END_OF_LINE, "", line_number));

        whole_tokens.push_back(line_tokens);
        line_tokens.clear();
    }

    size_t curent_line_number = 0;
    while (curent_line_number < whole_tokens.size())
    {
        unique_ptr<Statement> p = Parser(whole_tokens, curent_line_number);
        if (p == nullptr)
        {
            cerr << "Eroare" << endl;
            return 3;
        }
        state.push_back(move(p));
    }

    Evaluator(state);
    file.close();
    line_tokens.push_back(Token(TokenType::END_OF_FILE, "", line_number));
    whole_tokens.push_back(line_tokens);
    auto end = chrono::steady_clock::now();

    auto time = chrono::duration_cast<std::chrono::microseconds>(
        end - start);

    std::cout << "Timp: " << time.count() << " microsecunde\n";
    /*
        for (auto tk : whole_tokens)
            for (auto tt : tk)
                tt.print();

            */
}
