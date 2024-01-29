#include "tianditu_downloader.h"

INITIALIZE_EASYLOGGINGPP

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Invalid argument of " << argv[0] << "\n";
        std::cout << "Run " << argv[0] << " -h for help" << std::endl;
        return -1;
    }
    else
    {
        if (strcmp(argv[1], "-h") == 0)
        {
            std::cout << "Usage: " << argv[0] << " <configPath>\n";
            std::cout << "\t configPath: Path of config file.\n";
            std::cout << "Example: " << argv[0] << " config.json" << std::endl;
            return 0;
        }
        else
        {
            TianDiTuDownloader tdtLoader;
            if (!tdtLoader.loadConfig(argv[1]))
            {
                return -1;
            }
            tdtLoader.run();
        }
    }
    return 0;
}
