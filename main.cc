#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "dict_parser.h"
#include "field.h"
#include "logging.h"
using namespace dict_field;
using namespace dict_parser;

class People {
   public:
    People(const std::string& name) : name_(name) {}
    People instance(const std::string& name) { return People(name); }
    const std::string& name() { return name_; }

   private:
    std::string name_;
};

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

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    DictParser dictparser("demo.txt", record_builder_func);
    dictparser.parse_file();
    People p1("1");
    People p2 = p1.instance("2");
    std::cout << "p1.name=" << p1.name() << " p2.name=" << p2.name() << std::endl;
    std::cout << 0xffffffff << std::endl;
    return 0;
}