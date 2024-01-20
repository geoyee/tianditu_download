#include "tianditu_downloader.h"

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cout << "please input config path" << std::endl;
        return -1;
    }

    TianDiTuDownloader tdtLoader;
    if (!tdtLoader.loadConfig(argv[1]))
    {
        return -1;
    }

    tdtLoader.run();

    return 0;
}