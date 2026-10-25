#pragma once

#include <string>
#include <unordered_map>

namespace vcalc {

class UserDatabase {
public:
    explicit UserDatabase(std::string fileName);

    
    void load();

    
    bool find(const std::string& login) const noexcept;

    
    const std::string& getPassword(const std::string& login) const;

    const std::string& fileName() const noexcept { return fileName_; }
    std::size_t size() const noexcept { return users_.size(); }

private:
    std::string fileName_;
    std::unordered_map<std::string, std::string> users_;
};

}
