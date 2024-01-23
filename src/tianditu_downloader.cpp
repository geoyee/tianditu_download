#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include "httplib.h"
#include "nlohmann/json.hpp"
#include "tianditu_downloader.h"

#define M_PI 3.14159265358979323846

bool TianDiTuDownloader::loadConfig(const std::string &configPath)
{
    _configPath = configPath;
    nlohmann::json jsonSolver;
    std::ifstream jsonFile(configPath.c_str());
    jsonFile >> jsonSolver;
    jsonFile.close();
    LOG(INFO) << "======= Config Loading =======";
    try
    {
        _startIndex = jsonSolver.at("startIndex");
        _threadNum = jsonSolver.at("threadNum"); // TODO: 这部分用起来
        _level = jsonSolver.at("level");
        _numOfTk = jsonSolver.at("numOfTk");
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

        LOG(INFO) << "Start Index:\t" << _startIndex;
        LOG(INFO) << "Thread Num: \t" << _threadNum;
        LOG(INFO) << "Level:      \t" << _level;
        LOG(INFO) << "One Of TK:  \t" << _numOfTk;
        LOG(INFO) << "SaveDir:    \t" << _saveDir;
        LOG(INFO) << "keys:       \t" << _keys.size();
        for (int i = 0; i < _keys.size(); ++i)
        {
            LOG(INFO) << "            \t" << _keys.at(i);
        }
        LOG(INFO) << "Extent: \t\t" << _extent.minLng << " " << _extent.maxLng
                  << " " << _extent.minLat << " " << _extent.maxLat;
        LOG(INFO) << "======== Load Success ========";
        return true;
    }
    catch (...)
    {
        LOG(INFO) << "======== Load Failed =========";
        return false;
    }
}

TileIndex TianDiTuDownloader::lnglatToTileIndex(Lnglat lnglat, int level)
{
    double x = (lnglat.lng + 180) / 360;
    double y = (1 - std::log(std::tan(lnglat.lat * M_PI / 180) + 1 / std::cos(lnglat.lat * M_PI / 180)) / M_PI) / 2;
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
        LOG(INFO) << "[SUCCESS] tile " << level << "_" << xy.y << "_" << xy.x
                  << " saved";
        return true;
    }
    else
    {
        LOG(ERROR) << "[FAILED] tile " << level << "_" << xy.y << "_" << xy.x
                   << " can not download, due to: " << res->body;
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
    LOG(INFO) << "All tiles:  \t" << tileNum;

    std::string levelPath = _saveDir + "/" + std::to_string(_level);
    if (!std::filesystem::exists(std::filesystem::path(levelPath)))
    {
        std::filesystem::create_directory(std::filesystem::path(levelPath));
    }

    for (int i = _startIndex; i < tileNum; ++i)
    {
        TileIndex xy = {minXY.x + (i % (maxXY.x - minXY.x + 1)),
                        minXY.y + (i / (maxXY.x - minXY.x + 1))};

        // 按照Z（文件夹）/Y（文件夹）/X（文件夹）/tile.png的路径保存数据
        std::string filePath = levelPath + "/" + std::to_string(xy.y);
        if (!std::filesystem::exists(std::filesystem::path(filePath)))
        {
            std::filesystem::create_directory(std::filesystem::path(filePath));
        }
        filePath = filePath + "/" + std::to_string(xy.x);
        std::filesystem::create_directory(std::filesystem::path(filePath));
        filePath = filePath + "/" + "tile.png";
        TianDiTuDownloader::downloadTile(xy, _level, key, filePath);

        if (i == tileNum - 1)
        {
            LOG(INFO) << "======= All Tiles Saved ======";
        }
        else
        {
            if ((++nOfOneKey) == _numOfTk)
            {
                if (++keyIndex == _keys.size())
                {
                    LOG(INFO) << "======== All Keys Used =======";
                    _startIndex = ++i;
                    LOG(INFO) << "Next index: \t" << _startIndex;
                    break;
                }
                // 切换下一个key
                key = _keys[keyIndex];
                nOfOneKey = 0;
            }
        }
    }

    // 修改配置文件中的startIndex
    nlohmann::json jsonSolver;
    std::ifstream jsonFile(_configPath.c_str());
    jsonFile >> jsonSolver;
    jsonFile.close();
    jsonSolver["startIndex"] = _startIndex;
    std::ofstream jsonFileOut(_configPath.c_str());
    jsonFileOut << jsonSolver;
    jsonFileOut.close();

    LOG(INFO) << "========== Finished ==========";
}