#include <iostream>
#include <cassert>
#include "Map.h"

int main(int argc, char** argv) {
    if(argc != 4){
        std::cout << "Usage: ./MAPF <map filename> {<random> <nr>} / {<saved> <agent preset filename>}" << std::endl;
        return -1;
    }

    std::string map_file(argv[1]);
    Map m(map_file);
    std::string agent_option(argv[2]);
    if(agent_option == "random")
        m.addRandomAgents(std::atoi(argv[3]));
    else
        m.loadAgents(std::string(argv[3]));
    m.solve();



    //Map m("maps/6.txt");
    //m.addRandomAgents(6);
    //m.loadAgents("agents/6.txt");
    /*std::vector<int> v;
    m.print(v);
    v = m.astar(0);
    m.print(v);*/

    //m.mapf();

    //assert(v == m.configs_[0]);

    //m.print_anim();
    //m.solve();


    return 0;
}
