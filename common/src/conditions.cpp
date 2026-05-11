//
// Created by rvova on 10.05.2026.
//

#include <regex>

#include "datatypes.h"

std::unique_ptr<Condition> Condition::from_json(const nlohmann::json &j) {
    const std::string type = j.at("type");
    Column column = Column::from_json(j.at("column"));

    if (type == "comparison") {
        const auto comp_type = static_cast<ComparisonDataType>(j.at("comparison_type").get<int>());
        Value value = value_from_json(j.at("value"));
        return std::make_unique<ComparisonCondition>(std::move(column), comp_type, std::move(value));
    }
    if (type == "between") {
        auto start = value_from_json(j.at("start"));
        auto end = value_from_json(j.at("end"));
        return std::make_unique<BetweenCondition>(std::move(column), std::move(start), std::move(end));
    }
    if (type == "regex") {
        std::string regex = j.at("regex");
        return std::make_unique<RegexCondition>(std::move(column), std::move(regex));
    }

    throw std::runtime_error("Unknown condition type: " + type);
}

ComparisonCondition::ComparisonCondition(Column column, const ComparisonDataType comparison_type, Value value)
    : Condition(std::move(column)), _comparison_type(comparison_type), _value(std::move(value)) {
}

nlohmann::json ComparisonCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "comparison";
    j["column"] = _column.to_json();
    j["comparison_type"] = _comparison_type;
    j["value"] = value_to_json(_value);
    return j;
}

bool ComparisonCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column);
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

BetweenCondition::BetweenCondition(Column column, Value start, Value end)
    : Condition(std::move(column)), _start(std::move(start)), _end(std::move(end)) {
}

nlohmann::json BetweenCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "between";
    j["column"] = _column.to_json();
    j["start"] = value_to_json(_start);
    j["end"] = value_to_json(_end);
    return j;
}

bool BetweenCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column);
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

RegexCondition::RegexCondition(Column column, std::string regex)
    : Condition(std::move(column)), _regex(std::move(regex)) {
}


nlohmann::json RegexCondition::to_json() const {
    nlohmann::json j;
    j["type"] = "regex";
    j["column"] = _column.to_json();
    j["regex"] = _regex;
    return j;
}

bool RegexCondition::evaluate(const Row &column_values) const {
    const auto it = column_values.find(_column);
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
