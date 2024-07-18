#include "rpnmathparser.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <QDebug>

RpnMathParser::RpnMathParser() {}

/*!
 * Parses a mathematical expression
 * \param expression - the mathematical expression
 * \param err - reference to the debug string
 * \return the result of evaluating the input mathematical expression string in double format and writes "Success!" to err,
 * if parsing the expression fails, returns NAN and the corresponding error reason in err
 */
double RpnMathParser::parseString(QString expression, QString &err) {
    MathParserModel model;
    MathParserController controller(&model);

    // Convert QString to char* for processing
    QByteArray ba = expression.toLocal8Bit();
    char *input = ba.data();
    controller.setErrorString(err);

    // Attempt to set the input string in the controller
    if (controller.setInput(input)) {
        double result = controller.requestCalculations();
        err = "Success!";

        // Check if result is NaN, indicating an error in calculation
        if (result != result) {
            err = "Returned NaN, likely there was an invalid input (e.g., presence of real and imaginary parts)!";
            return NAN;
        }
        return result;
    } else {
        // Handle case where input is incorrect
        if (err.isEmpty()) {
            err = "Error: Incorrect expression input!";
        }
        return NAN;
    }
}

/*!
 * \brief Sets the input string for the MathParserController.
 * \param str Input string to be parsed.
 * \return Returns true if the input string is valid, otherwise false.
 */
bool MathParserController::setInput(const char *str) {
    if (!str) return false;

    // Free existing data before setting new input
    model_->freeData();
    strncpy(model_->input, str, 255);

    // Check if the input string is valid
    return model_->checkCorrectInput();
}

/*!
 * \brief Frees the calculation data in the MathParserController.
 */
void MathParserController::freeCalcData() {
    model_->freeData();
}

/*!
 * \brief Sets the error string in the MathParserController.
 * \param err Reference to a QString that contains the error message.
 */
void MathParserController::setErrorString(QString &err) {
    model_->setErrorString(err);
}

/*!
 * \brief Requests calculations based on the input string.
 * \return Returns the result of the calculations as a double.
 */
double MathParserController::requestCalculations() {
    // Parse the input string into lexemes if not already done
    if (model_->lexemesList.empty()) {
        model_->parseStringIntoLexemes();
    }
    model_->makeReversePolishNotationStack();
    double result = model_->calculateFullExpression();

    // Free data after calculation
    model_->freeData();
    return result;
}

/*!
 * \brief Constructor for MathParserModel.
 * Initializes the model by freeing existing data.
 */
MathParserModel::MathParserModel() {
    freeData();
}

/*!
 * \brief Frees the data stored in MathParserModel.
 * Resets current index, clears input, and empties stacks and lexeme list.
 */
void MathParserModel::freeData() {
    currentIndex = 0;
    input[0] = '\0';
    readyStack.clear();
    supportStack.clear();
    lexemesList.clear();
}

/*!
 * \brief Sets the error string in the MathParserModel.
 * \param err Reference to a QString that contains the error message.
 */
void MathParserModel::setErrorString(QString &err) {
    errorString = &err;
}

/*!
 * \brief Destructor for MathParserModel.
 * Ensures all data is freed upon object destruction.
 */
MathParserModel::~MathParserModel() {
    freeData();
}

/*!
 * \brief Checks if the input string is correct.
 * \return Returns true if the input is valid, otherwise false.
 */
bool MathParserModel::checkCorrectInput() {
    removeSpaces();

    // Check if input is empty
    if (input[0] == '\0') {
        QString error = "Error: Empty expression";
        *errorString = error;
        return false;
    }

    // Check for balanced parentheses
    if (!checkCorrectParentheses()) {
        QString temp = *errorString;
        if(temp.isEmpty()) {
            QString error = "Error: Mismatched parentheses! Possibly a missing closing parenthesis";
            *errorString = error;
        }
        return false;
    }

    // Check the order of operators and operands
    currentIndex = 0;
    return checkOperatorAndOperandsOrder(false);
}

/*!
 * \brief Removes spaces from the input string.
 */
void MathParserModel::removeSpaces() {
    int i = 0;
    int j = 0;
    for(i = 0, j = 0; input[i] != '\0'; i++) {
        if(input[i] == ' ') continue;
        input[j] = input[i];
        j++;
    }
    input[j] = '\0';
}

/*!
 * \brief Checks if the parentheses in the input string are correctly balanced.
 * \return Returns true if parentheses are balanced, otherwise false.
 */
bool MathParserModel::checkCorrectParentheses() {
    int parentheses_counter = 0;
    int size = QString(input).size();
    QString buff = input;

    // Check for specific invalid patterns like negative exponents without parentheses
    for(int i = 0; i < size; i++) {
        if(i + 1 < size) {
            if(buff.at(i) == '^' && buff.at(i + 1) == '-') {
                QString error = "Error! Missing parentheses when raising to a negative power! Location: " + QString::number(i + 1) + " character";
                *errorString = error;
                return false;
            }
        }
    }

    // Check for empty parentheses or mismatched parentheses
    for(int i = 0; i < size; i++) {
        if(input[i] == '(' && input[i + 1] == ')') {
            QString error = "Error! Missing function argument! Location between: " + QString::number(i) + " and " + QString::number(i + 1);
            *errorString = error;
            return false;
        } else if(input[i] == '(') {
            parentheses_counter++;
        } else if(input[i] == ')') {
            parentheses_counter--;
        }
    }
    return parentheses_counter == 0;
}

/*!
 * \brief Checks the order of operators and operands in the input string.
 * \param isCheckingInsideFunction Boolean flag indicating if checking is inside a function.
 * \return Returns true if the order is correct, otherwise false.
 */
bool MathParserModel::checkOperatorAndOperandsOrder(const bool isCheckingInsideFunction) {
    int allowSign = 1;
    int allowOperand = 1;
    int allowOperator = 0;
    int p_counter = 1;

    // Parse through the input string to ensure correct order of operators and operands
    while (input[currentIndex]) {
        if (input[currentIndex] == '(' || input[currentIndex] == ')') {
            if(input[currentIndex] == '(') {
                allowSign = 1;
                if(isCheckingInsideFunction) p_counter++;
            } else {
                if(isCheckingInsideFunction) p_counter--;
            }
            if(isCheckingInsideFunction && p_counter == 0) {
                break;
            }
            currentIndex++;
        } else if(allowSign && isSign()) {
            allowSign = 0;
            allowOperand = 1;
        } else if(allowOperand && (isNumber() || isFunction())) {
            allowOperand = 0;
            allowOperator = 1;
        } else if(allowOperator && isOperator()) {
            allowOperator = 0;
            allowOperand = 1;
        } else {
            QString bufMsg = *errorString;
            if (bufMsg.isEmpty()) {
                QString error = "Error: The input contains incorrect symbols or is incorrectly composed! Position";
                *errorString = error;
            }
            return false;
        }
    }
    return !allowOperand;
}

/*!
 * \brief MathParserModel::isSign
 * Checks if the current character is a sign (+ or -)
 * \return true if the current character is a sign, false otherwise
 */
bool MathParserModel::isSign() {
    if(input[currentIndex] == '-' || input[currentIndex] == '+') {
        currentIndex++;  // Move to the next character
        return true;
    } else {
        return false;
    }
}

/*!
 * \brief MathParserModel::isOperator
 * Checks if the current character is an operator (*, /, ^ or a sign)
 * \return true if the current character is an operator, false otherwise
 */
bool MathParserModel::isOperator() {
    if(isSign()) {
        return true;
    }
    if(input[currentIndex] == '*' || input[currentIndex] == '/' || input[currentIndex] == '^') {
        currentIndex++;  // Move to the next character
        return true;
    }
    return false;
}

/*!
 * \brief MathParserModel::isNumber
 * Checks if the current character sequence represents a number
 * \return true if the character sequence is a valid number, false otherwise
 */
bool MathParserModel::isNumber() {
    if(input[currentIndex] >= '0' && input[currentIndex] <= '9') {
        bool was_dot = false;
        // Loop through the characters while they form a valid number
        while((input[currentIndex] >= '0' && input[currentIndex] <= '9') || input[currentIndex] == '.') {
            if(input[currentIndex] == '.' && (input[currentIndex+1] > '9' || input[currentIndex+1] < '0' || was_dot)) {
                QString error = "Error! The expression contains a number with incorrect symbols after the dot! Location: " + QString::number(currentIndex+1);
                *errorString = error;
                return false;
            }
            if(input[currentIndex] == '.') {
                was_dot = true;
            }
            currentIndex++;
        }
        // Check for exponent form if applicable
        if(checkExponentionalForm()) {
            return true;
        }
    }
    return false;
}

/*!
 * \brief MathParserModel::isFunction
 * Checks if the current character sequence represents a mathematical function
 * \return true if the character sequence is a valid function, false otherwise
 */
bool MathParserModel::isFunction() {
    bool did_caught_function = false;
    // Check for known functions and advance the index accordingly
    if (!strncmp(&input[currentIndex], "cos", 3) || !strncmp(&input[currentIndex], "sin", 3)
        || !strncmp(&input[currentIndex], "tan", 3) || !strncmp(&input[currentIndex], "log", 3)
        || !strncmp(&input[currentIndex], "abs", 3)) {
        currentIndex += 3;
        did_caught_function = true;
    } else if (!strncmp(&input[currentIndex], "sqrt", 4)) {
        currentIndex += 4;
        did_caught_function = true;
    } else if (!strncmp(&input[currentIndex], "sqr", 3)) {
        currentIndex += 3;
        did_caught_function = true;
    } else if (!strncmp(&input[currentIndex], "ln", 2)) {
        currentIndex += 2;
        did_caught_function = true;
    }
    // Check for opening parenthesis and validate function expression
    if (did_caught_function && input[currentIndex++] == '(') {
        did_caught_function = checkOperatorAndOperandsOrder(true);
        if (did_caught_function && input[currentIndex] == ')') {
            currentIndex++;
            return true;
        }
    }
    return false;
}

/*!
 * \brief MathParserModel::checkExponentionalForm
 * Checks if the current character sequence represents a valid exponential form
 * \return true if the character sequence is a valid exponential form, false otherwise
 */
bool MathParserModel::checkExponentionalForm() {
    if (input[currentIndex] == 'e' || input[currentIndex] == 'E') {
        currentIndex++;
        if (input[currentIndex] == '-' || input[currentIndex] == '+') {
            currentIndex++;
        }
        // Check for digits after the exponent symbol
        if (input[currentIndex] < '0' || input[currentIndex] > '9') {
            QString error = "Error! Invalid exponential form! Location: " + QString::number(currentIndex+1) + "!";
            *errorString = error;
            return false;
        }
        // Loop through the digits
        while (input[currentIndex] >= '0' && input[currentIndex] <= '9') {
            currentIndex++;
        }
    }
    return true;
}

/*!
 * \brief MathParserModel::parseStringIntoLexemes
 * Parses the input string into a list of lexemes (tokens) for further processing
 */
void MathParserModel::parseStringIntoLexemes() {
    bool unarySignFlag = false;
    bool firstSignFlag = true;
    currentIndex = 0;
    if (input[currentIndex] != '-' && input[currentIndex] != '+') {
        firstSignFlag = false;
    }

    // Loop through the input string and add tokens to the lexeme list
    while (input[currentIndex]) {
        if ((input[currentIndex] >= '0' && input[currentIndex] <= '9') || input[currentIndex] == 'x' || input[currentIndex] == 'e' || input[currentIndex] == 'E') {
            addNumberToList();
        } else if (input[currentIndex] == '+' || input[currentIndex] == '-' || input[currentIndex] == '*' || input[currentIndex] == '/' || input[currentIndex] == '^') {
            addOperatorToList(unarySignFlag, firstSignFlag);
        } else if (input[currentIndex] == 'a' || input[currentIndex] == 's' || input[currentIndex] == 'l' || input[currentIndex] == 't' || input[currentIndex] == 'c' || input[currentIndex] == 'm') {
            addFunctionToList();
        } else if (input[currentIndex] == '(' || input[currentIndex] == ')') {
            addParenthesesToList(unarySignFlag);
        }
    }
}

/*!
 * \brief MathParserModel::addNumberToList
 * Adds a number token to the lexeme list
 */
void MathParserModel::addNumberToList() {
    double value = 0;
    int len = 0;
    sscanf(&input[currentIndex], "%le%n", &value, &len);
    currentIndex += len;
    if (len == 0) {
        currentIndex += 1;
        int exp = 0;
        sscanf(&input[currentIndex], "%d%n", &exp, &len);
        (--lexemesList.end())->value = exp;
        currentIndex += len;
        return;
    }
    lexemesList.emplace_back(value, 0, number);
}

/*!
 * \brief MathParserModel::addOperatorToList
 * Adds an operator token to the lexeme list
 * \param unarySignFlag - flag indicating if the operator is a unary sign
 * \param firstSignFlag - flag indicating if the operator is the first sign in the expression
 */
void MathParserModel::addOperatorToList(bool &unarySignFlag, bool &firstSignFlag) {
    if (input[currentIndex] == '+' || input[currentIndex] == '-') {
        if (unarySignFlag || firstSignFlag) {
            unarySignFlag = false;
            firstSignFlag = false;
            lexemesList.emplace_back(0, 0, number);
        }
        if (input[currentIndex] == '+') {
            lexemesList.emplace_back(0, 1, plus);
        } else {
            lexemesList.emplace_back(0, 1, minus);
        }
    } else if (input[currentIndex] == '*') {
        lexemesList.emplace_back(0, 2, mult);
    } else if (input[currentIndex] == '/') {
        lexemesList.emplace_back(0, 2, division);
    } else if (input[currentIndex] == '^') {
        lexemesList.emplace_back(0, 3, pow_t);
    }
    currentIndex++;
}

/*!
 * \brief MathParserModel::addFunctionToList
 * Adds a function token to the lexeme list
 */
void MathParserModel::addFunctionToList() {
    // Check for known functions and add them to the lexeme list
    if (!strncmp(&(input[currentIndex]), "cos", 3)) {
        lexemesList.emplace_back(0, 4, cos_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "sin", 3)) {
        lexemesList.emplace_back(0, 4, sin_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "tan", 3)) {
        lexemesList.emplace_back(0, 4, tan_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "log", 3)) {
        lexemesList.emplace_back(0, 4, log_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "abs", 3)) {
        lexemesList.emplace_back(0, 4, abs_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "sqrt", 4)) {
        lexemesList.emplace_back(0, 4, sqrt_t);
        currentIndex += 4;
    } else if (!strncmp(&(input[currentIndex]), "sqr", 3)) {
        lexemesList.emplace_back(0, 4, sqr_t);
        currentIndex += 3;
    } else if (!strncmp(&(input[currentIndex]), "ln", 2)) {
        lexemesList.emplace_back(0, 4, ln_t);
        currentIndex += 2;
    }
}

/*!
 * \brief Adds parentheses to the lexeme list.
 * \param unarySignFlag Reference to a flag indicating if the next character is a unary sign.
 */
void MathParserModel::addParenthesesToList(bool &unarySignFlag) {
    if (input[currentIndex] == '(') {
        lexemesList.emplace_back(0, 0, open_p);
        if (input[currentIndex + 1] == '+' || input[currentIndex + 1] == '-') {
            unarySignFlag = true;
        }
    } else {
        lexemesList.emplace_back(0, 0, close_p);
    }
    currentIndex++;
}

/*!
 * \brief Converts the list of lexemes into a reverse Polish notation stack.
 */
void MathParserModel::makeReversePolishNotationStack() {
    for (auto it = lexemesList.begin(); it != lexemesList.end(); it++) {
        if (it->type == number || it->type == x)
            readyStack.push_front(*it);
        else if (it->type >= plus)
            handleSupportStack(*it);
        else if (it->type == open_p)
            supportStack.push_front(*it);
        else if (it->type == close_p)
            handleCloseParentheses();
    }
    for (auto it = supportStack.begin(); it != supportStack.end(); it++)
        moveFromSupportToReady();
    readyStack.reverse();
}

/*!
 * \brief Handles lexemes on the support stack based on their priority.
 * \param lex The current lexeme being processed.
 */
void MathParserModel::handleSupportStack(MathParserModel::lexeme lex) {
    if (supportStack.empty()) {
        supportStack.push_front(lex);
    } else {
        for (auto it = supportStack.begin(); it != supportStack.end() && lex.priority <= it->priority; it++)
            moveFromSupportToReady();
        supportStack.push_front(lex);
    }
}

/*!
 * \brief Moves lexemes from the support stack to the ready stack.
 */
void MathParserModel::moveFromSupportToReady() {
    readyStack.push_front(*(supportStack.begin()));
    supportStack.pop_front();
}

/*!
 * \brief Handles closing parentheses in the support stack.
 * Moves lexemes to the ready stack until an opening parenthesis is encountered.
 */
void MathParserModel::handleCloseParentheses() {
    while (supportStack.begin()->type != open_p) {
        moveFromSupportToReady();
    }
    supportStack.pop_front();
}

/*!
 * \brief Calculates the full expression from the ready stack.
 * \return The result of the expression as a double.
 */
double MathParserModel::calculateFullExpression() {
    if (readyStack.size() == 1) {
        return readyStack.begin()->value;
    }

    auto prev_prev_it = readyStack.begin();
    auto prev_it = ++readyStack.begin();
    if (readyStack.size() == 2) {
        return calculateFunction(*prev_it, *prev_prev_it);
    }
    auto current_it = ++(++readyStack.begin());
    while ((current_it->type == number || current_it->type == x) && prev_it->type < 11) {
        current_it++;
        prev_it++;
        prev_prev_it++;
    }

    if (prev_it->type >= 11) {
        return squeezeFunctionResultWithNmb(prev_it, prev_prev_it);
    }

    if (current_it->type >= 11) {
        return squeezeFunctionResultWithNmb(current_it, prev_it);
    }

    prev_prev_it->value = calculateTwoOperators(*prev_prev_it, *prev_it, *current_it);
    prev_prev_it->type = number;
    prev_prev_it->priority = 0;
    readyStack.erase(current_it);
    readyStack.erase(prev_it);
    return calculateFullExpression();
}

/*!
 * \brief Squeezes function results with numbers in the ready stack.
 * \param function Iterator pointing to the function lexeme.
 * \param value Iterator pointing to the value lexeme.
 * \return The result of the expression as a double.
 */
double MathParserModel::squeezeFunctionResultWithNmb(const list<lexeme>::iterator function, const list<lexeme>::iterator value) {
    value->value = calculateFunction(*function, *value);
    value->type = number;
    value->priority = 0;
    readyStack.erase(function);
    return calculateFullExpression();
}

/*!
 * \brief Calculates the result of applying a function to a value.
 * \param function The function lexeme.
 * \param value The value lexeme.
 * \return The result of the function as a double.
 */
double MathParserModel::calculateFunction(const lexeme function, lexeme value) {
    double return_value = 0;
    if (function.type == cos_t) {
        return_value = cos(value.value);
    } else if (function.type == sin_t) {
        return_value = sin(value.value);
    } else if (function.type == tan_t) {
        return_value = tan(value.value);
    } else if (function.type == log_t) {
        return_value = log(value.value);
    } else if (function.type == ln_t) {
        return_value = log(value.value);
    } else if (function.type == sqrt_t) {
        return_value = sqrt(value.value);
    } else if (function.type == abs_t) {
        return_value = fabs(value.value);
    } else if (function.type == sqr_t) {
        return_value = pow(value.value, 2);
    }
    return return_value;
}

/*!
 * \brief Calculates the result of applying an operator to two operands.
 * \param operand1 The first operand lexeme.
 * \param operand2 The second operand lexeme.
 * \param operation The operator lexeme.
 * \return The result of the operation as a double.
 */
double MathParserModel::calculateTwoOperators(lexeme operand1, lexeme operand2, const lexeme operation) {
    double return_value = 0;
    if (operation.type == plus) {
        return_value += operand1.value + operand2.value;
    } else if (operation.type == minus) {
        return_value += operand1.value - operand2.value;
    } else if (operation.type == mult) {
        return_value += operand1.value * operand2.value;
    } else if (operation.type == division) {
        return_value += operand1.value / operand2.value;
    } else if (operation.type == pow_t) {
        return_value += pow(operand1.value, operand2.value);
    }
    return return_value;
}
