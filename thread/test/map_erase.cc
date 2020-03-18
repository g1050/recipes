#include <iostream>
#include <map>

using namespace std;;

int main()
{
    map<int,int> mp;
    mp[1024] = 1023;

    cout << mp.size() << endl;
    mp.erase(1024);
    cout << mp.size() << endl;
    return 0;
}

