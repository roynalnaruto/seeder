# Seeder Server

## compile MacOS
`g++ server.cpp -std=c++11 -lboost_system -lboost_thread-mt -o server.out`

`g++ client.cpp -std=c++11 -lboost_system -lboost_thread-mt -o client.out`

# compile linux
`g++ server.cpp -std=c++11 -lboost_system -lboost_thread -pthread -o server.out`

`g++ client.cpp -std=c++11 -lboost_system -lboost_thread -pthread -o client.out`

or using one script

`./build_server_client.sh`

## run
server runs on port `8001`

run `./client.out` through multiple terminals
