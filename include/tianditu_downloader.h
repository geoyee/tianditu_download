#pragma once

#ifndef TIANDITU_DOWNLOAD_H
#define TIANDITU_DOWNLOAD_H

#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include "easylogging++.h"

#define FAILD_PATH "./failedList.txt"

struct Lnglat
{
    double lng;
    double lat;
};

struct TileIndex
{
    int x;
    int y;
    int z;
};

struct Extent
{
    double minLng;
    double maxLng;
    double minLat;
    double maxLat;
};

enum class TileMode
{
    ZYX,
    ZXY
};

class TianDiTuDownloader
{
public:
    TianDiTuDownloader()
    {
        decodeErrorFile();
    }

    ~TianDiTuDownloader() = default;

    const int startIndex() const
    {
        return _startIndex;
    }

    const int level() const
    {
        return _level;
    }

    const std::string &saveDir() const
    {
        return _saveDir;
    }

    const std::vector<std::string> &keys() const
    {
        return _keys;
    }

    const Extent &extent() const
    {
        return _extent;
    }

    static TileIndex lnglatToTileIndex(Lnglat lnglat, int level);

    bool loadConfig(const std::string &configPath);
    bool requireTile(TileIndex xyz, const std::string &key, const std::string &path);
    void run();

private:
    void downloadTile(TileIndex xyz, const std::string &key, const std::string &levelPath);
    void encodeErrorFile();
    void decodeErrorFile();

    std::queue<TileIndex> _failedList = {};
    std::string _configPath = "./config.json";
    int _startIndex = 0;
    int _level = 1;
    int _numOfTk = 10000;
    std::string _saveDir = "./datas";
    std::vector<std::string> _keys = {};
    TileMode _tileMode = TileMode::ZYX;
    Extent _extent = {0, 0, 0, 0};
};

#endif // TIANDITU_DOWNLOAD_H
