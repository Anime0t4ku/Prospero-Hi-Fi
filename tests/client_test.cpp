#include "remote.hpp"
#include "client.hpp"
#include "websocket.hpp"
#include <cassert>
#include <chrono>
#include <thread>
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
namespace nxh {
bool listDirectory(const Source &, const std::string &, std::vector<Entry> &rows, std::string &) {
    rows = {{"Song", "song.flac", false}};
    return true;
}
} // namespace nxh
using namespace nxh;
template <class F> void await(F f) {
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (!f()) {
        assert(std::chrono::steady_clock::now() < end);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}
int main() {
    assert(websocketAccept("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
    assert(websocketKey().size() == 24);
    int pair[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
    std::string big(70000, 'x');
    std::thread sender([&] { assert(websocketWrite(pair[0], 1, big, true)); });
    unsigned opcode = 0;
    std::string data;
    assert(websocketRead(pair[1], opcode, data, true) && opcode == 1 && data == big);
    sender.join();
    close(pair[0]);
    close(pair[1]);
    Settings settings;
    settings.musicRoot = "sdmc:/music";
    auto makeServer = [&](int port) {
        return std::make_unique<RemoteServer>(
            port, [](auto &j) { return nlohmann::json{{"ok", true}, {"received", j}}; });
    };
    auto server = makeServer(0);
    assert(server->valid());
    int port = server->port();
    server->publish({{"settings", settingsDocument(settings)}, {"revision", 1}, {"title", "first"}},
                    settings, 1);
    ClientSession client({"Docked", "127.0.0.1", "nx", port});
    await([&] {
        return client.connected() && client.snapshot().value("title", std::string{}) == "first";
    });
    auto start = std::chrono::steady_clock::now();
    server->publish(
        {{"settings", settingsDocument(settings)}, {"revision", 2}, {"title", "second"}}, settings,
        1);
    await([&] { return client.snapshot().value("title", std::string{}) == "second"; });
    assert(std::chrono::steady_clock::now() - start < std::chrono::milliseconds(300));
    // Browser subscribers get small deltas; older NX clients continue receiving full state.
    nlohmann::json catalog={{"revision",1},{"albums",nlohmann::json::array()}};
    for(int i=0;i<1000;i++)catalog["albums"].push_back({{"title","Cached album "+std::to_string(i)}});
    server->publish({{"settings",settingsDocument(settings)},{"catalog",catalog},{"title","second"}},settings,1);
    int browser=socket(AF_INET,SOCK_STREAM,0);assert(browser>=0);
    timeval timeout{3,0};setsockopt(browser,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof timeout);
    sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(port);
    assert(connect(browser,reinterpret_cast<sockaddr*>(&address),sizeof address)==0);
    std::string upgrade="GET /ws?delta=1 HTTP/1.1\r\nHost: 127.0.0.1:"+std::to_string(port)+"\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    assert(send(browser,upgrade.data(),upgrade.size(),0)==ssize_t(upgrade.size()));
    std::string header;char byte;while(header.find("\r\n\r\n")==header.npos){assert(recv(browser,&byte,1,0)==1);header+=byte;}
    assert(header.find("101 Switching")!=header.npos);
    assert(websocketRead(browser,opcode,data,false));assert(nlohmann::json::parse(data)["catalog"]["albums"].size()==1000);
    server->publish({{"title","third"},{"playback",{{"position",2}}}},settings,1);
    assert(websocketRead(browser,opcode,data,false));auto delta=nlohmann::json::parse(data);
    assert(delta["update"]["title"]=="third"&&!delta["update"].contains("catalog")&&data.size()<1024);
    await([&]{auto value=client.snapshot();return value.value("title",std::string{})=="third"&&value["catalog"]["albums"].size()==1000&&value.contains("settings");});
    settings.volume=77;server->publish({{"settings",settingsDocument(settings)},{"catalog",catalog},{"title","fourth"}},settings,1);
    assert(websocketRead(browser,opcode,data,false));assert(nlohmann::json::parse(data)["settings"]["volume"]==77);
    websocketWrite(browser,8,"",true);close(browser);
    auto id = client.request("POST", "/api/cmd", {{"op", "playpause"}});
    ClientSession::Result result;
    await([&] { return client.poll(result); });
    assert(result.id == id && result.ok && result.data["received"]["op"] == "playpause");
    id = client.request("POST", "/api/browse",
                        {{"source", -1}, {"path", ""}, {"sources_revision", 1}});
    await([&] { return client.poll(result); });
    assert(result.ok && result.data["entries"][0]["path"] == "song.flac");
    server.reset();
    await([&] { return !client.connected(); });
    server = makeServer(port);
    assert(server->valid());
    server->publish({{"settings", settingsDocument(settings)}, {"title", "reconnected"}}, settings,
                    1);
    await([&] {
        return client.connected() &&
               client.snapshot().value("title", std::string{}) == "reconnected";
    });
    client.stop();
    server.reset();
    std::cout << "WebSocket handshake, masked large frames, live updates, HTTP commands, browse, "
                 "server shutdown and client reconnect passed.\n";
}
