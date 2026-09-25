#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <variant>
#include <cstdint>
struct ListValue;

using Value = std::variant<
    int,
    bool,
    std::string,
    std::shared_ptr<ListValue>>;

struct ListValue
{
    std::vector<Value> elements;
};

enum class TokenType
{
    OPEN_LIST,
    FUNCTION,
    RETURN,
    CLOSED_LIST,
    COMMA,
    DOT,
    LET,
    SHOUT,
    IF,
    ELSE,
    END,
    ENDL,
    WHILE,
    LIST,
    OPEN_NORMAL,
    CLOSED_NORMAL,
    NOT,
    TRUE,
    AND,
    OR,
    FALSE,
    IDENTIFIER,
    NUMBER,
    STRING,
    ASSIGN,
    END_OF_LINE,
    END_OF_FILE,
    PLUS,
    MINUS,
    MULTIPLY,
    DIV,
    MOD,
    EQUALS,
    NOT_EQUALS,
    LESS_THAN,
    GRATER_THAN,
    LESSEQQ_THAN,
    GRETAREQQ_THAN,

    UNKNOWN
};

std::string print_out_type(TokenType type)
{
    switch (type)
    {
    case TokenType::FUNCTION:
        return "FUNCTION";
    case TokenType::RETURN:
        return "RETURN";
    case TokenType::DOT:
        return "DOT";
    case TokenType::ENDL:
        return "ENDL";
    case TokenType::OPEN_LIST:
        return "OPEN LIST";
    case TokenType::CLOSED_LIST:
        return "CLOSED LIST";
    case TokenType::COMMA:
        return "COMMA";
    case TokenType::LIST:
        return "LIST";
    case TokenType::AND:
        return "AND";
    case TokenType::OR:
        return "OR";
    case TokenType::LET:
        return "LET";
    case TokenType::MOD:
        return "MOD";
    case TokenType::WHILE:
        return "WHILE";
    case TokenType::SHOUT:
        return "SHOUT";
    case TokenType::NUMBER:
        return "NUMBER";
    case TokenType::TRUE:
        return "TRUE";
    case TokenType::FALSE:
        return "FALSE";
    case TokenType::IDENTIFIER:
        return "IDENTIFIER";
    case TokenType::ASSIGN:
        return "ASSIGN";
    case TokenType::END_OF_LINE:
        return "END_OF_LINE";
    case TokenType::END_OF_FILE:
        return "END_OF_FILE";
    case TokenType::PLUS:
        return "PLUS";
    case TokenType::MINUS:
        return "MINUS";
    case TokenType::DIV:
        return "DIV";
    case TokenType::MULTIPLY:
        return "MULTIPLY";
    case TokenType::STRING:
        return "STRING";
    case TokenType::EQUALS:
        return "EQUALS";
        break;
    case TokenType::NOT_EQUALS:
        return "NOT EQUALS";
        break;
    case TokenType::LESS_THAN:
        return "LESS_THAN";
        break;
    case TokenType::LESSEQQ_THAN:
        return "LESS_OR_EQQ_THAN";
        break;
    case TokenType::GRATER_THAN:
        return "GREATER_THAN";
        break;
    case TokenType::GRETAREQQ_THAN:
        return "GRETAR_OR_EQQ_THAN";
        break;
    case TokenType::IF:
        return "IF";
        break;
    case TokenType::ELSE:
        return "else";
        break;
    case TokenType::END:
        return "END";
        break;
    case TokenType::OPEN_NORMAL:
        return "(";
        break;
    case TokenType::CLOSED_NORMAL:
        return ")";
        break;
    case TokenType::NOT:
        return "NOT";
        break;
    default:
        return "UNKNOWN";
        break;
    }
}
bool is_number(std::string word)
{

    if (word.size() == 0)
        return false;

    if (word[0] == '-')
    {
        if (word.size() == 1)
            return false;
        for (std::size_t i = 1; i < word.size(); i++)
            if (word[i] < '0' || word[i] > '9')
                return false;
    }
    else
        for (std::size_t i = 0; i < word.size(); i++)
            if (word[i] < '0' || word[i] > '9')
                return false;

    return true;
}

bool is_identifier(std::string word)
{

    if (word.size() == 0)
        return false;

    for (std::size_t i = 0; i < word.size(); i++)
    {
        if (word[i] == '_' || (word[i] >= '0' && word[i] <= '9'))
            continue;
        if (word[i] < 'a' || word[i] > 'z')
            return false;
    }

    return true;
}

bool is_stirng(std::string word)
{
    if (word.size() < 2)
        return false;

    return word[0] == '"' && word.back() == '"';
}

bool is_comparison(std::string word)
{
    return word == "==" || word == ">" || word == "<" || word == ">=" || word == "<=" || word == "!=";
}

TokenType return_comparison_TOKEN(std::string word)
{

    if (word == "==")
        return TokenType::EQUALS;
    else if (word == "<")
        return TokenType::LESS_THAN;
    else if (word == ">")
        return TokenType::GRATER_THAN;
    else if (word == ">=")
        return TokenType::GRETAREQQ_THAN;
    else if (word == "<=")
        return TokenType::LESSEQQ_THAN;
    else
        return TokenType::NOT_EQUALS;
}

TokenType type_of_token(std::string word)
{

    if (word == "let")
        return TokenType::LET;
    else if (word == "fun")
        return TokenType::FUNCTION;
    else if (word == "return")
        return TokenType::RETURN;
    else if (word == "[")
        return TokenType::OPEN_LIST;
    else if (word == "endl")
        return TokenType::ENDL;
    else if (word == ".")
        return TokenType::DOT;
    else if (word == "]")
        return TokenType::CLOSED_LIST;
    else if (word == ",")
        return TokenType::COMMA;
    else if (word == "shout")
        return TokenType::SHOUT;
    else if (word == "if")
        return TokenType::IF;
    else if (word == "!")
        return TokenType::NOT;
    else if (word == "and")
        return TokenType::AND;
    else if (word == "or")
        return TokenType::OR;
    else if (word == "else")
        return TokenType::ELSE;
    else if (word == "end")
        return TokenType::END;
    else if (word == "while")
        return TokenType::WHILE;
    else if (word == "true")
        return TokenType::TRUE;
    else if (word == "false")
        return TokenType::FALSE;
    else if (is_number(word))
        return TokenType::NUMBER;
    else if (word.size() == 1 && word[0] == '=')
        return TokenType::ASSIGN;
    else if (is_identifier(word))
        return TokenType::IDENTIFIER;
    else if (is_stirng(word))
        return TokenType::STRING;
    else if (word == "+")
        return TokenType::PLUS;
    else if (word == "-")
        return TokenType::MINUS;
    else if (word == "*")
        return TokenType::MULTIPLY;
    else if (word == "/")
        return TokenType::DIV;
    else if (word == "%")
        return TokenType::MOD;
    else if (word == "(")
        return TokenType::OPEN_NORMAL;
    else if (word == ")")
        return TokenType::CLOSED_NORMAL;
    else if (is_comparison(word))
        return return_comparison_TOKEN(word);

    return TokenType::UNKNOWN;
}

class Token
{
private:
    TokenType type;
    std::string original_text;
    int line_number;

public:
    Token(TokenType type, std::string original_text, int line_number)
    {
        this->type = type;
        this->original_text = original_text;
        this->line_number = line_number;
    }
    void print()
    {
        std::string ans;
        ans += print_out_type(type);

        while (ans.size() <= 15)
            ans += " ";
        if (type == TokenType::STRING)
            ans += original_text.substr(1, original_text.size() - 2);
        else
            ans += original_text;
        while (ans.size() <= 25)
            ans += " ";
        ans += "LINE: ";
        ans += std::to_string(line_number);
        std::cout << ans << std::endl;
    }
    TokenType get_type() { return type; }
    std::string get_original_text()
    {
        if (this->type == TokenType::STRING)
            return original_text.substr(1, original_text.size() - 2);
        else
            return original_text;
    }
    int get_line_number() { return line_number; }
};

class Expression
{
public:
    virtual ~Expression()
    {
    }
};
class LiteralExpression : public Expression
{
public:
    Value val;

    LiteralExpression(Value val) : val(val) {}
};

class VariableExpression : public Expression
{
public:
    std::string name;

    VariableExpression(std::string name = "") : name(name) {}
};

class BinaryExpression : public Expression
{
public:
    std::unique_ptr<Expression> left;
    TokenType op;
    std::unique_ptr<Expression> right;

    BinaryExpression(std::unique_ptr<Expression> left = nullptr, TokenType op = TokenType::UNKNOWN, std::unique_ptr<Expression> right = nullptr)
    {
        this->left = std::move(left);
        this->op = op;
        this->right = std::move(right);
    }
};

class UnaryExpression : public Expression
{
public:
    TokenType op;
    std::unique_ptr<Expression> right;
    UnaryExpression(TokenType op = TokenType::UNKNOWN, std::unique_ptr<Expression> right = nullptr)
    {
        this->op = op;
        this->right = std::move(right);
    }
};

class ListExpression : public Expression
{

public:
    std::vector<std::unique_ptr<Expression>> elements;

    ListExpression(std::vector<std::unique_ptr<Expression>> elements)
    {
        this->elements = std::move(elements);
    }
};

class IndexExpression : public Expression
{

public:
    std::unique_ptr<Expression> collection;
    std::unique_ptr<Expression> indx;
    IndexExpression(std::unique_ptr<Expression> collection = nullptr, std::unique_ptr<Expression> indx = nullptr)
    {
        this->collection = std::move(collection);
        this->indx = std::move(indx);
    }
};

class MethodCallExpression : public Expression
{
public:
    std::unique_ptr<Expression> object;
    std::string mothod_name;
    std::vector<std::unique_ptr<Expression>> arguments;

    MethodCallExpression(std::unique_ptr<Expression> object = nullptr, std::string method_name = "", std::vector<std::unique_ptr<Expression>> arguments = {})
    {
        this->object = std::move(object);
        this->mothod_name = method_name;
        this->arguments = std::move(arguments);
    }
};

class Statement
{
public:
    virtual ~Statement() {}
};

class LetStatement : public Statement
{
public:
    std::string variable_name;
    std::unique_ptr<Expression> expression;
    LetStatement(std::string variable_name = "", std::unique_ptr<Expression> expresion = nullptr)
    {
        this->variable_name = variable_name;
        this->expression = std::move(expresion);
    }
};

class ShoutStatement : public Statement
{
public:
    std::unique_ptr<Expression> expresion;
    ShoutStatement(std::unique_ptr<Expression> expresion = nullptr)
    {
        this->expresion = std::move(expresion);
    }
};

class IfStatement : public Statement
{
public:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> then_body;
    std::vector<std::unique_ptr<Statement>> else_body;
    IfStatement(std::unique_ptr<Expression> condition = nullptr, std::vector<std::unique_ptr<Statement>> then_body = {}, std::vector<std::unique_ptr<Statement>> else_body = {})
    {
        this->condition = std::move(condition);
        this->then_body = std::move(then_body);
        this->else_body = std::move(else_body);
    }
};

class WhileStatement : public Statement
{
public:
    std::unique_ptr<Expression> condition;
    std::vector<std::unique_ptr<Statement>> body;
    WhileStatement(std::unique_ptr<Expression> condition, std::vector<std::unique_ptr<Statement>> body)
    {
        this->condition = std::move(condition);
        this->body = std::move(body);
    }
};

class ExpressionStatement : public Statement
{

public:
    std::unique_ptr<Expression> expression;

    ExpressionStatement(std::unique_ptr<Expression> expression = nullptr)
    {
        this->expression = std::move(expression);
    }
};

class AssignmentStatement : public Statement
{
public:
    std::string variable_name;
    std::unique_ptr<Expression> expresion;
    AssignmentStatement(std::string variable_name = "", std::unique_ptr<Expression> expresion = nullptr)
    {
        this->variable_name = variable_name;
        this->expresion = std::move(expresion);
    }
};

class FunctionStatement : public Statement
{
public:
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::unique_ptr<Statement>> body;

    FunctionStatement(std::string name = "", std::vector<std::string> parameters = {}, std::vector<std::unique_ptr<Statement>> body = {})
    {
        this->name = name;
        this->parameters = parameters;
        this->body = std::move(body);
    }
};

class FunctionCallExpression : public Expression
{
public:
    std::string function_name;
    std::vector<std::unique_ptr<Expression>> args;
    FunctionCallExpression(std::string function_name = "", std::vector<std::unique_ptr<Expression>> args = {})
    {
        this->function_name = function_name;
        this->args = move(args);
    }
};

class ReturnStatemet : public Statement
{
public:
    std::unique_ptr<Expression> exp;
    ReturnStatemet(std::unique_ptr<Expression> exp = nullptr)
    {
        this->exp = std::move(exp);
    }
};

class BigInt
{
public:
    bool negative;
    std::vector<uint32_t> digits;

    uint32_t return_digit(std::string p)
    {
        uint32_t ans = 0;
        for (size_t i = 0; i < p.size(); i++)
            ans = ans * 10 + (p[i] - '0');
        return ans;
    }

    BigInt(std::string &text)
    {
        size_t start = 0;
        if (text[0] == '-')
        {
            negative = true;
            start = 1;
        }
        else
            negative = false;

        if (text.size() % 9 != 0)
        {
            int rest = text.size() % 9;
            uint32_t digit = 0;
            digit = return_digit(text.substr(start, rest));
            digits.push_back(digit);
            start = start + rest;
        }
        for (size_t i = start; i < text.size(); i += 9)
            digits.push_back(return_digit(text.substr(i, 9)));

      

        if (digits[0] == 0 && negative)
            negative = false;
        
        
    }

    void print()
    {
        for (size_t i = 0; i < digits.size(); i++)
            std::cout << digits[i] << " ";
    }
};
