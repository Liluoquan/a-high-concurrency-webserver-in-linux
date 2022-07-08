#include "../MemoryPool.h"
#include <memory>

using namespace std;

class Person {
public:
    int id_;
    std::string name_;
public:
    Person(int id, std::string name) : id_(id), name_(name) {
        printf("构造函数调用\n");
    }
    ~Person() {
        printf("析构函数调用\n");
    }
};

void test01() {
    printf("creating a person\n");
    shared_ptr<Person> p1(newElement<Person>(11, "Lawson"), deleteElement<Person>);
    printf("sizeof(name_) = %d\n", sizeof(p1->name_));
    printf("sizeof(Person) = %d\n", sizeof(Person));
}

int main() {
    test01();
}