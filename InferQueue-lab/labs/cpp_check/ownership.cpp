#include<iostream>
#include<memory>
#include<cassert>
class Resource//构造和析构时打印日志
{
private:
int id_;
    /* data */
public:
    explicit Resource(int id) : id_(id) {
        std::cout << "construct " << id_ << '\n';
    }
     ~Resource() {
        std::cout << "destroy " << id_ << '\n';
    }

    int id() const {
        return id_;
    }
};
void consume(std::unique_ptr<Resource> resource) {
    assert(resource);
    std::cout << "consume " << resource->id() << '\n';
}

int main()
{
    auto first = std::make_unique<Resource> (1);
    auto second = std::move(first);
    assert(first == nullptr);
    assert(second != nullptr);
    consume(std::move(second));
    assert(second == nullptr);

}



