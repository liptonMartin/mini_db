//
// Created by rvova on 18.05.2026.
//

#include <gtest/gtest.h>
#include <filesystem>
#include "storage_engine.h"
#include "datatypes.h"
#include "exceptions.h"

// Фикстура для тестов StorageEngine
class StorageEngineTest : public ::testing::Test {
protected:
    std::string test_db_name = "test_db";
    std::string test_table_name = "test_table";
    std::filesystem::path test_databases_path;

    void SetUp() override {
        // Создаем временную директорию для тестовых баз данных
        test_databases_path = std::filesystem::temp_directory_path() / "minidb_test_databases";
        std::filesystem::create_directories(test_databases_path);

        // Устанавливаем переменную окружения
        #ifdef _WIN32
            _putenv_s("PATH_TO_DATABASES", test_databases_path.string().c_str());
        #else
            setenv("PATH_TO_DATABASES", test_databases_path.string().c_str(), 1);
        #endif

        // Очистка перед каждым тестом
        cleanup();
    }

    void TearDown() override {
        // Очистка после каждого теста
        cleanup();

        // Удаляем временную директорию
        if (std::filesystem::exists(test_databases_path)) {
            std::filesystem::remove_all(test_databases_path);
        }
    }

    void cleanup() {
        // Удаляем тестовую базу данных, если она существует
        try {
            std::filesystem::path db_path = test_databases_path / test_db_name;
            if (std::filesystem::exists(db_path)) {
                StorageEngine::drop_database(test_db_name);
            }
        } catch (...) {
            // Игнорируем ошибки при очистке
        }
    }

    std::vector<Column> create_test_columns() {
        return {
            Column("id", DataType::Int, false, false),
            Column("name", DataType::String, false, false),
            Column("age", DataType::Int, true, false)
        };
    }
};

// ===== Тесты для create_database =====

TEST_F(StorageEngineTest, CreateDatabase) {
    EXPECT_NO_THROW(StorageEngine::create_database(test_db_name));

    // Проверяем, что директория базы данных создана
    std::filesystem::path db_path = test_databases_path / test_db_name;
    EXPECT_TRUE(std::filesystem::exists(db_path));

    // Проверяем, что файл схемы создан
    std::filesystem::path schema_path = db_path / (test_db_name + ".schema");
    EXPECT_TRUE(std::filesystem::exists(schema_path));
}

TEST_F(StorageEngineTest, CreateDatabaseTwice) {
    StorageEngine::create_database(test_db_name);

    // Попытка создать базу данных с тем же именем должна вызвать исключение
    EXPECT_THROW(StorageEngine::create_database(test_db_name), DatabaseHasAlreadyExistsException);
}

// ===== Тесты для drop_database =====

TEST_F(StorageEngineTest, DropDatabase) {
    StorageEngine::create_database(test_db_name);

    EXPECT_NO_THROW(StorageEngine::drop_database(test_db_name));

    // Проверяем, что директория базы данных удалена
    std::filesystem::path db_path = test_databases_path / test_db_name;
    EXPECT_FALSE(std::filesystem::exists(db_path));
}

TEST_F(StorageEngineTest, DropNonExistentDatabase) {
    // Попытка удалить несуществующую базу данных должна вызвать исключение
    EXPECT_THROW(StorageEngine::drop_database("non_existent_db"), DatabaseDoesNotExistException);
}

// ===== Тесты для create_table =====

TEST_F(StorageEngineTest, CreateTable) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();

    EXPECT_NO_THROW(StorageEngine::create_table(test_db_name, test_table_name, columns));

    // Проверяем, что файл таблицы создан
    std::filesystem::path table_path = test_databases_path / test_db_name / (test_table_name + ".binary");
    EXPECT_TRUE(std::filesystem::exists(table_path));
}

TEST_F(StorageEngineTest, CreateTableInNonExistentDatabase) {
    auto columns = create_test_columns();

    // Попытка создать таблицу в несуществующей базе данных
    EXPECT_THROW(StorageEngine::create_table("non_existent_db", test_table_name, columns),
                 DatabaseDoesNotExistException);
}

TEST_F(StorageEngineTest, CreateTableTwice) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();

    StorageEngine::create_table(test_db_name, test_table_name, columns);

    // Попытка создать таблицу с тем же именем
    EXPECT_THROW(StorageEngine::create_table(test_db_name, test_table_name, columns),
                 TableHasAlreadyExistsException);
}

// ===== Тесты для drop_table =====

TEST_F(StorageEngineTest, DropTable) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    EXPECT_NO_THROW(StorageEngine::drop_table(test_db_name, test_table_name));

    // Проверяем, что файл таблицы удален
    std::filesystem::path table_path = test_databases_path / test_db_name / (test_table_name + ".binary");
    EXPECT_FALSE(std::filesystem::exists(table_path));
}

TEST_F(StorageEngineTest, DropNonExistentTable) {
    StorageEngine::create_database(test_db_name);

    // Попытка удалить несуществующую таблицу
    EXPECT_THROW(StorageEngine::drop_table(test_db_name, "non_existent_table"),
                 TableDoesNotExistException);
}

// ===== Тесты для insert_elements =====

TEST_F(StorageEngineTest, InsertElements) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    std::vector<Value> values = {1, std::string("Alice"), 25};

    EXPECT_NO_THROW(StorageEngine::insert_elements(test_db_name, test_table_name, column_names, values));
}

TEST_F(StorageEngineTest, InsertElementsPartialColumns) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    // Вставка только обязательных колонок (age nullable)
    std::vector<std::string> column_names = {"id", "name"};
    std::vector<Value> values = {2, std::string("Bob")};

    EXPECT_NO_THROW(StorageEngine::insert_elements(test_db_name, test_table_name, column_names, values));
}

TEST_F(StorageEngineTest, InsertMultipleRows) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};

    // Вставляем несколько строк
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Проверяем через select
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, nullptr);
    EXPECT_EQ(result["id"].size(), 3);
}

// ===== Тесты для select_elements =====

TEST_F(StorageEngineTest, SelectAllElements) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});

    // Выбираем все элементы без условий
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, nullptr);

    EXPECT_TRUE(result.contains("id"));
    EXPECT_TRUE(result.contains("name"));
    EXPECT_TRUE(result.contains("age"));
    EXPECT_EQ(result["id"].size(), 2);
}

TEST_F(StorageEngineTest, SelectWithCondition) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Выбираем элементы с условием age > 25
    auto condition = std::make_unique<ComparisonCondition>("age", ComparisonDataType::Greater, 25);
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, std::move(condition));

    EXPECT_EQ(result["id"].size(), 2);
}

TEST_F(StorageEngineTest, SelectWithAlias) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});

    // Выбираем с алиасами
    std::unordered_map<std::string, Alias> columns_with_aliases = {
        {"id", "user_id"},
        {"name", "user_name"}
    };

    auto result = StorageEngine::select_elements(test_db_name, test_table_name, columns_with_aliases, nullptr);

    EXPECT_TRUE(result.contains("user_id"));
    EXPECT_TRUE(result.contains("user_name"));
    EXPECT_FALSE(result.contains("id"));
    EXPECT_FALSE(result.contains("name"));
}

TEST_F(StorageEngineTest, SelectEmptyTable) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    // Выбираем из пустой таблицы
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, nullptr);

    EXPECT_TRUE(result.contains("id"));
    EXPECT_TRUE(result.contains("name"));
    EXPECT_TRUE(result.contains("age"));
    EXPECT_EQ(result["id"].size(), 0);
}

// ===== Тесты для update_elements =====

TEST_F(StorageEngineTest, UpdateElements) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});

    // Обновляем возраст Alice
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);
    std::vector<std::string> update_columns = {"age"};
    std::vector<Value> update_values = {26};

    EXPECT_NO_THROW(StorageEngine::update_elements(test_db_name, test_table_name,
                                                   std::move(condition), update_columns, update_values));

    // Проверяем, что значение обновилось
    auto select_condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, std::move(select_condition));

    EXPECT_EQ(result["age"][0], 26);
}

TEST_F(StorageEngineTest, UpdateMultipleColumns) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});

    // Обновляем несколько колонок
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 1);
    std::vector<std::string> update_columns = {"name", "age"};
    std::vector<Value> update_values = {std::string("Alicia"), 26};

    EXPECT_NO_THROW(StorageEngine::update_elements(test_db_name, test_table_name,
                                                   std::move(condition), update_columns, update_values));
}


// ===== Тесты для delete_elements =====

TEST_F(StorageEngineTest, DeleteElements) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Удаляем одну строку
    auto condition = std::make_unique<ComparisonCondition>("id", ComparisonDataType::Equal, 2);
    EXPECT_NO_THROW(StorageEngine::delete_elements(test_db_name, test_table_name, std::move(condition)));

    // Проверяем, что осталось 2 строки
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, nullptr);
    EXPECT_EQ(result["id"].size(), 2);
}

TEST_F(StorageEngineTest, DeleteMultipleElements) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Удаляем несколько строк (age >= 30)
    auto condition = std::make_unique<ComparisonCondition>("age", ComparisonDataType::GreaterEqual, 30);
    EXPECT_NO_THROW(StorageEngine::delete_elements(test_db_name, test_table_name, std::move(condition)));

    // Проверяем, что осталась 1 строка
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, nullptr);
    EXPECT_EQ(result["id"].size(), 1);
}

// ===== Тесты с различными типами условий =====

TEST_F(StorageEngineTest, SelectWithBetweenCondition) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Выбираем элементы с условием BETWEEN
    auto condition = std::make_unique<BetweenCondition>("age", 26, 34);
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, std::move(condition));

    EXPECT_EQ(result["id"].size(), 1);
    EXPECT_EQ(result["id"][0], 2);
}

TEST_F(StorageEngineTest, SelectWithRegexCondition) {
    StorageEngine::create_database(test_db_name);
    auto columns = create_test_columns();
    StorageEngine::create_table(test_db_name, test_table_name, columns);

    std::vector<std::string> column_names = {"id", "name", "age"};
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {1, std::string("Alice"), 25});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {2, std::string("Bob"), 30});
    StorageEngine::insert_elements(test_db_name, test_table_name, column_names,
                                   {3, std::string("Charlie"), 35});

    // Выбираем элементы с условием LIKE (regex)
    auto condition = std::make_unique<RegexCondition>("name", ".*li.*");
    auto result = StorageEngine::select_elements(test_db_name, test_table_name, std::nullopt, std::move(condition));

    EXPECT_EQ(result["id"].size(), 2); // Alice и Charlie
}