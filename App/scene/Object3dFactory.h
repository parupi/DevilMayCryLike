#pragma once
#include <string>
#include <functional>
#include <unordered_map>
#include "3d/Object/Object3d.h"

class Object3dFactory
{
public:
    using Creator = std::function<std::unique_ptr<Object3d>(const std::string&)>;

    static std::unique_ptr<Object3d> Create(const std::string& className, const std::string& objectName);

    // 新しいクラスを登録する。アプリ起動時に一度だけ呼ぶ。
    static void Register(const std::string& className, Creator creator);

private:
    static std::unordered_map<std::string, Creator>& Registry();
};
