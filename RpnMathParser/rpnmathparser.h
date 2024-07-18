#ifndef RPNMATHPARSER_H
#define RPNMATHPARSER_H

#include <QString>
#include <list>
using std::list;

class MathParserModel;

/*!
 * \brief A class - controller providing functionality for parsing mathematical expressions
 *
 * \details
 * Supported operators: +, -, *, /, ^, (, )
 * Supported functions: sin, cos, tan, ln, log, sqrt, abs, sqr
 * Test example: ((abs(-(cos(1) / (2^2 - (-0.5) * (sqrt(2)))) / ln(10) + (2^2 * sin(1)) - 1.234e-3)) + (tan(1)))
 * \warning Google calculator considers sqr(x) to be sqrt(x), although sqr means square (x^2), while sqrt means square root (√x)!
 */
class MathParserController {
private:
    MathParserModel *model_;
public:
    explicit MathParserController(MathParserModel *m): model_(m) {}

    bool setInput(const char *str);
    double requestCalculations();
    void freeCalcData();
    void setErrorString(QString &err);
};

/*!
 * \brief A facade class providing tools for parsing mathematical expressions
 */
class RpnMathParser {
public:
    RpnMathParser();
    static double parseString(QString expression, QString &err);
};

/*!
 * \brief A class representing the model containing all the logic for parsing and calculating mathematical expressions using RPN
 *
 * \details
 * Contains the interface for the architectural controller class
 */
class MathParserModel {
    friend bool MathParserController::setInput(const char *str);
    friend double MathParserController::requestCalculations();
    friend void MathParserController::freeCalcData();
    friend void MathParserController::setErrorString(QString &err);
public:
    MathParserModel();
    ~MathParserModel();

private:
    char input[256];
    unsigned currentIndex;
    QString *errorString;

    void freeData();
    void setErrorString(QString &err);

    // Functions for parsing and validating the input expression
    bool checkCorrectInput();
    void removeSpaces();
    bool checkCorrectParentheses();
    bool checkOperatorAndOperandsOrder(const bool isCheckingInsideFunction);
    bool isSign();
    bool isOperator();
    bool isNumber();
    bool isFunction();
    bool checkExponentionalForm();

    // Functions for adding and processing lexemes
    enum lexeme_type {
        number = 1, x, open_p, close_p, plus, minus, mult, division, mod_t, pow_t,
        cos_t, sin_t, tan_t, sqrt_t, ln_t, log_t, abs_t, sqr_t
    };

    /*!
     * \brief Lexeme structure for parsing a mathematical expression
     *
     * \details
     * value - value;
     * priority - priority;
     * lexeme_type - type of lexeme;
     */
    struct lexeme {
        double value;
        int priority;
        lexeme_type type;
        lexeme(double val, int prio, lexeme_type t) {
            value = val;
            priority = prio;
            type = t;
        }
    };

    std::list<lexeme> lexemesList;
    void parseStringIntoLexemes();
    void addNumberToList();
    void addOperatorToList(bool &unarySignFlag, bool &firstSignFlag);
    void addFunctionToList();
    void addParenthesesToList(bool &unarySignFlag);

// Functions for working with RPN
    list<lexeme> supportStack;
    list<lexeme> readyStack;
    void makeReversePolishNotationStack();
    void handleSupportStack(lexeme lex);
    void moveFromSupportToReady();
    void handleCloseParentheses();

    // Functions for calculating the parsed RPN string
    using iterator = list<lexeme>::iterator;
    double calculateFullExpression();
    double calculateFunction(const lexeme function, lexeme value);
    double squeezeFunctionResultWithNmb(const iterator function, const iterator value);
    double calculateTwoOperators(lexeme operand1, lexeme operand2, const lexeme operation);
};

#endif // RPNMATHPARSER_H
