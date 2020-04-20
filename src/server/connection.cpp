#include "connection.h"
#include "database.h"
#include <cmath>
#include <json/json.h>

const double EPS = 1e-7;

Json::Value to_json(const Entry& entry) {
    Json::Value root;
    root["id"] = (int32_t)entry.id;
    root["price"] = entry.robot.price;
    root["weight"] = entry.robot.weight;
    root["name"] = entry.robot.name;
    return root;
}

Robot from_json(const Json::Value json) { // TODO: check if all fields are present
    Robot robot;
    robot.price =  json["price"].asInt();
    robot.weight = json["weight"].asFloat();
    robot.name =   json["name"].asString();
    return robot;
}

Json::Value DBConnection::ping(const Json::Value& argument) {
    Json::Value root;
    root["status"] = 200;
    return root;
}

Json::Value DBConnection::add(const Json::Value& argument) {
    Robot robot = from_json(argument);
    db.add(robot);
    Json::Value root;
    root["status"] = 200;
    root["result"] = (int32_t)db.get_total_entries()-1;
    return root;
}

Json::Value DBConnection::remove(const Json::Value& argument) {
    size_t id = argument.asLargestUInt();
    Json::Value root;
    try { 
        db.remove(id);
    } catch (std::runtime_error& e) {
        root["status"] = 404;
        return root;
    }
    root["status"] = 200;
    return root;
}

Json::Value DBConnection::update(const Json::Value& argument) {
    size_t id = argument["id"].asLargestUInt();
    Robot robot = from_json(argument);
    Json::Value root;
    try { 
        db.update(id, robot);
    } catch (std::runtime_error& e) {
        root["status"] = 404;
        return root;
    }
    root["status"] = 200;
    return root;
}

Json::Value DBConnection::find(const Json::Value& argument) {
    size_t id = argument.asLargestUInt();
    Json::Value root;
    Entry entry;
    try {
        entry = db.find(id);
    } catch (std::runtime_error& e) {
        root["status"] = 404;
        return root;
    }
    root["result"] = to_json(entry);
    root["status"] = 200;
    return root;
}

Json::Value DBConnection::find_all(const Json::Value& argument) {
    std::function<bool(const Robot&)> predicate;
    if (argument.isMember("price")) {
        predicate = [&](const Robot& r) {
            return r.price == argument["price"].asInt();
        };
    } else if (argument.isMember("name")) {
        predicate = [&](const Robot& r) {
            return r.name == argument["name"].asString();
        };
    } else if (argument.isMember("weight")) {
        predicate = [&](const Robot& r) {
            return fabs(r.weight - argument["weight"].asFloat()) < EPS;
        };
    } else {
        predicate = [](const Robot& r) { return true; };
    }
    std::vector<Entry> found = db.find_all(predicate);
    Json::Value root;
    root["status"] = 200;
    root["result"] = Json::arrayValue;
    root["result"].resize(found.size());
    for (size_t e = 0; e < found.size(); e++) {
        root["result"][(int32_t)e] = to_json(found[e]);
    }
    return root;
}

std::string DBConnection::process(const std::string& request) {
    Json::Value command;
    Json::Value response;
    if (!reader.parse(request, command)) {
        return "{\"status\":400}\n";
    }
    std::string command_type = command["command"].asString();
    Json::Value argument = command["arg"];
         if (command_type == "add")      response = add(argument);
    else if (command_type == "remove")   response = remove(argument);
    else if (command_type == "update")   response = update(argument);
    else if (command_type == "find")     response = find(argument);
    else if (command_type == "find_all") response = find_all(argument);
    else if (command_type == "ping")     response = ping(argument);
    else return "{\"status\": 400}\n";
    return writer.write(response);
}



