#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include "nlohmann/json.hpp"
#include "httplib.h"
#include "tianditu_downloader.h"

#define M_PI 3.14159265358979323846

#define FAILED_PATH "failedList.txt"

#define PR_STARTINDEX "startIndex"
#define PR_LEVEL "level"
#define PR_NUMOFTK "numOfTk"
#define PR_SAVEDIR "saveDir"
#define PR_KEYS "keys"
#define PR_TILEMODE "tileMode"
#define PR_EXTENT "extent"

TileIndex TianDiTuDownloader::lnglatToTileIndex(Lnglat lnglat, int level)
{
    double x = (lnglat.lng + 180) / 360;
    double y = (1 - std::log(std::tan(lnglat.lat * M_PI / 180) + 1 / std::cos(lnglat.lat * M_PI / 180)) / M_PI) / 2;
    double titleX = std::floor(x * pow(2, level));
    double titleY = std::floor(y * pow(2, level));
    return TileIndex{static_cast<int>(titleX), static_cast<int>(titleY), level};
}

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
        _startIndex = jsonSolver.at(PR_STARTINDEX);
        _level = jsonSolver.at(PR_LEVEL);
        _numOfTk = jsonSolver.at(PR_NUMOFTK);
        _saveDir = jsonSolver.at(PR_SAVEDIR);
        std::filesystem::path path(_saveDir);
        if (!std::filesystem::exists(path))
        {
            std::filesystem::create_directory(path);
        }
        _keys.resize(jsonSolver.at(PR_KEYS).size());
        for (int i = 0; i < jsonSolver.at(PR_KEYS).size(); ++i)
        {
            _keys[i] = jsonSolver.at(PR_KEYS).at(i);
        }
        _tileMode = jsonSolver.at(PR_TILEMODE) == "ZYX" ? TileMode::ZYX : TileMode::ZXY;
        _extent = {jsonSolver.at(PR_EXTENT)[0],
                   jsonSolver.at(PR_EXTENT)[1],
                   jsonSolver.at(PR_EXTENT)[2],
                   jsonSolver.at(PR_EXTENT)[3]};

        LOG(INFO) << "Start Index:\t" << _startIndex;
        LOG(INFO) << "Level:      \t" << _level;
        LOG(INFO) << "One Of TK:  \t" << _numOfTk;
        LOG(INFO) << "SaveDir:    \t" << _saveDir;
        LOG(INFO) << "keys:       \t" << _keys.size();
        for (int i = 0; i < _keys.size(); ++i)
        {
            LOG(INFO) << "            \t" << _keys.at(i);
        }
        LOG(INFO) << "Tile Mode:  \t" << (_tileMode == TileMode::ZYX ? "ZYX" : "ZXY");
        LOG(INFO) << "Extent:     \t" << _extent.minLng << " " << _extent.maxLng << " " << _extent.minLat << " "
                  << _extent.maxLat;
        LOG(INFO) << "======== Load Success ========";
        return true;
    }
    catch (...)
    {
        LOG(INFO) << "======== Load Failed =========";
        return false;
    }
}

bool TianDiTuDownloader::requireTile(TileIndex xyz, const std::string &key, const std::string &path)
{
    // 请求头
    httplib::Headers headers;
    headers.insert({"Accept", "text/html,application/xhtml+xml,application/xml;"});
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
    params.insert({"TILEMATRIX", std::to_string(xyz.z)});
    params.insert({"TILEROW", std::to_string(xyz.y)});
    params.insert({"TILECOL", std::to_string(xyz.x)});
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
        LOG(INFO) << "[SUCCESS] tile " << xyz.z << "_" << xyz.y << "_" << xyz.x << " saved";
        return true;
    }
    else
    {
        _failedList.push(xyz);
        LOG(ERROR) << "[FAILED] tile " << xyz.z << "_" << xyz.y << "_" << xyz.x
                   << " can not download, due to: " << res->body;
        return false;
    }
}

void TianDiTuDownloader::run()
{
    int nOfOneKey = 0;
    int keyIndex = 0;
    std::string key = _keys[keyIndex];

    std::string levelPath = _saveDir + "/" + std::to_string(_level);
    if (!std::filesystem::exists(std::filesystem::path(levelPath)))
    {
        std::filesystem::create_directory(std::filesystem::path(levelPath));
    }

    LOG(INFO) << "======= Reloading ======";
    // 先下载失败的瓦片
    size_t n = _failedList.size();
    LOG(INFO) << "Reload tiles:  \t" << n;
    for (int i = 0; i < n; ++i)
    {
        TileIndex xyz = _failedList.front();
        downloadTile(xyz, key, levelPath);
        _failedList.pop();
    }

    LOG(INFO) << "======= Downloading ======";
    // 再继续下载
    Lnglat minLnglat = {_extent.minLng, _extent.minLat};
    Lnglat maxLnglat = {_extent.maxLng, _extent.maxLat};
    TileIndex minXY = TianDiTuDownloader::lnglatToTileIndex(minLnglat, _level);
    TileIndex maxXY = TianDiTuDownloader::lnglatToTileIndex(maxLnglat, _level);
    int tileNum = (maxXY.x - minXY.x + 1) * (maxXY.y - minXY.y + 1);
    LOG(INFO) << "All tiles:  \t" << tileNum;

    for (int i = _startIndex; i < tileNum; ++i)
    {
        TileIndex xyz = {minXY.x + (i % (maxXY.x - minXY.x + 1)), minXY.y + (i / (maxXY.x - minXY.x + 1)), _level};
        downloadTile(xyz, key, levelPath);

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
    jsonSolver[PR_STARTINDEX] = _startIndex;
    std::ofstream jsonFileOut(_configPath.c_str());
    jsonFileOut << jsonSolver;
    jsonFileOut.close();

    // 保存失败的瓦片
    encodeErrorFile();

    LOG(INFO) << "========== Finished ==========";
}

void TianDiTuDownloader::downloadTile(TileIndex xyz, const std::string &key, const std::string &levelPath)
{
    std::string filePath = levelPath;
    if (_tileMode == TileMode::ZYX)
    {
        filePath = filePath + "/" + std::to_string(xyz.y);
        if (!std::filesystem::exists(std::filesystem::path(filePath)))
        {
            std::filesystem::create_directory(std::filesystem::path(filePath));
        }
        filePath = filePath + "/" + std::to_string(xyz.x);
    }
    else
    {
        filePath = filePath + "/" + std::to_string(xyz.x);
        if (!std::filesystem::exists(std::filesystem::path(filePath)))
        {
            std::filesystem::create_directory(std::filesystem::path(filePath));
        }
        filePath = filePath + "/" + std::to_string(xyz.y);
    }
    std::filesystem::create_directory(std::filesystem::path(filePath));
    filePath = filePath + "/" + "tile.png";
    TianDiTuDownloader::requireTile(xyz, key, filePath);
}

void TianDiTuDownloader::encodeErrorFile()
{
    std::ofstream out(FAILED_PATH);
    while (!_failedList.empty())
    {
        TileIndex xyz = _failedList.front();
        out << xyz.x << "," << xyz.y << "," << xyz.z << std::endl;
        _failedList.pop();
    }
    out.close();
}

void TianDiTuDownloader::decodeErrorFile()
{
    std::ifstream in(FAILED_PATH);
    std::string line;
    while (getline(in, line))
    {
        TileIndex xyz{atoi(line.substr(0, line.find(",")).c_str()),
                      atoi(line.substr(line.find(",") + 1, line.rfind(",") - line.find(",") - 1).c_str()),
                      atoi(line.substr(line.rfind(",") + 1).c_str())};
        _failedList.push(xyz);
    }
    in.close();
}