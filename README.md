# tianditu_download

天地图瓦片下载

## 配置

```json
{
    "startIndex": 0,
    "level": 14,
    "numOfTk": 10000,
    "saveDir": "./datas",
    "keys": [
        "8e879a4cad078fd3ce7456f2737fc4cc"
    ],
    "tileMode": "ZYX",
    "extent": [
        103.937226,
        104.149369,
        30.757615,
        30.583421
    ]
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

## 依赖

- [json](https://github.com/nlohmann/json)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [easyloggingpp](https://github.com/abumq/easyloggingpp)
