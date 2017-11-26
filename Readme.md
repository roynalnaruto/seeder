# Seeder Server

## compile
g++ server.cpp -lboost_system -lboost_thread-mt -o server.out
g++ client.cpp -lboost_system -lboost_thread-mt -o client.out

## run
server runs on port 8001
run ./client.out through multiple terminals
