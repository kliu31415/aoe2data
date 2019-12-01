#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <iomanip>
#include <chrono>
#include <thread>
#include <sstream>
#include <curl/curl.h>
#include <ext/pb_ds/assoc_container.hpp>
#define NUM_CIVS 19
using namespace std;
using namespace __gnu_pbds;
template<class T> string to_str(T x)
{
    stringstream ss;
    ss << x;
    return ss.str();
}
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
void fetch(string url, string &data)
{
    CURL *curl;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    CURLcode err = curl_easy_perform(curl);
    if(err != CURLE_OK)
        cout << "CURL error: " << curl_easy_strerror(err) << endl;
    curl_easy_cleanup(curl);
}
void showProgress(int cur, int n, int W = 50)
{
    n = max(n, 1);
    cout << "\r[";
    for(int i=0; i<cur*W/n; i++)
        cout << "=";
    for(int i=0; i<W - cur*W/n; i++)
        cout << " ";
    cout << "] " << cur*100/n << "%";
    cout.flush();
}
string civNames[NUM_CIVS + 100];
struct Player
{
    int civ, rating;
    Player(){}
    Player(int rating, int civ)
    {
        this->rating = rating;
        this->civ = civ;
    }
    string getCivName()
    {
        return civNames[civ];
    }
    static string getCivName(int c)
    {
        return civNames[c];
    }
    static void initCivs()
    {
        civNames[0] = "N/A";
        civNames[1] = "Britons";
        civNames[2] = "Franks";
        civNames[3] = "Goths";
        civNames[4] = "Teutons";
        civNames[5] = "Japanese";
        civNames[6] = "Chinese";
        civNames[7] = "Byzantines";
        civNames[8] = "Persians";
        civNames[9] = "Saracens";
        civNames[10] = "Turks";
        civNames[11] = "Vikings";
        civNames[12] = "Mongols";
        civNames[13] = "Celts";
        civNames[14] = "Spanish";
        civNames[15] = "Aztecs";
        civNames[16] = "Mayans";
        civNames[17] = "Huns";
        civNames[18] = "Koreans";
    }
};
struct Game
{
    vector<Player> winners, losers;
    string mapName, ladderName, gameMod;
    int duration, matchID;
    Game(){}
    Game(ifstream &fin)
    {
        int wsize, lsize;
        string s;
        getline(fin, s);
        matchID = atoi(s.c_str());
        getline(fin, ladderName);
        getline(fin, gameMod);
        getline(fin, mapName);
        getline(fin, s);
        duration = atoi(s.c_str());
        getline(fin, s);
        wsize = atoi(s.c_str());
        getline(fin, s);
        lsize = atoi(s.c_str());

        winners.resize(wsize);
        losers.resize(lsize);
        for(auto &i: winners)
        {
            getline(fin, s);
            i.civ = atoi(s.c_str());
            getline(fin, s);
            i.rating = atoi(s.c_str());
        }
        for(auto &i: losers)
        {
            getline(fin, s);
            i.civ = atoi(s.c_str());
            getline(fin, s);
            i.rating = atoi(s.c_str());
        }
    }
    int getNumPlayers()
    {
        return winners.size() + losers.size();
    }
    int getMatchRating()
    {
        int res = 0;
        for(auto &i: winners)
            res += i.rating;
        for(auto &i: losers)
            res += i.rating;
        return res / (winners.size() + losers.size());
    }
    bool isValidGame()
    {
        for(auto &i: winners)
            if(i.civ<1 || i.civ>=NUM_CIVS)
                return false;
        for(auto &i: losers)
            if(i.civ<1 || i.civ>=NUM_CIVS)
                return false;
        return getNumPlayers() > 0 && winners.size() == losers.size();
    }
    void printInfo()
    {
        if(!isValidGame())
        {
            cout << "Invalid game" << endl;
            return;
        }
        cout << "Match ID: " << matchID << "\n";
        cout << "Ladder: " << ladderName << "\n";
        cout << "Game Mod: " << gameMod << "\n";
        cout << "Map: " << mapName << "\n";
        cout << "Duration: " << duration << "\n";
        cout << "Winning team members:\n";
        cout.setf(ios::left);
        for(auto &i: winners)
            cout << setw(11) << i.getCivName() << i.rating << "\n";
        cout << "Losing team members:\n";
        for(auto &i: losers)
            cout << setw(11) << i.getCivName() << i.rating << "\n";
        cout.flush();
    }
    void outputToFile(ofstream &fout)
    {
        if(!isValidGame())
            return;
        fout << matchID << "\n" << ladderName << "\n" << gameMod << "\n" << mapName << "\n" << duration << "\n";
        fout << winners.size() << "\n" << losers.size() << "\n";
        for(auto &i: winners)
            fout << i.civ << "\n" << i.rating << "\n";
        for(auto &i: losers)
            fout << i.civ << "\n" << i.rating << "\n";
    }
    void outputToFile(string s)
    {
        ofstream fout(s.c_str(), ios::app);
        outputToFile(fout);
    }
    bool operator == (const Game &x) const
    {
        return matchID == x.matchID;
    }
    bool operator < (const Game &x) const
    {
        return matchID < x.matchID;
    }
};
void printWinRates(vector<Game> &games, string ladder = "", string mapName = "", int minRating = 0, int durMin = 0, int durMax = 1e9)
{
    sort(games.begin(), games.end());
    games.resize(unique(games.begin(), games.end()) - games.begin());
    int played[NUM_CIVS]{}, won[NUM_CIVS]{};
    int numGames = 0;
    for(auto &i: games)
    {
        if(i.isValidGame() && (ladder=="" or ladder==i.ladderName) && (mapName=="" or mapName==i.mapName) && i.getMatchRating()>=minRating
        && i.duration>=durMin && i.duration<=durMax)
        {
            for(auto &j: i.winners)
            {
                won[j.civ]++;
                played[j.civ]++;
            }
            for(auto &j: i.losers)
            {
                played[j.civ]++;
            }
            numGames++;
        }
    }
    cout << "n = " << numGames << "\n";
    cout.setf(ios::left);
    for(int i=1; i<NUM_CIVS; i++)
    {
        if(played[i] == 0)
            cout << setw(11) << Player::getCivName(i) << " N/A\n";
        else cout << setw(11) << Player::getCivName(i) << " " << won[i] * 100 / played[i] << "%\n";
    }
    cout.flush();
}
string tillChar(string &s, unsigned pos, char c)
{
    string res;
    while(s[pos] != c)
        res += s[pos++];
    return res;
}
vector<string> split(string &s, char c = ' ')
{
    vector<string> res;
    int prv = 0;
    s += c;
    for(int i=0; i<s.size(); i++)
    {
        if(s[i] == c)
        {
            if(i != prv)
                res.push_back(s.substr(prv, i-prv));
            prv = i+1;
        }
    }
    s.pop_back();
    return res;
}
int time_str_to_int(string s)
{
    auto x = split(s, ':');
    reverse(x.begin(), x.end());
    int mult = 1;
    int res = 0;
    for(auto &i: x)
    {
        res += atoi(i.c_str()) * mult;
        mult *= 60;
    }
    return res;
}
Game ExtractGameDataFromMatchHTML(string &s)
{
    Game g;
    string patLadder = "Age-of-Empires-II-The-Conquerors/";
    string patMap = "text-align: right;\">";
    string patColWin = "#00A651\">";
    string patColLose = "#FF0000\">";
    string patCiv = "/AOC/civs/";
    string patNewRating = "New Rating: <b>";
    string patMatch = "/match/view/";
    string patDuration = "#EDF3F9\">";
    string patGameMod = patDuration; //they're the same
    auto pos = s.find(patLadder);
    if(pos == string::npos) //not aoe2
        return g;
    pos += patLadder.size();
    g.ladderName = tillChar(s, pos, '\"');
    pos = s.find(patMatch, pos);
    if(pos == string::npos) // no match id
        return g;
    pos += patMatch.size();
    g.matchID = atoi(tillChar(s, pos, '/').c_str());
    pos = s.find(patMap, pos);
    if(pos == string::npos) //no map... bugged???
        return g;
    pos += patMap.size();
    g.mapName = tillChar(s, pos, '<');
    for(int i=0; i<2; i++)
    {
        pos = s.find(patDuration, pos);
        if(pos == string::npos) //no duration?
            return g;
        pos += patDuration.size();
    }
    g.duration = time_str_to_int(tillChar(s, pos, '<'));
    for(int i=0; i<2; i++)
    {
        pos = s.find(patGameMod, pos);
        if(pos == string::npos) //no duration?
            return g;
        pos += patGameMod.size();
    }
    g.gameMod = tillChar(s, pos, '<');
    if(s.find("(Computer)", pos) != string::npos) //no computers
        return Game();
    while(true) //grab winners
    {
        unsigned prvpos = pos;
        pos = s.find(patNewRating, pos);
        assert(pos != string::npos);
        pos += patNewRating.size();
        int rating = atoi(tillChar(s, pos, '<').c_str());
        auto nxt = s.find(patColWin, pos);
        if(nxt == string::npos) //no more green text, so no more winners
        {
            pos = prvpos;
            break;
        }
        nxt += patColWin.size(); //patColWin.size() == patColLose.size() so it doesn't matter

        rating -= atoi(tillChar(s, nxt, '<').c_str()); //calculate the rating before this game happened
        pos = s.find(patCiv, nxt) + patCiv.size();
        int civ = atoi(tillChar(s, pos, '.').c_str());

        g.winners.emplace_back(rating, civ);
    }
    while(true)
    {
        pos = s.find(patCiv, pos);
        if(pos == string::npos)
            return g;
        pos += patCiv.size();
        int civ = atoi(tillChar(s, pos, '.').c_str());
        pos = s.find(patColLose, pos) + patColLose.size();
        int rating = -atoi(tillChar(s, pos, '<').c_str());
        pos = s.find(patNewRating, pos) + patNewRating.size();
        rating += atoi(tillChar(s, pos, '<').c_str());
        g.losers.emplace_back(rating, civ);
    }
}
int main(int argc, char **argv)
{
    ios::sync_with_stdio(false);
    curl_global_init(CURL_GLOBAL_ALL);
    Player::initCivs();
    int orig = 17411000;
    int n = 0;
    if(argc >= 2)
        orig = atoi(argv[1]);
    if(argc >= 3)
        n = atoi(argv[2]);
    ifstream fin("data.txt");
    gp_hash_table<int, null_type> matchIDs;
    vector<Game> games;
    if(!fin.fail())
    {
        while(!fin.eof())
        {
            Game g(fin);
            games.push_back(g);
            matchIDs.insert(g.matchID);
        }
        fin.close();
    }
    ofstream fout("data.txt", ios::app);
    showProgress(0, n);
    for(int i=0; i<n; i++)
    {
        if(matchIDs.find(orig + i) != matchIDs.end())
            continue;
        string url = "https://www.voobly.com/match/view/" + to_str(orig + i);
        string data;
        fetch(url, data);
        Game g = ExtractGameDataFromMatchHTML(data);
        games.push_back(g);
        g.outputToFile(fout);
        showProgress(i, n);
    }
    showProgress(n, n);
    fout.close();
    cout << endl;
    printWinRates(games, "", "Black Forest", 0);
    return 0;
}
