#pragma once

#ifndef TIANDITU_DOWNLOAD_H
#define TIANDITU_DOWNLOAD_H

#include <iostream>
#include <string>
#include <vector>
#include "easylogging++.h"

struct Lnglat
{
    double lng;
    double lat;
};

struct TileIndex
{
    int x;
    int y;
};

struct Extent
{
    double minLng;
    double maxLng;
    double minLat;
    double maxLat;
};

class TianDiTuDownloader
{
public:
    TianDiTuDownloader() {}
    ~TianDiTuDownloader() = default;

    const int startIndex() const { return _startIndex; }
    const int threadNum() const { return _threadNum; }
    const int level() const { return _level; }
    const std::string &saveDir() const { return _saveDir; }
    const std::vector<std::string> &keys() const { return _keys; }
    const Extent &extent() const { return _extent; }

    bool loadConfig(const std::string &configPath);
    static TileIndex lnglatToTileIndex(Lnglat lnglat, int level);
    static bool downloadTile(TileIndex xy, int level, const std::string &key,
                             const std::string &path);
    void run();

private:
    int _startIndex = 0;
    int _threadNum = 1;
    int _level = 1;
    int _numOfTk = 10000;
    std::string _configPath = "./config.json";
    std::string _saveDir = "./";
    std::vector<std::string> _keys = {};
    Extent _extent = {0, 0, 0, 0};
};

#endif // TIANDITU_DOWNLOAD_H