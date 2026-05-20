//
// Created by rvova on 10.05.2026.
//

#include <regex>

#include "datatypes.h"

std::unique_ptr<Condition> Condition::from_json(const nlohmann::json &j) {
    const std::string type = j.at("type");

    if (type == "not") {
        auto operand = Condition::from_json(j.at("operand"));
        return std::make_unique<NotCondition>(std::move(operand));
    }
    if (type == "and") {
        auto left = Condition::from_json(j.at("left"));
        auto right = Condition::from_json(j.at("right"));
        return std::make_unique<AndCondition>(std::move(left), std::move(right));
    }
    if (type == "or") {
        auto left = Condition::from_json(j.at("left"));
        auto right = Condition::from_json(j.at("right"));
        return std::make_unique<OrCondition>(std::move(left), std::move(right));
    }

    const std::string column_name = j.at("column_name");

    if (type == "comparison") {
        const auto comp_type = static_cast<ComparisonDataType>(j.at("comparison_type").get<int>());
        Value value = value_from_json(j.at("value"));
        return std::make_unique<ComparisonCondition>(column_name, comp_type, std::move(value));
    }
    if (type == "between") {
        auto start = value_from_json(j.at("start"));
        auto end = value_from_json(j.at("end"));
        return std::make_unique<BetweenCondition>(column_name, std::move(start), std::move(end));
    }
    if (type == "regex") {
        std::string regex = j.at("regex");
        return std::make_unique<RegexCondition>(column_name, std::move(regex));
    }

    throw std::runtime_error("Unknown condition type: " + type);
}

ComparisonCondition::ComparisonCondition(const std::string &column_name, const ComparisonDataType comparison_type,
                                         Value value)
    : Condition(column_name), _comparison_type(comparison_type), _value(std::move(value)) {
}

nlohmann::json ComparisonCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "comparison";
    j["column_name"] = _column_name;
    j["comparison_type"] = _comparison_type;
    j["value"] = value_to_json(_value);
    return j;
}

bool ComparisonCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column_name);
    if (it == column_values.end()) {
        throw std::runtime_error("Column not found in row");
    }

    const Value &col_val = it->second;

    // Обработка NULL
    if (std::holds_alternative<Null>(col_val) || std::holds_alternative<Null>(_value)) {
        if (_comparison_type == ComparisonDataType::Equal) {
            return std::holds_alternative<Null>(col_val) && std::holds_alternative<Null>(_value);
        }
        if (_comparison_type == ComparisonDataType::NotEqual) {
            return std::holds_alternative<Null>(col_val) != std::holds_alternative<Null>(_value);
        }
        return false;
    }

    // Получаем значения в зависимости от типа
    if (std::holds_alternative<int>(col_val)) {
        const auto a = std::get<int>(col_val);
        const auto b = std::get<int>(_value);

        switch (_comparison_type) {
            case ComparisonDataType::Equal: return a == b;
            case ComparisonDataType::NotEqual: return a != b;
            case ComparisonDataType::Greater: return a > b;
            case ComparisonDataType::GreaterEqual: return a >= b;
            case ComparisonDataType::Less: return a < b;
            case ComparisonDataType::LessEqual: return a <= b;
        }
    } else if (std::holds_alternative<std::string>(col_val)) {
        const auto a = std::get<std::string>(col_val);
        const auto b = std::get<std::string>(_value);

        switch (_comparison_type) {
            case ComparisonDataType::Equal: return a == b;
            case ComparisonDataType::NotEqual: return a != b;
            case ComparisonDataType::Greater: return a > b;
            case ComparisonDataType::GreaterEqual: return a >= b;
            case ComparisonDataType::Less: return a < b;
            case ComparisonDataType::LessEqual: return a <= b;
        }
    }

    throw std::runtime_error("Unknown value type");
}

BetweenCondition::BetweenCondition(const std::string& column_name, Value start, Value end)
    : Condition(column_name), _start(std::move(start)), _end(std::move(end)) {
}

nlohmann::json BetweenCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "between";
    j["column_name"] = _column_name;
    j["start"] = value_to_json(_start);
    j["end"] = value_to_json(_end);
    return j;
}

bool BetweenCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column_name);
    if (it == column_values.end()) {
        throw std::runtime_error("Column not found in row");
    }

    const auto &col_val = it->second;

    // NULL не может быть в диапазоне
    if (std::holds_alternative<Null>(col_val) ||
        std::holds_alternative<Null>(_start) ||
        std::holds_alternative<Null>(_end)) {
        return false;
    }

    // Проверка для int
    if (std::holds_alternative<int>(col_val)) {
        const auto val = std::get<int>(col_val);
        const auto start = std::get<int>(_start);
        const auto end = std::get<int>(_end);
        return val >= start && val <= end;
    }

    // Проверка для string
    if (std::holds_alternative<std::string>(col_val)) {
        const auto val = std::get<std::string>(col_val);
        const auto start = std::get<std::string>(_start);
        const auto end = std::get<std::string>(_end);
        return val >= start && val <= end;
    }

    throw std::runtime_error("Unsupported type for BETWEEN");
}

RegexCondition::RegexCondition(const std::string& column_name, std::string regex)
    : Condition(column_name), _regex(std::move(regex)) {
}


nlohmann::json RegexCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "regex";
    j["column_name"] = _column_name;
    j["regex"] = _regex;
    return j;
}

bool RegexCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column_name);
    if (it == column_values.end()) {
        throw std::runtime_error("Column not found in row");
    }

    const Value &col_val = it->second;

    // Только строки поддерживают regex
    if (!std::holds_alternative<std::string>(col_val)) {
        return false;
    }

    const auto str = std::get<std::string>(col_val);
    const std::regex regex(_regex);
    return std::regex_match(str, regex);
}

NotCondition::NotCondition(std::unique_ptr<Condition> operand)
    : Condition(""), _operand(std::move(operand)) {
}

bool NotCondition::evaluate(const Row &column_values) const {
    return !_operand->evaluate(column_values);
}

nlohmann::json NotCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "not";
    j["operand"] = _operand->to_json();
    return j;
}

AndCondition::AndCondition(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right)
    : Condition(""), _left(std::move(left)), _right(std::move(right)) {
}

bool AndCondition::evaluate(const Row &column_values) const {
    return _left->evaluate(column_values) && _right->evaluate(column_values);
}

nlohmann::json AndCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "and";
    j["left"] = _left->to_json();
    j["right"] = _right->to_json();
    return j;
}

OrCondition::OrCondition(std::unique_ptr<Condition> left, std::unique_ptr<Condition> right)
    : Condition(""), _left(std::move(left)), _right(std::move(right)) {
}

bool OrCondition::evaluate(const Row &column_values) const {
    return _left->evaluate(column_values) || _right->evaluate(column_values);
}

nlohmann::json OrCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "or";
    j["left"] = _left->to_json();
    j["right"] = _right->to_json();
    return j;
}
