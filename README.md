# tianditu_download

![Static Badge](https://img.shields.io/badge/https%3A%2F%2Fimg.shields.io%2Fbadge%2Fany_text-C%2B%2B17-blue?style=flat&label=Standard%20C%2B%2B&link=https%3A%2F%2Fen.cppreference.com%2Fw%2Fcpp%2F17)
![Static Badge](https://img.shields.io/badge/https%3A%2F%2Fimg.shields.io%2Fbadge%2Fany_text-json-orange?label=JSON&link=https%3A%2F%2Fgithub.com%2Fnlohmann%2Fjson)
![Static Badge](https://img.shields.io/badge/https%3A%2F%2Fimg.shields.io%2Fbadge%2Fany_text-cpp--httplib-orange?label=HTTP&link=https%3A%2F%2Fgithub.com%2Fyhirose%2Fcpp-httplib)
![Static Badge](https://img.shields.io/badge/https%3A%2F%2Fimg.shields.io%2Fbadge%2Fany_text-easyloggingpp-orange?label=LOG&link=https%3A%2F%2Fgithub.com%2Fabumq%2Feasyloggingpp)

[![CMake on multiple platforms](https://github.com/geoyee/tianditu_download/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/geoyee/tianditu_download/actions/workflows/cmake-multi-platform.yml)

天地图瓦片下载

## 配置

```json
{
    "startIndex": 0,                            // 开始的索引，每次下载后会自动更新，达到继续下载的目的
    "level": 14,                                // 地图层级
    "numOfTk": 10000,                           // 每一个token一天能下多少张瓦片，个人开发者为1万/天
    "saveDir": "./datas",                       // 保存数据的路径
    "keys": [
        "8exxxxxxxxxxxxxxxxxxxxxxxxxxxxcc",
        "92xxxxxxxxxxxxxxxxxxxxxxxxxxxx4e"
    ],                                          // token的列表，token越多，每天能下的就越多
    "tileMode": "ZYX",                          // 保存文件夹是按Z/Y/X/tile.png还是Z/X/Y/tile.png
    "extent": [
        103.937226,
        104.149369,
        30.757615,
        30.583421
    ]                                           // 下载的范围
}
```

## 使用

```shell
// 编译
cd tianditu_download
mkdir build
cd build
cmake ..
make

// 使用（Windows）
cd build
TiandituDowunloader.exe config.json
```

## 参考

- [tianditu-python](https://github.com/huifer/tianditu-python/tree/master)
