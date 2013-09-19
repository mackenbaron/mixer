/*
 * controller.cpp
 *
 *  Created on: Jul 22, 2013
 *      Author: palau
 */



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <map>
#include <string>
#include <iostream>

#include "Jzon.h"
#include "mixer.h"

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);
void stop_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);
void add_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void modify_layout(Jzon::Object rootNode, Jzon::Object *outRootNode);
void enable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void disable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_stream(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_destinations(Jzon::Object rootNode, Jzon::Object *outRootNode);
void get_destination(Jzon::Object rootNode, Jzon::Object *outRootNode);
void exit_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode);
void initialize_action_mapping();
int get_socket(int port, int *sock, int *new_sock);

std::map<std::string, void(*)(Jzon::Object, Jzon::Object*)> commands;
Jzon::Object rootNode, root_response;
Jzon::Parser parser(rootNode);
Jzon::Writer writer(root_response, Jzon::NoFormat);
bool should_stop=false;
mixer *m;

int main(int argc, char *argv[]){

    int sockfd, newsockfd, portno, n;
    const char* res;
    std::string result; 
    char buffer[256];
     
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    portno = atoi(argv[1]);
    get_socket(portno, &sockfd, &newsockfd);

    initialize_action_mapping();
    m = mixer::get_instance();

    while(!should_stop){
        bzero(buffer,256);
        rootNode.Clear();
        root_response.Clear();
        n = read(newsockfd,buffer,255);
        if (n < 0) error("ERROR reading from socket");
        if (n != 0) {
            std::cout << "Buffer_in:" << buffer << std::endl;
            parser.SetJson(buffer);
            if (!parser.Parse()) {
                std::cout << "Error: " << parser.GetError() << std::endl;
            }else { 
                commands[rootNode.Get("action").ToString()](rootNode, &root_response);
                writer.Write();
                result = writer.GetResult();
                std::cout << "Result:" << result << std::endl;
                res = result.c_str();
                n = write(newsockfd,res,result.size());
            }
        }
    }

     close(newsockfd);
     close(sockfd);
     return 0; 
 }

int get_socket(int port, int *sock, int *new_sock){
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    *sock = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock < 0) 
       error("ERROR opening socket");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);
    if (bind(*sock, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) 
             error("ERROR on binding");
    listen(*sock,1);
    clilen = sizeof(cli_addr);
    *new_sock = accept(*sock, 
                (struct sockaddr *) &cli_addr, 
                &clilen);
    if (*new_sock < 0) 
         error("ERROR on accept");

     return 0;
}


void initialize_action_mapping() {
    commands["start_mixer"] = start_mixer;
    commands["stop_mixer"] = stop_mixer;
    commands["add_stream"] = add_stream;
    commands["remove_stream"] = remove_stream;
    commands["modify_stream"] = modify_stream;
    commands["modify_layout"] = modify_layout;
    commands["add_destination"] = add_destination;
    commands["enable_stream"] = enable_stream;
    commands["disable_stream"] = disable_stream;
    commands["remove_destination"] = remove_destination;
    commands["get_streams"] = get_streams;
    commands["get_stream"] = get_stream;
    commands["get_destinations"] = get_destinations;
    commands["get_destination"] = get_destination;
    commands["exit_mixer"] = exit_mixer;
}

void start_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    int max_streams = rootNode.Get("params").Get("max_streams").ToInt();
    int in_port = rootNode.Get("params").Get("input_port").ToInt();
    int out_port = 56;
    printf("m->init(%d, %d, %d, %d, %d);\nm->exec()\n", 
        width, height, max_streams, in_port, out_port);
    m->init(width, height, max_streams, in_port, out_port); 
    m->exec();
    outRootNode->Add("error", Jzon::null);   
}

void stop_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    outRootNode->Add("error", Jzon::null);
    m->stop();
    printf("m->stop()");  
}

void add_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    if (m->add_source(width, height, H264) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->add_source(%d, %d, H264)\n", width, height);
    }
}

void remove_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    if (m->remove_source(id) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->remove_source(%d)\n", id);
    }
}

void modify_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    int x = rootNode.Get("params").Get("x").ToInt();
    int y = rootNode.Get("params").Get("y").ToInt();
    int layer = rootNode.Get("params").Get("layer").ToInt();
    bool keep_aspect_ratio = rootNode.Get("params").Get("keep_aspect_ratio").ToBool();
    if (m->modify_stream(id, width, height, x, y, layer, keep_aspect_ratio) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->modify_stream(%d, %d, %d, %d, %d, %d, %d);\n", 
            id, width, height, x, y, layer, keep_aspect_ratio);
    }
}

void modify_layout(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int width = rootNode.Get("params").Get("width").ToInt();
    int height = rootNode.Get("params").Get("height").ToInt();
    bool resize_streams = rootNode.Get("params").Get("resize_streams").ToBool();
    if (m->resize_output(width, height, resize_streams) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->resize_output(%d, %d, %d)\n", width, height, resize_streams);
    } 
}

void enable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    outRootNode->Add("error", Jzon::null);
    printf("m->enable_stream(%d)\n", id);
}

void disable_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    outRootNode->Add("error", Jzon::null);
    printf("m->disable_stream(%d)\n", id);
}

void add_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    std::string ip_string = rootNode.Get("params").Get("ip").ToString();
    int port = rootNode.Get("params").Get("port").ToInt();
    char *ip = new char[ip_string.length() + 1];
    strcpy(ip, ip_string.c_str());

    if (m->add_destination(H264, ip, port) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->add_destination(H264, %s, %d)\n", ip, port);
    } 
}

void remove_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    outRootNode->Add("error", Jzon::null);
    if (m->remove_destination(id) == -1){
        outRootNode->Add("error", "errore");
    }else {
        outRootNode->Add("error", Jzon::null);
        printf("m->remove_destination(%d)\n", id);
    } 
}

void get_streams(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int i;
    Jzon::Array list;
    std::vector<int> streams_id = m->get_streams_id();
    for (i=0; i<streams_id.size(); i++){
        Jzon::Object stream;
        std::map<std::string, int> stream_map;
        m->get_stream_info(stream_map, streams_id[i]);
        stream.Add("id", stream_map["id"]);
        stream.Add("orig_width", stream_map["orig_width"]);
        stream.Add("orig_height", stream_map["orig_height"]);
        stream.Add("width", stream_map["width"]);
        stream.Add("height", stream_map["height"]);
        stream.Add("x", stream_map["x"]);
        stream.Add("y", stream_map["y"]);
        stream.Add("layer", stream_map["layer"]);
        stream.Add("active", stream_map["active"]);
        list.Add(stream);
    }
    outRootNode->Add("streams", list);
}

void get_stream(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    std::map<std::string, int> stream_map;
    m->get_stream_info(stream_map, id);
    outRootNode->Add("id", stream_map["id"]);
    outRootNode->Add("orig_width", stream_map["orig_width"]);
    outRootNode->Add("orig_height", stream_map["orig_height"]);
    outRootNode->Add("width", stream_map["width"]);
    outRootNode->Add("height", stream_map["height"]);
    outRootNode->Add("x", stream_map["x"]);
    outRootNode->Add("y", stream_map["y"]);
    outRootNode->Add("layer", stream_map["layer"]);
    outRootNode->Add("active", stream_map["active"]);
}

void get_destinations(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int i;
    Jzon::Array list;
    map<uint32_t, mixer::Dst> dst_map = m->get_destinations();
    std::map<uint32_t,mixer::Dst>::iterator it;
    for (it=dst_map.begin(); it!=dst_map.end(); it++){
        Jzon::Object dst;
        string ip;
        int port;
        m->get_destination(it->first, ip, &port);
        dst.Add("id", (int)it->first);
        dst.Add("ip", ip);
        dst.Add("port", port);
        list.Add(dst);
    }
    outRootNode->Add("destinations", list);
}
void get_destination(Jzon::Object rootNode, Jzon::Object *outRootNode){
    int id = rootNode.Get("params").Get("id").ToInt();
    int port;
    string ip;
    if (m->get_destination(id, ip, &port) == -1){
        outRootNode->Add("error", "Destination ID not found");
    } else {
        outRootNode->Add("id", id);
        outRootNode->Add("ip", ip);
        outRootNode->Add("port", port);
    }
}

void exit_mixer(Jzon::Object rootNode, Jzon::Object *outRootNode){
    outRootNode->Add("error", Jzon::null);
    should_stop = true;
}