//
// Created by wty on 5/5/23.
//

#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *opt = "c:s:d";
    char c;
    while ((c = getopt(argc, argv, opt)) != -1)
    {
        switch (c) {
            case 'c':
                std::cout << "c param: " << atoi(optarg) << std::endl;
                break;
            case 's':
                std::cout << "s param: " << atoi(optarg) << std::endl;
                break;
            case 'd':
                std::cout << "d param: " << atoi(optarg) << std::endl;
                break;
        }
    }
    return 0;
}