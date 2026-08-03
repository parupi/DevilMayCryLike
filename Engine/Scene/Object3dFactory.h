#pragma once
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "World3D/Object/Object3d.h"

/// <summary>
/// ステージデータの "class" 名から Object3d を作る登録制ファクトリ。
///
/// 登録するのは App 側（RegisterAllGameObjects）だが、
/// エディタの生成メニューがクラス一覧を引くため置き場所は Engine 側にしてある。
/// （Engine/Editor は App を include できない）
/// </summary>
class Object3dFactory
{
public:
    using Creator = std::function<std::unique_ptr<Object3d>(const std::string&)>;

    /// <summary>クラスを登録する。アプリ起動時に一度だけ呼ぶ</summary>
    /// <param name="usesModelName">
    /// Ground / Prop のように、生成時にモデル名を差し替えられるクラスなら true。
    /// エディタの生成ダイアログがモデル選択を出すかどうかの判断に使う
    /// </param>
    static void Register(const std::string& className, Creator creator, bool usesModelName = false);

    /// <summary>未登録のクラス名を渡した場合は素の Object3d を返す</summary>
    static std::unique_ptr<Object3d> Create(const std::string& className, const std::string& objectName);

    /// <summary>登録済みクラス名の一覧（辞書順）</summary>
    static std::vector<std::string> GetClassNames();

    /// <summary>そのクラスがモデル名を使うか</summary>
    static bool UsesModelName(const std::string& className);

private:
    struct Entry {
        Creator creator;
        bool usesModelName = false;
    };

    static std::unordered_map<std::string, Entry>& Registry();
};
