
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <exception>
#include <random>
#include <iostream>
#include <ctime>
#include <chrono>
#include <thread>
#include <set>
#include <deque>
#include <map>


#pragma once





struct Map { // coordinates are indexed by a single integer x, where w = x % w_ and h = x / h_
    int h_;
    int w_;
    int max_len_;
    std::vector<bool> walls_; // walls_[i] == true if an obstacle is at i-th coord
    std::vector<int> starts_; // integer at i-th index equals the coordinate of the start of i-th agent
    std::vector<int> ends_;
    std::vector<bool> filled_spaces_;
    std::random_device rd_;
    std::mt19937 rand_engine_;
    std::vector<std::vector<int>> configs_; // configs_[i] contains coordinates of all agents at time [i]
    std::vector<std::set<int>> reservation_table_; //reservation_table_[a] contains values of coordinates that are occupied by an agent at time a



    int size() const { return w_ * h_; }




    Map(const std::string& file) : h_(0), w_(0), rd_(std::random_device()), rand_engine_(rd_())/*, reservation_table_(1)*/ {
        std::string filename = file;
        std::ifstream inFile(filename);
        if(!inFile.is_open() || !inFile)
            throw std::runtime_error("Cannot open map file");
        std::string row;
        getline(inFile, row);
        w_ = (int)row.size();
        h_++;
        for(size_t i = 0; i < w_; ++i){
            if(row[i] == ' ')
                walls_.push_back(false);
            else
                walls_.push_back(true);
        }
        row.clear();

        while(getline(inFile, row) && !row.empty()){
            h_++;
            for(size_t i = 0; i < w_; ++i){
                if(row[i] == ' ')
                    walls_.push_back(false);
                else
                    walls_.push_back(true);
            }
            row.clear();
        }
        filled_spaces_ = walls_;
        /*inFile.close();
        for(int s : starts_){
            reservation_table_[0].insert(s);
        }*/

    }
    int getRandomEmptyCoord(){
        std::uniform_int_distribution<int> distr(0, (w_ * h_) - 1);
        int res = 0;
        while(filled_spaces_[res]){
            res = distr(rand_engine_);
        }
        return res;
    }


    Map& addRandomAgents(int nrAgents){
        int coord;
        if(nrAgents >= h_ * w_ / 2)
            throw std::logic_error("too many agents");
        for(int i = 0; i < nrAgents; ++i){
            coord = getRandomEmptyCoord();
            starts_.push_back(coord);
            filled_spaces_[coord] = true;
            coord = getRandomEmptyCoord();
            ends_.push_back(coord);
            filled_spaces_[coord] = true;
        }
        return *this;
    }

    void solve(){
        std::vector<int> v;
        print(v);
        bool solution = mapf();
        if(!solution){
            std::cout << "Unable to find a solution." << std::endl;
            return;
        }
        print_anim();
    }


    void print_anim(){
        for(int i = 0; i < max_len_; ++i){
            //system("clear");
            std::cout << "\033[2J\033[1;1H";
            std::vector<int> v;
            for(auto& config : configs_){
                if(config.size() < i)
                    v.push_back(-1);
                else
                    v.push_back(config[i]);
            }
            //std::cout << std::endl;
            print(v);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }


    Map& loadAgents(const std::string& file){
        std::string filename = file;
        std::ifstream inFile(filename);
        if(!inFile.is_open() || !inFile)
            throw std::runtime_error("Cannot open agent file");
        std::string line;
        int s_x, s_y, e_x, e_y, start, end;
        while(getline(inFile, line) && line[0] >= '0' && line[0] <= '9'){
            std::stringstream ss;
            ss.str(line);
            ss >> s_x >> s_y >> e_x >> e_y;
            if(s_x >= w_ || s_y >= h_ || e_x >= w_ || e_y >= h_)
                throw std::out_of_range("agent start/end coordinates out of bounds");
            start = s_x + s_y * w_;
            end = e_x + e_y * w_;
            starts_.push_back(start);
            ends_.push_back(end);
            line.clear();
        }
        return *this;
    }


    void print(std::vector<int>& config){
        std::vector<char> pic(w_ * h_, ' ');
        for(int i = 0; i < w_ * h_; ++i){
            if(walls_[i])
                pic[i] = '#';
        }
        for(int start : starts_){
            pic[start] = 'S';
        }
        for(int end : ends_){
            pic[end] = 'E';
        }
        for(int i = 0; i < config.size(); ++i){
            if(config[i] != -1)
                pic[config[i]] = (char)('a' + i);
            else
                (pic[ends_[i]]) = (char)('a' + i);
        }
        for(int r = 0; r < h_;  r++){
            for(int c = 0; c < w_; ++c){
                std::cout << pic[(r * w_) + c];
            }
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

    std::vector<int> neighbors(int coord){
        std::vector<int> res;

        if(coord > 0 && !walls_[coord - 1])
            res.push_back(coord - 1);
        if(coord + 1 < (w_ * h_) && !walls_[coord + 1])
            res.push_back(coord + 1);
        if(coord - w_ >= 0 && !walls_[coord - w_])
            res.push_back(coord - w_);
        if(coord + w_ < w_ * h_ && !walls_[coord + w_])
            res.push_back(coord + w_);
        res.push_back(coord);
        return res;
    }

    int manhattan_dist(int a, int b) const{
        if(a == b)
            return 0;
        int a_x = a % w_;
        int a_y = a / w_;
        int b_x = b % w_;
        int b_y = b / w_;
        return (int)(abs(a_x - b_x)) + (int)(abs(a_y - b_y));
    }

    std::vector<int> astar(int agentId){
        int start = starts_[agentId];
        int end = ends_[agentId];
        std::set<std::pair<int, int>> closed;
        std::deque<std::pair<int, int>> open; //first int is coord, second is path length
        int curLen = 0;
        open.push_back({start, curLen});
        std::map<std::pair<int, int>, std::pair<int, int>> edges;
        int cur_coord;
        while(!open.empty()){
            cur_coord = open.front().first;
            curLen = open.front().second;
            open.pop_front();
            if(cur_coord == end)
                break;
            for(int adj : neighbors(cur_coord)){
                if(closed.count({adj, curLen}) || (curLen + 1 < reservation_table_.size() && reservation_table_[curLen + 1].count(adj)))
                    continue;
                bool b = true;
                for(auto c : open)
                    if(c.first == adj)
                        b = false;
                if(!b)
                    continue;
                auto it = std::lower_bound(open.begin(), open.end(), std::make_pair(adj, curLen + 1), [&](auto a, auto b){
                    return manhattan_dist(a.first, end)/* + 30 * (a.first == cur_coord)*/ + a.second < manhattan_dist(b.first, end)/* + 30 * (b.first == cur_coord)*/ + b.second;});
                open.insert(it, {adj, curLen + 1});
                edges[{adj, curLen + 1}] = {cur_coord, curLen};
            }
            closed.insert({cur_coord, curLen});
            if(curLen > 200)
                break;
        }
        if(cur_coord != end){
            return {};
        }
        std::deque<int> path;
        path.push_back(end);
        std::pair<int, int> cur(end, curLen);
        while(edges.count(cur)){
            cur = edges[cur];
            path.push_front(cur.first);
        }
        std::vector<int> res;
        for(auto it = path./*r*/begin(); it != path./*r*/end(); ++it)
            res.push_back(*it);
        return res;
    }

    //using cooperative A* algorithm described here: https://www.davidsilver.uk/wp-content/uploads/2020/03/coop-path-AIIDE.pdf
    bool mapf(){
        std::vector<int> agent_order(starts_.size());
        for(int i = 0; i < starts_.size(); ++i)
            agent_order[i] = i;
        //configs_.resize(starts_.size());
        int nrIt = 0;
        while(true) {
            reservation_table_.clear();
            reservation_table_.push_back({});
            for(int s : starts_){
                reservation_table_[0].insert(s);
            }
            configs_.clear();
            configs_.resize(starts_.size());
            std::vector<int> path;
            std::shuffle(agent_order.begin(), agent_order.end(), rand_engine_);
            for(int agent: agent_order){
                path = astar(agent);
                if (path.empty())
                    break;
                configs_[agent] = path;
                if(reservation_table_.size() <= path.size()) {
                    reservation_table_.resize(path.size());
                    max_len_ = (int)path.size();
                    /*while(reservation_table_.size() != max_len_)
                        reservation_table_.push_back({ends_[agent]});*/
                }
                for(int i = 0; i < path.size(); ++i){
                    reservation_table_[i].insert(path[i]);
                }
                for(int i = 1; i < path.size(); ++i){
                    reservation_table_[i - 1].insert(path[i]);
                }
                for(int i = 0; i < configs_.size(); ++i){
                    if(!configs_[i].empty()){
                        while(configs_[i].size() != max_len_) {
                            reservation_table_[configs_[i].size()].insert(ends_[i]);
                            configs_[i].push_back(ends_[i]);
                        }
                    }
                }
            }
            if(path.empty()){
                if(++nrIt > 100)
                    return false;
                continue;
            }




            break;
        }
        std::cout << "solution found!" << std::endl;
        return true;
    }

};
