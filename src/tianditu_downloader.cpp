#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "tianditu_downloader.h"

#define M_PI 3.14159265358979323846

bool TianDiTuDownloader::loadConfig(const std::string &configPath)
{
    nlohmann::json jsonSolver;
    std::ifstream jsonFile(configPath.c_str());
    jsonFile >> jsonSolver;
    try
    {
        _startIndex = jsonSolver.at("startIndex");
        _threadNum = jsonSolver.at("threadNum"); // TODO: 这部分用起来
        _level = jsonSolver.at("level");
        _saveDir = jsonSolver.at("saveDir");
        std::filesystem::path path(_saveDir);
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directory(path);
        }
        _keys.resize(jsonSolver.at("keys").size());
        for (int i = 0; i < jsonSolver.at("keys").size(); ++i)
        {
            _keys[i] = jsonSolver.at("keys").at(i);
        }
        _extent = {jsonSolver.at("extent")[0], jsonSolver.at("extent")[1],
                   jsonSolver.at("extent")[2], jsonSolver.at("extent")[3]};

        // TODO: 打印在log中
        // std::cout << "==== load config success ====" << std::endl;
        // std::cout << "Start Index: \t" << _startIndex << std::endl;
        // std::cout << "Thread Num: \t" << _threadNum << std::endl;
        // std::cout << "Level: \t\t" << _level << std::endl;
        // std::cout << "SaveDir: \t" << _saveDir << std::endl;
        // std::cout << "keys: \n";
        // for (int i = 0; i < _keys.size(); ++i) {
        //   std::cout << "\t\t" << _keys.at(i) << std::endl;
        // }
        // std::cout << "Extent: \t" << _extent.minLng << " " << _extent.maxLng << "
        // "
        //           << _extent.minLat << " " << _extent.maxLat << std::endl;

        return true;
    }
    catch (...)
    {
        return false;
    }
}

TileIndex TianDiTuDownloader::lnglatToTileIndex(Lnglat lnglat, int level)
{
    double x = (lnglat.lng + 180) / 360;
    double y = (1 - std::log(std::tan(lnglat.lat * M_PI / 180) +
                             1 / std::cos(lnglat.lat * M_PI / 180)) /
                        M_PI) /
               2;
    double titleX = std::floor(x * pow(2, level));
    double titleY = std::floor(y * pow(2, level));
    return TileIndex{static_cast<int>(titleX), static_cast<int>(titleY)};
}

bool TianDiTuDownloader::downloadTile(TileIndex xy, int level,
                                      const std::string &key,
                                      const std::string &path)
{
    // 请求头
    httplib::Headers headers;
    headers.insert(
        {"Accept", "text/html,application/xhtml+xml,application/xml;"});
    headers.insert({"User-Agent", "Mozilla/5.0"});

    // 请求参数
    httplib::Params params;
    params.insert({"SERVICE", "WMTS"});
    params.insert({"REQUEST", "GetTile"});
    params.insert({"VERSION", "1.0.0"});
    params.insert({"LAYER", "img"});
    params.insert({"STYLE", "default"});
    params.insert({"TILEMATRIXSET", "w"});
    params.insert({"FORMAT", "tiles"});
    params.insert({"TILEMATRIX", std::to_string(level)});
    params.insert({"TILEROW", std::to_string(xy.y)});
    params.insert({"TILECOL", std::to_string(xy.x)});
    params.insert({"tk", key});

    // 使用httplib下载瓦片数据到本地路径path
    httplib::Client cli("http://t0.tianditu.gov.cn");
    auto res = cli.Get("/img_w/wmts", params, headers);

    // 成功则保存
    if (res && res->status == 200)
    {
        std::ofstream out(path, std::ios::binary);
        out << res->body;
        out.close();
        return true;
    }
    else
    {
        // TODO: 打印在log中
        return false;
    }
}

void TianDiTuDownloader::run()
{
    Lnglat minLnglat = {_extent.minLng, _extent.minLat};
    Lnglat maxLnglat = {_extent.maxLng, _extent.maxLat};
    TileIndex minXY = TianDiTuDownloader::lnglatToTileIndex(minLnglat, _level);
    TileIndex maxXY = TianDiTuDownloader::lnglatToTileIndex(maxLnglat, _level);

    int nOfOneKey = 0;
    int keyIndex = 0;
    std::string key = _keys[keyIndex];
    int tileNum = (maxXY.x - minXY.x + 1) * (maxXY.y - minXY.y + 1);
    // TODO: 打印在log中
    // std::cout << "All tiles: \t" << tileNum << std::endl;
    for (int i = _startIndex; i < tileNum; ++i)
    {
        TileIndex xy = {minXY.x + (i % (maxXY.x - minXY.x + 1)),
                        minXY.y + (i / (maxXY.x - minXY.x + 1))};
        // 按照Z（文件夹）/Y（文件夹）/X.png的路径保存数据
        std::string filePath = _saveDir + "/" + std::to_string(_level);
        if (!std::filesystem::exists(std::filesystem::path(filePath)))
        {
            std::filesystem::create_directory(std::filesystem::path(filePath));
        }
        filePath = filePath + "/" + std::to_string(xy.y);
        if (!std::filesystem::exists(std::filesystem::path(filePath)))
        {
            std::filesystem::create_directory(std::filesystem::path(filePath));
        }
        filePath = filePath + "/" + std::to_string(xy.x) + ".png";
        TianDiTuDownloader::downloadTile(xy, _level, key, filePath);

        // 一个key一天只能下1w个瓦片
        if ((++nOfOneKey) == 10000)
        {
            nOfOneKey = 0;
            key = _keys[++keyIndex];
            if (keyIndex == _keys.size())
            {
                std::cout << "All keys used!" << std::endl;
                _startIndex = i + 1;
                break;
            }
        }
    }

    // TODO: 修改配置文件的起始索引
    std::cout << "Next start index: " << _startIndex << std::endl;
}