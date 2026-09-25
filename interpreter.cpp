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
#include <optional>

using namespace std;
ofstream terminal("output.txt");

struct ExecutionResult
{
    bool did_return = false;
    std::optional<Value> value = std::nullopt;
};
void print_value(const Value &value, ostream &out)
{
    if (holds_alternative<int>(value))
    {
        out << get<int>(value);
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

        out << "[";

        for (size_t i = 0; i < list->elements.size(); i++)
        {
            if (i > 0)
                out << ", ";

            print_value(list->elements[i], out);
        }

        out << "]";
    }
}
bool turn_boolean(Value x)
{
    if (holds_alternative<int>(x))
        return get<int>(x) != 0;

    if (holds_alternative<string>(x))
    {
        string s = get<string>(x);
        return !s.empty();
    }
    return get<bool>(x);
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

        if (ch == '[' || ch == ']' || ch == ',' || ch == '.')
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
        ans = stoi(p.get_original_text());
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
        Value val = stoi(line_tokens[current].get_original_text());
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

    auto *method_call_exp = dynamic_cast<MethodCallExpression *>(expresion);
    if (method_call_exp)
    {
        auto object_result = evaluate_expresion(method_call_exp->object.get());
        if (!object_result)
            return nullopt;

        Value object = *object_result;

        if (!holds_alternative<shared_ptr<ListValue>>(object))
        {
            cerr << "Metoda aplicabila doar pe liste " << endl;
            return nullopt;
        }
        auto list = get<shared_ptr<ListValue>>(object);
        if (method_call_exp->mothod_name == "size")
        {
            if (!method_call_exp->arguments.empty())
            {
                cerr << "Metoda nu acepta argumente " << endl;
                return nullopt;
            }
            return Value{static_cast<int>(list->elements.size())};
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

                if (!holds_alternative<int>(indx))
                {
                    cerr << "Indexare imposibila" << endl;
                    return nullopt;
                }

                if (get<int>(indx) <= 0 || list->elements.size() <= static_cast<size_t>(get<int>(indx)))
                {
                    cerr << "Index imposibil" << endl;
                    return nullopt;
                }

                if (list->elements.size() <= 0)
                {
                    cerr << "lita goala operatie pop imposibila " << endl;
                    return nullopt;
                }
                Value to_be_returned = list->elements[get<int>(indx)];
                list->elements.erase(list->elements.begin() + (get<int>(indx)));
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

        if (!holds_alternative<shared_ptr<ListValue>>(collection_value))
        {
            cerr << "Operatia se poate aplica doar pe liste" << endl;
            return nullopt;
        }
        auto list = get<shared_ptr<ListValue>>(collection_value);
        auto indx_result = evaluate_expresion(indx_expression->indx.get());
        if (!indx_result)
            return nullopt;

        Value indx = *indx_result;

        if (!holds_alternative<int>(indx))
        {
            cerr << "Indexare imposibila" << endl;
            return nullopt;
        }
        int index = get<int>(indx);

        if (index < 0 ||
            static_cast<size_t>(index) >= list->elements.size())
        {
            cerr << "Index in afara listei" << endl;
            return nullopt;
        }
        return list->elements[get<int>(indx)];
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

        if (holds_alternative<string>(left) && holds_alternative<int>(right) && op == TokenType::MULTIPLY)
        {
            string ans1 = "";
            int num_times = get<int>(right);
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
        if (holds_alternative<int>(left) != holds_alternative<int>(right))
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
            if (holds_alternative<string>(left) != holds_alternative<string>(right))
            {
                cerr << "Operatie invalida" << endl;
                return nullopt;
            }
            if (holds_alternative<string>(left))
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
            else if (holds_alternative<int>(left))
            {
                if (op == TokenType::EQUALS)
                {
                    ans = (get<int>(left) == get<int>(right));
                    return ans;
                }
                else if (op == TokenType::NOT_EQUALS)
                {
                    ans = (get<int>(left) != get<int>(right));
                    return ans;
                }
                else if (op == TokenType::LESS_THAN)
                {
                    ans = (get<int>(left) < get<int>(right));
                    return ans;
                }
                else if (op == TokenType::LESSEQQ_THAN)
                {
                    ans = (get<int>(left) <= get<int>(right));
                    return ans;
                }
                else if (op == TokenType::GRATER_THAN)
                {
                    ans = (get<int>(left) > get<int>(right));
                    return ans;
                }
                else
                {
                    ans = (get<int>(left) >= get<int>(right));
                    return ans;
                }
            }
        }

        if (op == TokenType::PLUS)
            ans = get<int>(left) + get<int>(right);
        else if (op == TokenType::MINUS)
            ans = get<int>(left) - get<int>(right);
        else if (op == TokenType::MULTIPLY)
            ans = get<int>(left) * get<int>(right);
        else if (op == TokenType::DIV)
        {
            if (get<int>(right) == 0)
            {
                cerr << "Division by zero imposibile " << endl;
                return nullopt;
            }
            ans = get<int>(left) / get<int>(right);
        }
        else if (op == TokenType::MOD)
        {
            if (get<int>(right) == 0)
            {
                cerr << "Modulo by 0 imposibile" << endl;
                return nullopt;
            }
            ans = get<int>(left) % get<int>(right);
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
            if (holds_alternative<int>(right))
            {
                Value ans = -1 * (get<int>(right));
                return ans;
            }
            else if (holds_alternative<string>(right))
            {
                string s = get<string>(right);
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
    auto *shout = dynamic_cast<ShoutStatement *>(statement);
    if (shout != nullptr)
    {
        auto var_to_be_shouted_res = evaluate_expresion(shout->expresion.get());
        if (!var_to_be_shouted_res)
            return {false, nullopt};
        Value var_to_be_shouted = *var_to_be_shouted_res;
        print_value(var_to_be_shouted, terminal);
        return {false, nullopt};
        ;
    }

    auto return_state = dynamic_cast<ReturnStatemet *>(statement);
    if (return_state)
    {
        auto val_res = evaluate_expresion(return_state->exp.get());
        if (!val_res)
            return {false, nullopt};

        return {true, *val_res};
    }

    auto function_state = dynamic_cast<FunctionStatement *>(statement);
    if (function_state)
    {
        functions[function_state->name] = function_state;
        return {false, nullopt};
    }

    auto *assign_state = dynamic_cast<AssignmentStatement *>(statement);
    if (assign_state)
    {
        string var_name = assign_state->variable_name;
        auto val_to_res = evaluate_expresion(assign_state->expresion.get());
        if (!val_to_res)
            return {false, nullopt};
        ;

        Value val_to = *val_to_res;
        if (variables.count(var_name))
        {
            variables[var_name] = val_to;
            return {false, nullopt};
            ;
        }
        else
        {
            cerr << "Variabila nu exista " << endl;
            return {false, nullopt};
            ;
        }
    }

    auto *expression_statement =
        dynamic_cast<ExpressionStatement *>(statement);

    if (expression_statement)
    {
        evaluate_expresion(expression_statement->expression.get());

        return {false, nullopt};
        ;
    }

    auto *let = dynamic_cast<LetStatement *>(statement);
    if (let != nullptr)
    {
        string var_name = let->variable_name;
        auto var_res = evaluate_expresion(let->expression.get());
        if (!var_res)
            return {false, nullopt};
        ;

        Value var = *var_res;
        variables[var_name] = var;
    }
    auto if_state = dynamic_cast<IfStatement *>(statement);
    if (if_state != nullptr)
    {
        auto is_ok_res = evaluate_condition(if_state->condition.get());
        if (!is_ok_res)
            return {false, nullopt};
        ;
        Value is_ok = *is_ok_res;
        if (turn_boolean(is_ok))
        {
            for (size_t i = 0; i < if_state->then_body.size(); i++)
            {
                auto res = Execute_statement(if_state->then_body[i].get());
                if (res.did_return)
                    return res;
            }
        }
        else
        {
            for (size_t i = 0; i < if_state->else_body.size(); i++)
            {
                auto res = Execute_statement(if_state->else_body[i].get());
                if (res.did_return)
                    return res;
            }
        }
    }
    auto while_state = dynamic_cast<WhileStatement *>(statement);
    if (while_state != nullptr)
    {
        auto is_ok_res = evaluate_expresion(while_state->condition.get());
        if (!is_ok_res)
            return {false, nullopt};
        ;
        Value is_ok = *is_ok_res;
        Value copy = is_ok;
        while (turn_boolean(is_ok))
        {
            for (size_t i = 0; i < while_state->body.size(); i++)
            {
                auto res = Execute_statement(while_state->body[i].get());
                if (res.did_return)
                    return res;
            }
            auto is_ok_1 = evaluate_expresion(while_state->condition.get());
            if (!is_ok_1)
                return {false, nullopt};
            ;
            is_ok = *is_ok_1;
            if ((holds_alternative<bool>(is_ok) != holds_alternative<bool>(copy)) || (holds_alternative<int>(is_ok) != holds_alternative<int>(copy)) || (holds_alternative<string>(is_ok) != holds_alternative<string>(copy)))
            {
                cerr << "Conditia nu isi poate schimba tipul " << endl;
                return {false, nullopt};
                ;
            }
        }
    }
    return {false, nullopt};
}
void Evaluator(vector<unique_ptr<Statement>> &state)
{
    for (size_t i = 0; i < state.size(); i++)
    {
        Statement *p = state[i].get();
        Execute_statement(p);
    }
}
/*
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

        vector<string> token = tokenizer(line);
        for (auto word : token)
        {
            cout << word << endl;
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
            cerr << "Eroare la lina " << whole_tokens[curent_line_number][0].get_line_number() << endl;
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

    for (auto tk : whole_tokens)
        for (auto tt : tk)
            tt.print();
}
*/
int main()
{
    string penis = "12345678901234567890";
    BigInt p(penis);
    p.print();
}