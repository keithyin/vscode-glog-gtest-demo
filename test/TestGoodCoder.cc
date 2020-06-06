#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <string>
#include <tuple>

#include "../include/dict_parser.h"
#include "../include/field.h"
#include "../include/logging.h"

using namespace dict_field;
using namespace dict_parser;

TEST(GoodCoderTest, PrimitiveTypeField) {
    std::string height_str = "180";
    std::shared_ptr<Field<int>> heightfield = std::make_shared<Field<int>>("height");
    heightfield->deserilization(height_str);
    EXPECT_EQ(heightfield->data(), 180);

    std::string weight = "170.5";
    std::shared_ptr<Field<float>> weightfield = std::make_shared<Field<float>>("weight");
    weightfield->deserilization(weight);
    EXPECT_EQ(weightfield->data(), 170.5);

    std::string height_uint32 = "180";
    std::shared_ptr<Field<uint32_t>> weightfield_uint32 = std::make_shared<Field<uint32_t>>("weight");
    weightfield_uint32->deserilization(height_uint32);
    EXPECT_EQ(weightfield_uint32->data(), 180);

    std::string height_uint64 = "180";
    std::shared_ptr<Field<uint64_t>> weightfield_uint64 = std::make_shared<Field<uint64_t>>("weight");
    weightfield_uint64->deserilization(height_uint64);
    EXPECT_EQ(weightfield_uint64->data(), 180);

    std::string info = "hello world";
    std::shared_ptr<Field<std::string>> info_field = std::make_shared<Field<std::string>>("info");
    info_field->deserilization(info);
    EXPECT_EQ(info_field->data(), info);
}

TEST(GoodCoderTest, ArrayField) {
    std::string wantedids = "3:11111,22222,33333";
    std::shared_ptr<ArrayField<Field<uint64_t>>> ids_field = std::make_shared<ArrayField<Field<uint64_t>>>("wantedids");
    ids_field->deserilization(wantedids);
    EXPECT_EQ(ids_field->sub_fields_at(0)->data(), 11111);
    EXPECT_EQ(ids_field->sub_fields_at(1)->data(), 22222);
    EXPECT_EQ(ids_field->sub_fields_at(2)->data(), 33333);
}

TEST(GoodCoderTest, NestedField) {
    std::string peopleinfo = "yinpeng02#182#170.5";
    std::shared_ptr<Field<std::string>> namefield = std::make_shared<Field<std::string>>("name");
    std::shared_ptr<Field<int>> heightfield = std::make_shared<Field<int>>("height");
    std::shared_ptr<Field<float>> weightfield = std::make_shared<Field<float>>("weight");

    std::shared_ptr<NestedField> people_info_field = std::make_shared<NestedField>("people_info");
    people_info_field->add_field(namefield);
    people_info_field->add_field(heightfield);
    people_info_field->add_field(weightfield);

    people_info_field->deserilization(peopleinfo);

    namefield = std::dynamic_pointer_cast<Field<std::string>>(people_info_field->sub_fields_at(0));
    heightfield = std::dynamic_pointer_cast<Field<int>>(people_info_field->sub_fields_at(1));
    weightfield = std::dynamic_pointer_cast<Field<float>>(people_info_field->sub_fields_at(2));

    EXPECT_EQ(namefield->data(), "yinpeng02");
    EXPECT_EQ(heightfield->data(), 182);
    EXPECT_EQ(weightfield->data(), 170.5);

    namefield = std::dynamic_pointer_cast<Field<std::string>>(people_info_field->get_field("name"));
    heightfield = std::dynamic_pointer_cast<Field<int>>(people_info_field->get_field("height"));
    weightfield = std::dynamic_pointer_cast<Field<float>>(people_info_field->get_field("weight"));

    EXPECT_EQ(namefield->data(), "yinpeng02");
    EXPECT_EQ(heightfield->data(), 182);
    EXPECT_EQ(weightfield->data(), 170.5);
}

TEST(GoodCoderTest, Record) {
    std::string inp = "san#zhang\t18\t180\t3:math,cs,physis";
    std::shared_ptr<Record> record = std::make_shared<Record>();

    std::shared_ptr<NestedField> nf = std::make_shared<NestedField>("name", "#");
    nf->add_field(std::make_shared<Field<std::string>>("first_name"));
    nf->add_field(std::make_shared<Field<std::string>>("second_name"));
    record->add_field(nf);

    record->add_field(std::make_shared<Field<uint32_t>>("age"));
    record->add_field(std::make_shared<Field<int>>("height"));
    record->add_field(std::make_shared<ArrayField<Field<std::string>>>("items"));

    record->deserilization(inp);

    nf = std::dynamic_pointer_cast<NestedField>(record->get_field("name"));
    std::shared_ptr<Field<std::string>> firstname_field =
        std::dynamic_pointer_cast<Field<std::string>>(nf->get_field("first_name"));
    std::shared_ptr<Field<std::string>> secondname_field =
        std::dynamic_pointer_cast<Field<std::string>>(nf->get_field("second_name"));

    std::shared_ptr<Field<uint32_t>> agefield = std::dynamic_pointer_cast<Field<uint32_t>>(record->get_field("age"));
    std::shared_ptr<Field<int>> heightfield = std::dynamic_pointer_cast<Field<int>>(record->get_field("height"));

    std::shared_ptr<ArrayField<Field<std::string>>> itemsfield =
        std::dynamic_pointer_cast<ArrayField<Field<std::string>>>(record->get_field("items"));

    EXPECT_EQ(firstname_field->data(), "san");
    EXPECT_EQ(secondname_field->data(), "zhang");
    EXPECT_EQ(agefield->data(), 18);
    EXPECT_EQ(heightfield->data(), 180);
    EXPECT_EQ(itemsfield->sub_fields_at(0)->data(), "math");
    EXPECT_EQ(itemsfield->sub_fields_at(1)->data(), "cs");
    EXPECT_EQ(itemsfield->sub_fields_at(2)->data(), "physis");
}

struct DIYStrut {
    int age;
    std::string name;
    bool parse(const std::string& inp) {
        std::vector<std::string> items;
        string_splitter(inp, ",", items);
        if (items.size() != 2) {
            return false;
        }
        bool succ = false;
        succ = dict_field::parse(items[0], age);
        if (!succ) {
            return false;
        }
        succ = dict_field::parse(items[1], name);
        if (!succ) {
            return false;
        }
        return succ;
    }
};

TEST(GoodCoderTest, DIYStruct) {
    // 简单的自定义结构 使用 NestedField 就可以完成
    std::string info = "199,wangwu";
    std::shared_ptr<Field<DIYStrut>> infofield = std::make_shared<Field<DIYStrut>>("info");
    infofield->deserilization(info);
    EXPECT_EQ(infofield->data().age, 199);
    EXPECT_EQ(infofield->data().name, "wangwu");
}

std::shared_ptr<Record> record_builder_func() {
    std::shared_ptr<Record> record = std::make_shared<Record>();
    record->add_field(std::make_shared<Field<std::string>>("name"));
    record->add_field(std::make_shared<Field<uint32_t>>("age"));
    record->add_field(std::make_shared<Field<int>>("height"));
    record->add_field(std::make_shared<ArrayField<Field<std::string>>>("items"));

    std::shared_ptr<NestedField> nf = std::make_shared<NestedField>("money", ",");
    nf->add_field(std::make_shared<Field<int>>("income"));
    nf->add_field(std::make_shared<Field<int>>("expensis"));

    record->add_field(nf);
    return record;
}

TEST(GoodCoderTest, DictParser) {
    DictParser dictparser("datas/demo.txt", record_builder_func);
    dictparser.parse_file();

    ASSERT_EQ(dictparser.parsed_result().size(), 2);
    std::shared_ptr<Field<std::string>> name_field_first_succ_line =
        std::dynamic_pointer_cast<Field<std::string>>(dictparser.parsed_result()[0]->get_field("name"));
    std::shared_ptr<Field<std::string>> name_field_second_succ_line =
        std::dynamic_pointer_cast<Field<std::string>>(dictparser.parsed_result()[1]->get_field("name"));

    EXPECT_EQ(name_field_first_succ_line->data(), "dengyuting");
    EXPECT_EQ(name_field_second_succ_line->data(), "yinpeng");
}

REGISTER_FIELD("Field<DIYStruct>", dict_field::Field<DIYStrut>::new_instance, 10);

TEST(GoodCoderTest, DictParserWithHeaderFile) {
    DictParser dictparser("datas/demo2.txt",
                          record_builder_func,  // 提供了 header_file , 就不会使用提供的 record_builder_func 了.
                          "datas/header_file.txt");
    dictparser.parse_file();

    ASSERT_EQ(dictparser.parsed_result().size(), 3);
    std::shared_ptr<Field<std::string>> name_field_first_succ_line =
        std::dynamic_pointer_cast<Field<std::string>>(dictparser.parsed_result()[0]->sub_fields_at(0));
    std::shared_ptr<Field<std::string>> name_field_second_succ_line =
        std::dynamic_pointer_cast<Field<std::string>>(dictparser.parsed_result()[1]->sub_fields_at(0));

    EXPECT_EQ(name_field_first_succ_line->data(), "yinpeng");
    EXPECT_EQ(name_field_second_succ_line->data(), "dengyuting");
}
